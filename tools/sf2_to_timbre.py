#!/usr/bin/env python3
"""
SoundFont 2 (.sf2) ファイルから CapsuleSampler 用のティンバーデータ
(48kHz/16bit/mono の WAV と JSON 定義) を生成するツール。

忠実な変換は目的とせず、おおまかにプレイアブルなティンバーへ
近似変換することを目的とする。
- SF2 のフィルタ / LFO / モジュレーションエンベロープは無視する
- delay/hold エンベロープフェーズは無視する
- ステレオサンプルはモノラルにダウンミックスする
- 微小チューニング (pitch_correction / fineTune の端数) は四捨五入で root note に丸める
- ゾーンが部分的に重なる場合は、ノート/ベロシティ軸で分割して
  「完全一致 or 完全分離」になるよう正規化する

使用例:
    # プリセット一覧を表示
    python sf2_to_timbre.py --sf2 GeneralUser.sf2 --list

    # bank=0, program=0 のプリセットを 'piano2' という名前で出力
    python sf2_to_timbre.py --sf2 GeneralUser.sf2 \\
        --preset 0:0 --name piano2 --category keys
"""

import argparse
import io
import json
import math
import os
import sys
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple

import numpy as np
import soundfile as sf
from scipy.signal import resample_poly

from sf2utils.sf2parse import Sf2File


# ---------------------------------------------------------------------------
# SF2 generator opcode constants
# ---------------------------------------------------------------------------
GEN_START_ADDRS_OFFSET = 0
GEN_END_ADDRS_OFFSET = 1
GEN_STARTLOOP_ADDRS_OFFSET = 2
GEN_ENDLOOP_ADDRS_OFFSET = 3
GEN_START_ADDRS_COARSE = 4
GEN_END_ADDRS_COARSE = 12
GEN_STARTLOOP_ADDRS_COARSE = 45
GEN_ENDLOOP_ADDRS_COARSE = 50
GEN_DELAY_VOL_ENV = 33
GEN_ATTACK_VOL_ENV = 34
GEN_HOLD_VOL_ENV = 35
GEN_DECAY_VOL_ENV = 36
GEN_SUSTAIN_VOL_ENV = 37
GEN_RELEASE_VOL_ENV = 38
GEN_KEY_RANGE = 43
GEN_VEL_RANGE = 44
GEN_COARSE_TUNE = 51
GEN_FINE_TUNE = 52
GEN_SAMPLE_MODES = 54
GEN_OVERRIDING_ROOT_KEY = 58

TARGET_SAMPLE_RATE = 48000
LOOP_TAIL_PADDING = 1024
SILENT_LOOP_LEN = 64       # ループ無し SF2 サンプルに付ける無音ループ区間の長さ
ENV_TARGET_RATIO = 0.01  # decay/release が「目標値の1%以内」に到達する時間で換算


# ---------------------------------------------------------------------------
# sf2utils accessor helpers
# ---------------------------------------------------------------------------
def _gen_short(bag_or_gens, oper: int) -> Optional[int]:
    """signed short 値の generator を取得。無ければ None。"""
    gens = bag_or_gens if isinstance(bag_or_gens, dict) else (
        getattr(bag_or_gens, "gens", None) or {})
    g = gens.get(oper)
    if g is None:
        return None
    if hasattr(g, "short"):
        return int(g.short)
    if hasattr(g, "amount"):
        v = int(g.amount)
        return v - 0x10000 if v >= 0x8000 else v
    return None


def _gen_word(bag_or_gens, oper: int) -> Optional[int]:
    """unsigned word 値の generator を取得。"""
    gens = bag_or_gens if isinstance(bag_or_gens, dict) else (
        getattr(bag_or_gens, "gens", None) or {})
    g = gens.get(oper)
    if g is None:
        return None
    if hasattr(g, "word"):
        return int(g.word)
    if hasattr(g, "amount"):
        return int(g.amount) & 0xFFFF
    return None


