#pragma once

#include <stdint.h>
#include <algorithm>
#include "BatteryModel.h"

// 読んだ電池電圧と本体の状態から、表示する残量（%）を決める。
// ハードウェアに依存しないので、native-test でも検証できる。
// M5StopWatch-wararyoLauncher の BatteryEstimator を元に、負荷による補正と平均化を足したもの
class BatteryEstimator {
public:
    struct Reading {
        uint32_t nowMs = 0;
        int mv = 0;               // 電池電圧。0 以下は読み出し失敗
        bool powered = false;     // USB 給電がある
        bool charging = false;    // 充電中
        BatteryModel::LoadInput load;
    };

    // 充電電流で電池電圧が持ち上がる分。この機種では未測定で、M5StopWatch の実測値（44 mV）を仮に使う
    static constexpr int kChargeLiftMv = 44;
    // USB 給電があり充電していないとき、これ以上なら満充電とみなす。下回るなら、充電の一時停止や
    // 充電禁止で、電池は休んでいる
    static constexpr int kFullMinMv = 4100;
    // 給電・充電・負荷が変わった直後は電圧がまだ動いているので、この間は前の値を表示し続ける
    static constexpr uint32_t kSettleMs = 60 * 1000;
    // 平均する読み出しの回数。電圧のふらつき（瞬間的な負荷の増減）をならす
    static constexpr int kAverageCount = 6;

    // 読み出しを1回反映して、表示する残量を返す。まだ決まらないときは -1
    int update(const Reading& r) {
        const bool changed = hasCondition && (r.powered != lastPowered || r.charging != lastCharging || r.load != lastLoad);
        if (changed) {
            changedAtMs = r.nowMs;
            hasChange = true;
            sampleCount = 0;
            sampleNext = 0;
            // 給電の有無が変わったら、電池駆動中に表示を下げ続ける制約をやり直す
            if (r.powered != lastPowered) floorValid = false;
        }
        hasCondition = true;
        lastPowered = r.powered;
        lastCharging = r.charging;
        lastLoad = r.load;

        const bool settling = hasChange && (uint32_t)(r.nowMs - changedAtMs) < kSettleMs;
        if (settling && shown >= 0) return shown;
        if (r.mv <= 0) return shown;

        if (r.powered && !r.charging && r.mv >= kFullMinMv) {
            sampleCount = 0;
            sampleNext = 0;
            return shown = 100;
        }

        // 基準の負荷のときの電圧に直してから平均する
        int mv;
        if (r.powered) {
            // 給電中、電池の負荷は充電電流だけ（充電していなければ 0）
            mv = BatteryModel::compensateMv(r.charging ? r.mv - kChargeLiftMv : r.mv, 0);
        } else {
            mv = BatteryModel::compensateMv(r.mv, BatteryModel::estimateLoadMa(r.load));
        }
        samples[sampleNext] = mv;
        sampleNext = (sampleNext + 1) % kAverageCount;
        if (sampleCount < kAverageCount) sampleCount++;
        int sum = 0;
        for (int i = 0; i < sampleCount; ++i) sum += samples[i];
        int percent = BatteryModel::percentFromCurveMv((sum + sampleCount / 2) / sampleCount);

        // 100% は充電が終わったときだけ表示する
        if (r.powered && r.charging) percent = std::min(percent, 99);

        // 電池駆動中は表示を上げない。減光からの復帰などで電圧が戻り、数字が上がって見えるのを防ぐ
        if (!r.powered) {
            if (floorValid) percent = std::min(percent, shown);
            floorValid = true;
        }
        return shown = percent;
    }

    int getShown() const { return shown; }

private:
    int shown = -1;

    bool hasCondition = false;
    bool lastPowered = false;
    bool lastCharging = false;
    BatteryModel::LoadInput lastLoad;
    bool hasChange = false;
    uint32_t changedAtMs = 0;
    bool floorValid = false;

    int samples[kAverageCount] = {};
    int sampleCount = 0;
    int sampleNext = 0;
};
