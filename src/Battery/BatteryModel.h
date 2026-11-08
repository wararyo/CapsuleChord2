#pragma once

#include <stdint.h>
#include <algorithm>

// 電池残量の推定に使う、実機で測ったカーブと負荷のモデル。
// ハードウェアに依存しないので、native-test でも検証できる。
// 測定の詳細は docs/battery-life-improvement-plan.md の5章（2026-11-08 方法C 放電（S2））。
namespace BatteryModel {

struct CurvePoint { int mv; int percent; };

// 電池の残量（%）と、基準の負荷（kCurveLoadMa）で放電しているときの電池電圧の関係。
// 減光中（S2、電池側 約52 mA）で満充電から電源断まで放電させたときの、
// 電源断までに残っている時間の割合。一定の電流での放電なので、残っている電荷の割合とみなせる
inline constexpr CurvePoint kCurve[] = {
    {4080, 100}, {4050, 97}, {4000, 87}, {3950, 75}, {3900, 65}, {3850, 55},
    {3800, 42}, {3750, 34}, {3700, 28}, {3650, 24}, {3600, 20}, {3550, 16},
    {3500, 12}, {3450, 8}, {3400, 6}, {3350, 4}, {3300, 3}, {3250, 2},
    {3200, 2}, {3100, 1}, {3050, 0},
};

// kCurve を測ったときの電池電流
inline constexpr int kCurveLoadMa = 52;

// 電池の内部抵抗（配線や保護回路を含む）。S3（約188 mA）と S2（約52 mA）の放電で、
// 同じ残量の点の電圧差が 50〜120 mV だったことから求めた（約0.6〜0.9 Ω）
inline constexpr int kInternalResistanceMilliOhm = 700;

// 電圧から残量（%）を引く。点の間は線形補間し、両端は丸める
inline int percentFromCurveMv(int mv) {
    constexpr int count = int(sizeof(kCurve) / sizeof(kCurve[0]));
    if (mv >= kCurve[0].mv) return kCurve[0].percent;
    for (int i = 1; i < count; ++i) {
        const auto& hi = kCurve[i - 1];
        const auto& lo = kCurve[i];
        if (mv < lo.mv) continue;
        const int span = hi.mv - lo.mv;
        return lo.percent + ((hi.percent - lo.percent) * (mv - lo.mv) + span / 2) / span;
    }
    return kCurve[count - 1].percent;
}

// 電池電流が loadMa のときに読んだ電圧を、基準の負荷のときの電圧に直す。
// 電流が多いほど内部抵抗で電圧が下がるので、その分を足す
inline int compensateMv(int mv, int loadMa) {
    return mv + (loadMa - kCurveLoadMa) * kInternalResistanceMilliOhm / 1000;
}

// 電池電流の推定に使う、本体の状態
struct LoadInput {
    bool dimmed = false;            // 減光中（無操作による減光、または「[測定] 減光を強制」）
    uint8_t screenBrightness = 1;   // Settings.display.brightness（0: 明るい, 1: 普通, 2: 暗い）
    // 今後足す場合はここに足し、estimateLoadMa() に項を足す。
    // 方法A の実測（電池側に換算）では、キーの明るさは「明るい」+11 mA / 「暗い」−4 mA、
    // 演奏中（ドラム再生）は +15 mA 程度で、残量にすると数ポイント以内
    bool operator==(const LoadInput& o) const {
        return dimmed == o.dimmed && screenBrightness == o.screenBrightness;
    }
    bool operator!=(const LoadInput& o) const { return !(*this == o); }
};

// 減光中の電池電流。画面とキーは設定によらず同じ暗さになる
inline constexpr int kDimmedLoadMa = 52;
// 通常時（画面・キーの明るさ「普通」）の電池電流
inline constexpr int kActiveBaseLoadMa = 145;
// 画面の明るさによる増減（0: 明るい, 1: 普通, 2: 暗い）
inline constexpr int kScreenBrightnessLoadMa[] = {73, 0, -43};

// 電池電流を推定する。方法A の USB 電流から、電池電流 ≈ (USB電流 − 78) / 0.75 で換算した値
inline int estimateLoadMa(const LoadInput& in) {
    if (in.dimmed) return kDimmedLoadMa;
    int ma = kActiveBaseLoadMa;
    constexpr int levels = int(sizeof(kScreenBrightnessLoadMa) / sizeof(kScreenBrightnessLoadMa[0]));
    if (in.screenBrightness < levels) ma += kScreenBrightnessLoadMa[in.screenBrightness];
    return ma;
}

} // namespace BatteryModel