def _gen_range(bag_or_gens, oper: int) -> Optional[Tuple[int, int]]:
    gens = bag_or_gens if isinstance(bag_or_gens, dict) else (
        getattr(bag_or_gens, "gens", None) or {})
    g = gens.get(oper)
    if g is None:
        return None
    if hasattr(g, "amount_as_sorted_range"):
        r = g.amount_as_sorted_range
        if isinstance(r, (tuple, list)) and len(r) == 2:
            return (int(r[0]), int(r[1]))
    lo = getattr(g, "amount_lo_byte", None)
    hi = getattr(g, "amount_hi_byte", None)
    if lo is not None and hi is not None:
        a, b = int(lo), int(hi)
        return (min(a, b), max(a, b))
    return None


# ---------------------------------------------------------------------------
# Zone extraction
# ---------------------------------------------------------------------------
@dataclass
class Zone:
    """1つの (key range, vel range, sample, generators) の組。"""
    key_lo: int
    key_hi: int
    vel_lo: int
    vel_hi: int
    sample: object
    sample_modes: int
    delay_tc: int
    attack_tc: int
    hold_tc: int
    decay_tc: int
    sustain_cb: int
    release_tc: int
    coarse_tune: int
    fine_tune: int
    overriding_root: Optional[int]
    start_offset: int
    end_offset: int
    startloop_offset: int
    endloop_offset: int


def _merge_gens(parent: dict, child: dict) -> dict:
    """instrument global zone の gens を base にして bag の gens を上書き。"""
    out = dict(parent)
    out.update(child)
    return out


def _bag_gens_dict(bag) -> dict:
    return dict(getattr(bag, "gens", None) or {})


def _bag_key_range(bag, fallback_gens: dict) -> Optional[Tuple[int, int]]:
    r = getattr(bag, "key_range", None)
    if isinstance(r, (tuple, list)) and len(r) == 2 and r[0] is not None:
        return (int(r[0]), int(r[1]))
    return _gen_range(fallback_gens, GEN_KEY_RANGE)


def _bag_vel_range(bag, fallback_gens: dict) -> Optional[Tuple[int, int]]:
    r = getattr(bag, "velocity_range", None)
    if isinstance(r, (tuple, list)) and len(r) == 2 and r[0] is not None:
        return (int(r[0]), int(r[1]))
    return _gen_range(fallback_gens, GEN_VEL_RANGE)


def _zones_from_instrument(instr,
                           preset_key_range: Optional[Tuple[int, int]],
                           preset_vel_range: Optional[Tuple[int, int]]) -> List[Zone]:
    """1つの Sf2Instrument から Zone のリストを作る。"""
    zones: List[Zone] = []
    inst_bags = list(getattr(instr, "bags", []) or [])

    # global instrument zone (sample 未指定の bag) — 最初の1つをデフォルトとして採用
    global_gens: dict = {}
    for bag in inst_bags:
        if getattr(bag, "sample", None) is None:
            global_gens = _bag_gens_dict(bag)
            break

    for bag in inst_bags:
        sample = getattr(bag, "sample", None)
        if sample is None:
            continue
        if getattr(sample, "is_rom", False):
            continue

        merged_gens = _merge_gens(global_gens, _bag_gens_dict(bag))

        # 範囲は bag のプロパティを優先 (sf2utils が正しく解釈してくれる)
        kr = _bag_key_range(bag, merged_gens) or (0, 127)
        vr = _bag_vel_range(bag, merged_gens) or (0, 127)
        key_lo, key_hi = kr
        vel_lo, vel_hi = vr

        if preset_key_range:
            key_lo = max(key_lo, preset_key_range[0])
            key_hi = min(key_hi, preset_key_range[1])
        if preset_vel_range:
            vel_lo = max(vel_lo, preset_vel_range[0])
            vel_hi = min(vel_hi, preset_vel_range[1])
        if key_lo > key_hi or vel_lo > vel_hi:
            continue

        def short(oper, default=0):
            v = _gen_short(merged_gens, oper)
            return default if v is None else v

        def word(oper, default=0):
            v = _gen_word(merged_gens, oper)
            return default if v is None else v

        zones.append(Zone(
            key_lo=key_lo,
            key_hi=key_hi,
            vel_lo=vel_lo,
            vel_hi=vel_hi,
            sample=sample,
            sample_modes=word(GEN_SAMPLE_MODES, 0),
            delay_tc=short(GEN_DELAY_VOL_ENV, -12000),
            attack_tc=short(GEN_ATTACK_VOL_ENV, -12000),
            hold_tc=short(GEN_HOLD_VOL_ENV, -12000),
            decay_tc=short(GEN_DECAY_VOL_ENV, -12000),
            sustain_cb=short(GEN_SUSTAIN_VOL_ENV, 0),
            release_tc=short(GEN_RELEASE_VOL_ENV, -12000),
            coarse_tune=short(GEN_COARSE_TUNE, 0),
            fine_tune=short(GEN_FINE_TUNE, 0),
            overriding_root=_gen_short(merged_gens, GEN_OVERRIDING_ROOT_KEY),
            start_offset=(short(GEN_START_ADDRS_COARSE, 0) * 32768
                          + short(GEN_START_ADDRS_OFFSET, 0)),
            end_offset=(short(GEN_END_ADDRS_COARSE, 0) * 32768
                        + short(GEN_END_ADDRS_OFFSET, 0)),
            startloop_offset=(short(GEN_STARTLOOP_ADDRS_COARSE, 0) * 32768
                              + short(GEN_STARTLOOP_ADDRS_OFFSET, 0)),
            endloop_offset=(short(GEN_ENDLOOP_ADDRS_COARSE, 0) * 32768
                            + short(GEN_ENDLOOP_ADDRS_OFFSET, 0)),
        ))

    return zones


def collect_zones(preset) -> List[Zone]:
    """Sf2Preset 全体から Zone を集める。"""
    zones: List[Zone] = []
    preset_bags = list(getattr(preset, "bags", []) or [])
    for bag in preset_bags:
        instr = getattr(bag, "instrument", None)
        if instr is None:
            continue
        merged_gens = _bag_gens_dict(bag)
        pkr = _bag_key_range(bag, merged_gens)
        pvr = _bag_vel_range(bag, merged_gens)
        zones.extend(_zones_from_instrument(instr, pkr, pvr))
    return zones


# ---------------------------------------------------------------------------
# Audio extraction & loop tail synthesis
# ---------------------------------------------------------------------------
def _read_sample_pcm(sample) -> np.ndarray:
    """Sf2Sample から int16 mono numpy 配列を取得。"""
    raw = None
    for attr in ("raw_sample_data", "smpl"):
        if hasattr(sample, attr):
            try:
                raw = getattr(sample, attr)
                if callable(raw):
                    raw = raw()
                break
            except Exception:
                raw = None
    if raw is None:
        raise RuntimeError(f"sample {getattr(sample,'name','?')} のデータを取得できません")
    if isinstance(raw, (bytes, bytearray, memoryview)):
        arr = np.frombuffer(bytes(raw), dtype="<i2").astype(np.int16)
    else:
        arr = np.asarray(raw, dtype=np.int16)
    return arr


def _resample_to_target(pcm: np.ndarray, src_rate: int) -> Tuple[np.ndarray, float]:
    """48kHz/int16 にリサンプル。戻り値は (data, ratio=tgt/src)。"""
    if src_rate == TARGET_SAMPLE_RATE:
        return pcm.astype(np.int16, copy=False), 1.0
    g = math.gcd(TARGET_SAMPLE_RATE, src_rate)
    up = TARGET_SAMPLE_RATE // g
    down = src_rate // g
    f = pcm.astype(np.float32)
    out = resample_poly(f, up, down).astype(np.float32)
    out = np.clip(out, -32768, 32767).astype(np.int16)
    return out, TARGET_SAMPLE_RATE / src_rate


def _trim_tail_below(pcm: np.ndarray, threshold_dbfs: float,
                     safety_samples: int = 1024) -> np.ndarray:
    """振幅が threshold_dbfs を下回ったところで pcm の末尾を切り詰める。"""
    if len(pcm) == 0:
        return pcm
    threshold = (10.0 ** (threshold_dbfs / 20.0)) * 32767.0
    abs_pcm = np.abs(pcm.astype(np.int32))
    above = np.where(abs_pcm > threshold)[0]
    if len(above) == 0:
        return pcm[:1]
    last = int(above[-1]) + 1 + safety_samples
    return pcm[:min(last, len(pcm))]


def build_sample_buffer(zone: Zone,
                        trim_tail_db: Optional[float] = None
                        ) -> Tuple[np.ndarray, int, int, bool]:
    """
    ゾーンから WAV 用 PCM、loop_start、loop_end、adsr_enabled を返す。
    CapsuleSampler の要求どおりループ末尾に複製とパディングを付加する。

    trim_tail_db が指定された場合、ループ無しサンプルでは指定 dBFS を下回った
    末尾を切り詰める (ループサンプルでは末尾は固定の無音パディングなので不要)。
    """
    s = zone.sample
    pcm_all = _read_sample_pcm(s)

    sample_start = 0  # raw_sample_data は既にこのサンプル分のデータ
    sample_end = len(pcm_all)

    # sf2utils は sample.start_loop / end_loop をサンプル先頭からの
    # 相対位置として返してくる。そこに generator offset を加算する。
    rel_loop_start = max(0, int(getattr(s, "start_loop", 0))) + zone.startloop_offset
    rel_loop_end = max(0, int(getattr(s, "end_loop", 0))) + zone.endloop_offset
    rel_start = sample_start + zone.start_offset
    rel_end = sample_end + zone.end_offset  # end_offset は通常負

    rel_start = max(0, min(len(pcm_all), rel_start))
    rel_end = max(rel_start, min(len(pcm_all), rel_end))
    pcm = pcm_all[rel_start:rel_end]

    rel_loop_start -= zone.start_offset  # pcm がトリミング後なので合わせる
    rel_loop_end -= zone.start_offset
    rel_loop_start = max(0, min(len(pcm), rel_loop_start))
    rel_loop_end = max(rel_loop_start, min(len(pcm), rel_loop_end))

    src_rate = int(getattr(s, "sample_rate", TARGET_SAMPLE_RATE))
    pcm48, ratio = _resample_to_target(pcm, src_rate)
    loop_start = int(round(rel_loop_start * ratio))
    loop_end = int(round(rel_loop_end * ratio))
    loop_start = max(0, min(len(pcm48), loop_start))
    loop_end = max(loop_start, min(len(pcm48), loop_end))

    use_loop = zone.sample_modes in (1, 3) and loop_end > loop_start

    if use_loop:
        # ループポイントの後ろにループ区間先頭から LOOP_TAIL_PADDING サンプル分を置く。
        # CapsuleSampler の player は loop_end を超えて少し先読みするため、その範囲に
        # ループ区間先頭の波形が連続して並んでいる必要がある。
        # ループ区間そのものを完全複製する必要はない。
        head = pcm48[:loop_end]
        loop_region = pcm48[loop_start:loop_end]
        tail = _make_loop_tail(loop_region, LOOP_TAIL_PADDING)
        out = np.concatenate([head, tail])
        return out, loop_start, loop_end, True
    else:
        # SF2 上はループ無し。サンプル末尾の直後に短い無音ループ区間を作り、
        # adsr=true 化することで note-off 時に release が効くようにする。
        # ワンショットの場合は loop_end の後ろに 1024 サンプルの無音を置けばよい。
        body = pcm48
        if trim_tail_db is not None:
            body = _trim_tail_below(body, trim_tail_db)
        loop_start = len(body)
        loop_end = loop_start + SILENT_LOOP_LEN
        tail = np.zeros(SILENT_LOOP_LEN + LOOP_TAIL_PADDING, dtype=np.int16)
        out = np.concatenate([body, tail])
        return out, loop_start, loop_end, True


def _make_loop_tail(loop_region: np.ndarray, length: int) -> np.ndarray:
    """ループ区間先頭から length サンプル分のデータを作る。
    ループ区間が length より短ければ繰り返してフィルする。"""
    if len(loop_region) == 0:
        return np.zeros(length, dtype=np.int16)
    if len(loop_region) >= length:
        return loop_region[:length].astype(np.int16, copy=True)
    n = (length + len(loop_region) - 1) // len(loop_region)
    return np.tile(loop_region, n)[:length].astype(np.int16, copy=True)


# ---------------------------------------------------------------------------
# ADSR conversion (SF2 timecents/centibels -> CapsuleSampler coefficients)
# ---------------------------------------------------------------------------
def _tc_to_seconds(timecents: int) -> float:
    if timecents <= -12000:
        return 0.0
    return 2.0 ** (timecents / 1200.0)


def _attack_coeff(seconds: float) -> float:
    if seconds <= 0:
        return 1.0
    n = seconds * TARGET_SAMPLE_RATE / 64.0
    if n <= 1:
        return 1.0
    v = 1.0 / n
    return max(min(v, 1.0), 1e-4)


def _decay_or_release_coeff(seconds: float) -> float:
    """T 秒で初期値の ENV_TARGET_RATIO まで減衰する係数。"""
    if seconds <= 0:
        return 0.0
    n = seconds * TARGET_SAMPLE_RATE / 64.0
    if n < 1:
        return 0.0
    k = math.exp(math.log(ENV_TARGET_RATIO) / n)
    return max(min(k, 0.99999), 0.0)


def _sustain_level(centibels: int) -> float:
    cb = max(0, min(1000, centibels))
    return 10.0 ** (-cb / 200.0)


def adsr_from_zone(zone: Zone, has_loop: bool) -> Tuple[bool, float, float, float, float]:
    """(adsr_enabled, attack, decay, sustain, release)
    build_sample_buffer により全ゾーンに無音ループ区間が用意されるため、
    ADSR は常に有効化する。"""
    attack = _attack_coeff(_tc_to_seconds(zone.attack_tc))
    decay = _decay_or_release_coeff(_tc_to_seconds(zone.decay_tc))
    sustain = _sustain_level(zone.sustain_cb)
    release = _decay_or_release_coeff(_tc_to_seconds(zone.release_tc))
    return True, attack, decay, sustain, release


# ---------------------------------------------------------------------------
# Effective root key (簡略化)
# ---------------------------------------------------------------------------
def effective_root_key(zone: Zone) -> int:
    s = zone.sample
    base = zone.overriding_root
    if base is None or base < 0:
        base = int(getattr(s, "original_pitch", 60))
    pitch_correction_cents = int(getattr(s, "pitch_correction", 0))
    total_cents = (zone.coarse_tune * 100) + zone.fine_tune + pitch_correction_cents
    # CapsuleSampler は半音単位。端数はここで丸めて落とす。
    root = base - int(round(total_cents / 100.0))
    return max(0, min(127, root))


# ---------------------------------------------------------------------------
# Zone flattening (CapsuleSampler の制約に合わせて分割)
# ---------------------------------------------------------------------------
@dataclass
class FlatZone:
    key_lo: int
    key_hi: int
    vel_lo: int
    vel_hi: int
    src: Zone


def _zone_signature(z: Zone) -> tuple:
    """CapsuleSampler の出力に影響する Zone のフィールド一式から一意キーを作る。
    SF2 上は別 bag/Zone でも、結果として同一サンプル+同一 ADSR+同一 root に
    なるものは同一視される。"""
    return (
        id(z.sample), z.sample_modes,
        z.start_offset, z.end_offset,
        z.startloop_offset, z.endloop_offset,
        z.attack_tc, z.decay_tc, z.sustain_cb, z.release_tc,
        z.coarse_tune, z.fine_tune, z.overriding_root,
    )


def flatten_zones(zones: List[Zone]) -> List[FlatZone]:
    """
    CapsuleSampler の制約:
      - 任意の2ゾーンのキー範囲は完全一致 or 完全分離
      - 同じキー範囲内ではベロシティ範囲が重複しない
    部分的に重なる SF2 ゾーンはキー軸/ベロシティ軸で分割して制約を満たす。
    複数候補が同じセルを覆う場合は range が狭い方を優先する。
    """
    if not zones:
        return []

    # キー軸の境界点を集めて分割する
    key_points = set()
    for z in zones:
        key_points.add(z.key_lo)
        key_points.add(z.key_hi + 1)
    key_bounds = sorted(key_points)
    key_segments: List[Tuple[int, int]] = []
    for i in range(len(key_bounds) - 1):
        lo = key_bounds[i]
        hi = key_bounds[i + 1] - 1
        if hi >= lo:
            key_segments.append((lo, hi))

    flat: List[FlatZone] = []
    for klo, khi in key_segments:
        # このキー区間に重なる zones
        overlapping = [z for z in zones if z.key_lo <= klo and z.key_hi >= khi]
        if not overlapping:
            continue
        # ベロシティ軸の境界
        vel_points = set()
        for z in overlapping:
            vel_points.add(z.vel_lo)
            vel_points.add(z.vel_hi + 1)
        vel_bounds = sorted(vel_points)
        for j in range(len(vel_bounds) - 1):
            vlo = vel_bounds[j]
            vhi = vel_bounds[j + 1] - 1
            if vhi < vlo:
                continue
            cands = [z for z in overlapping if z.vel_lo <= vlo and z.vel_hi >= vhi]
            if not cands:
                continue
            # 範囲が狭いものを優先 (最も specific なゾーンを採用)
            cands.sort(key=lambda z: (
                (z.key_hi - z.key_lo) * 128 + (z.vel_hi - z.vel_lo)
            ))
            flat.append(FlatZone(klo, khi, vlo, vhi, cands[0]))

    # signature が一致するゾーンは「論理的に同じ」として同一視してマージする。
    sig_of = {id(fz): _zone_signature(fz.src) for fz in flat}

    # 1) 同じ signature & 同じベロシティ範囲で隣接するキー区間をマージ
    flat.sort(key=lambda f: (f.vel_lo, f.vel_hi, sig_of[id(f)], f.key_lo))
    merged: List[FlatZone] = []
    for fz in flat:
        if (merged
                and sig_of[id(merged[-1])] == sig_of[id(fz)]
                and merged[-1].vel_lo == fz.vel_lo
                and merged[-1].vel_hi == fz.vel_hi
                and merged[-1].key_hi + 1 == fz.key_lo):
            new = FlatZone(merged[-1].key_lo, fz.key_hi,
                           fz.vel_lo, fz.vel_hi, fz.src)
            sig_of[id(new)] = sig_of[id(fz)]
            merged[-1] = new
        else:
            merged.append(fz)

    # 2) 同じキー範囲 & 同じ signature で隣接する/同一ベロシティ区間をマージ。
    #    SF2 側でベロシティレイヤごとに gain やフィルタが違っても
    #    CapsuleSampler では表現できないため、出力上の区別がない区間は統合する。
    merged.sort(key=lambda f: (f.key_lo, f.key_hi, sig_of[id(f)], f.vel_lo))
    merged2: List[FlatZone] = []
    for fz in merged:
        if (merged2
                and sig_of[id(merged2[-1])] == sig_of[id(fz)]
                and merged2[-1].key_lo == fz.key_lo
                and merged2[-1].key_hi == fz.key_hi
                and merged2[-1].vel_hi + 1 >= fz.vel_lo):
            new = FlatZone(fz.key_lo, fz.key_hi,
                           merged2[-1].vel_lo,
                           max(merged2[-1].vel_hi, fz.vel_hi),
                           fz.src)
            sig_of[id(new)] = sig_of[id(fz)]
            merged2[-1] = new
        else:
            merged2.append(fz)

    merged2.sort(key=lambda f: (f.key_lo, f.vel_lo))
    return merged2


def thin_zones(flat: List[FlatZone], key_step: int) -> List[FlatZone]:
    """
    ベロシティレイヤーごとに、root note の差が key_step 半音以上離れたゾーンだけを
    残し、間引かれたゾーンのキー範囲は直前の残ったゾーンに併合する。
    key_step <= 1 のときは何もしない。
    """
    if key_step <= 1 or not flat:
        return flat

    from collections import defaultdict
    groups: Dict[Tuple[int, int], List[FlatZone]] = defaultdict(list)
    for fz in flat:
        groups[(fz.vel_lo, fz.vel_hi)].append(fz)

    result: List[FlatZone] = []
    for _, zones in groups.items():
        zones.sort(key=lambda f: f.key_lo)
        kept: List[FlatZone] = []
        last_root: Optional[int] = None
        for fz in zones:
            root = effective_root_key(fz.src)
            if last_root is None or (root - last_root) >= key_step:
                kept.append(FlatZone(fz.key_lo, fz.key_hi,
                                     fz.vel_lo, fz.vel_hi, fz.src))
                last_root = root
            else:
                kept[-1].key_hi = max(kept[-1].key_hi, fz.key_hi)
        # 端の取りこぼしを最終ゾーンが拾うようにする
        if kept and zones:
            kept[-1].key_hi = max(kept[-1].key_hi, zones[-1].key_hi)
        result.extend(kept)

    result.sort(key=lambda f: (f.key_lo, f.vel_lo))
    return result


# ---------------------------------------------------------------------------
# Main conversion
# ---------------------------------------------------------------------------
def find_preset(sf2: Sf2File, spec: str):
    presets = [p for p in sf2.presets if getattr(p, "name", "EOP") != "EOP"]
    if ":" in spec:
        bank_str, prog_str = spec.split(":", 1)
        try:
            bank = int(bank_str)
            prog = int(prog_str)
        except ValueError:
            return None
        for p in presets:
            if int(getattr(p, "bank", -1)) == bank and int(getattr(p, "preset", -1)) == prog:
                return p
        return None
    # 名前一致
    for p in presets:
        if getattr(p, "name", "") == spec:
            return p
    # index
    try:
        idx = int(spec)
        return presets[idx]
    except (ValueError, IndexError):
        return None


def list_presets(sf2: Sf2File):
    print("bank:program  name")
    print("------------- ----")
    for p in sf2.presets:
        name = getattr(p, "name", "")
        if name == "EOP":
            continue
        bank = int(getattr(p, "bank", 0))
        prog = int(getattr(p, "preset", 0))
        print(f"{bank:>4}:{prog:<6}  {name}")


def convert(sf2_path: str, preset_spec: str, name: str, category: str,
            output_root: str,
            trim_tail_db: Optional[float] = None,
            key_step: int = 1) -> None:
    with open(sf2_path, "rb") as fh:
        sf2 = Sf2File(fh)

        preset = find_preset(sf2, preset_spec)
        if preset is None:
            print(f"プリセット {preset_spec!r} が見つかりません", file=sys.stderr)
            sys.exit(1)

        zones = collect_zones(preset)
        if not zones:
            print("変換可能なゾーンが見つかりません", file=sys.stderr)
            sys.exit(1)

        flat = flatten_zones(zones)
        if not flat:
            print("ゾーン正規化の結果が空になりました", file=sys.stderr)
            sys.exit(1)

        if key_step > 1:
            before = len(flat)
            flat = thin_zones(flat, key_step)
            print(f"  キー間引き: {before} → {len(flat)} ゾーン (step={key_step})")

        out_dir = os.path.join(output_root, name)
        os.makedirs(out_dir, exist_ok=True)

        # 同じ Sf2Sample + ループ設定は1つの WAV として共有する
        wav_cache: Dict[Tuple[int, int], str] = {}
        wav_meta: Dict[str, Tuple[int, int, bool]] = {}  # path -> (loop_start, loop_end, adsr)
        samples_json: List[dict] = []

        for fz in flat:
            z = fz.src
            sample_id = id(z.sample)
            cache_key = (sample_id, z.sample_modes,
                         z.start_offset, z.end_offset,
                         z.startloop_offset, z.endloop_offset)
            cache_path = wav_cache.get(cache_key)
            if cache_path is None:
                pcm, loop_start, loop_end, has_loop = build_sample_buffer(
                    z, trim_tail_db=trim_tail_db)
                base_name = getattr(z.sample, "name", f"sample_{len(wav_cache)}")
                safe = "".join(c if c.isalnum() or c in "-_" else "_" for c in base_name)
                if not safe:
                    safe = f"sample_{len(wav_cache)}"
                wav_filename = f"{safe}.wav"
                # 重複したら連番
                idx = 1
                while wav_filename in wav_meta:
                    wav_filename = f"{safe}_{idx}.wav"
                    idx += 1
                full_path = os.path.join(out_dir, wav_filename)
                sf.write(full_path, pcm, TARGET_SAMPLE_RATE, subtype="PCM_16")
                wav_cache[cache_key] = wav_filename
                wav_meta[wav_filename] = (loop_start, loop_end, has_loop)
                cache_path = wav_filename

            loop_start, loop_end, has_loop = wav_meta[cache_path]
            adsr_enabled, attack, decay, sustain, release = adsr_from_zone(z, has_loop)
            samples_json.append({
                "lower-note-no": fz.key_lo,
                "upper-note-no": fz.key_hi,
                "lower-velocity": fz.vel_lo,
                "upper-velocity": fz.vel_hi,
                "sample": {
                    "path": cache_path,
                    "root": effective_root_key(z),
                    "loop-start": loop_start,
                    "loop-end": loop_end,
                    "adsr-enabled": adsr_enabled,
                    "attack": round(attack, 6),
                    "decay": round(decay, 6),
                    "sustain": round(sustain, 6),
                    "release": round(release, 6),
                },
            })

        timbre = {
            "name": getattr(preset, "name", name),
            "category": category,
            "samples": samples_json,
        }
        json_path = os.path.join(out_dir, f"{name}.json")
        with open(json_path, "w", encoding="utf-8") as jf:
            json.dump(timbre, jf, ensure_ascii=False, indent=2)

        print(f"出力先: {out_dir}")
        print(f"  WAV: {len(wav_meta)} ファイル")
        print(f"  サンプルゾーン: {len(samples_json)} 個")


def main():
    parser = argparse.ArgumentParser(
        description="SF2 から CapsuleSampler 用ティンバーへ変換するツール"
    )
    parser.add_argument("--sf2", required=True, help="入力 SF2 ファイル")
    parser.add_argument("--list", action="store_true",
                        help="プリセット一覧を表示して終了")
    parser.add_argument("--preset",
                        help="プリセット指定 (bank:program / 名前 / index)")
    parser.add_argument("--name",
                        help="出力ティンバー名 (出力ディレクトリ名 兼 JSON ファイル名)")
    parser.add_argument("--category", default="keys",
                        help="ティンバーの category フィールド (default: keys)")
    parser.add_argument("--output-dir", default="../data/timbres",
                        help="出力先のティンバールートディレクトリ")
    parser.add_argument("--trim-tail-db", type=float, default=None,
                        help="ループなしサンプルで、振幅がこの dBFS を下回った"
                             "末尾を切り詰める (例: -60)")
    parser.add_argument("--key-step", type=int, default=1,
                        help="root note の差がこの半音数以上離れたゾーンだけ"
                             "残してキーを間引く (オクターブごとなら 12)")
    args = parser.parse_args()

    if args.list:
        with open(args.sf2, "rb") as fh:
            list_presets(Sf2File(fh))
        return

    if not args.preset or not args.name:
        parser.error("--preset と --name は --list 以外では必須です")

    convert(args.sf2, args.preset, args.name, args.category, args.output_dir,
            trim_tail_db=args.trim_tail_db, key_step=args.key_step)


if __name__ == "__main__":
    main()
