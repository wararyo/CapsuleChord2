#include <unity.h>
// 電池残量の推定（src/Battery）のテスト。test_music.cpp の runAllTests() から呼ばれる
#include "../src/Battery/BatteryModel.h"
#include "../src/Battery/BatteryEstimator.h"

using namespace BatteryModel;

// ====================
// カーブ
// ====================

void test_curve_points_and_clamp(void) {
    TEST_ASSERT_EQUAL(100, percentFromCurveMv(4200));
    TEST_ASSERT_EQUAL(100, percentFromCurveMv(4080));
    TEST_ASSERT_EQUAL(42, percentFromCurveMv(3800));
    TEST_ASSERT_EQUAL(0, percentFromCurveMv(3050));
    TEST_ASSERT_EQUAL(0, percentFromCurveMv(2900));
}

void test_curve_interpolates(void) {
    // 3800 (42) と 3850 (55) の中間
    TEST_ASSERT_EQUAL(49, percentFromCurveMv(3825));
}

void test_curve_is_monotonic(void) {
    int prev = percentFromCurveMv(2900);
    for (int mv = 2900; mv <= 4200; mv += 5) {
        int p = percentFromCurveMv(mv);
        TEST_ASSERT_TRUE(p >= prev);
        prev = p;
    }
}

// ====================
// 負荷
// ====================

void test_load_dimmed_ignores_brightness(void) {
    LoadInput in;
    in.dimmed = true;
    in.screenBrightness = 0;
    TEST_ASSERT_EQUAL(kDimmedLoadMa, estimateLoadMa(in));
}

void test_load_screen_brightness(void) {
    LoadInput in;
    in.screenBrightness = 1;
    TEST_ASSERT_EQUAL(kActiveBaseLoadMa, estimateLoadMa(in));
    in.screenBrightness = 0;
    TEST_ASSERT_EQUAL(kActiveBaseLoadMa + 73, estimateLoadMa(in));
    in.screenBrightness = 2;
    TEST_ASSERT_EQUAL(kActiveBaseLoadMa - 43, estimateLoadMa(in));
    // 範囲外は「普通」と同じ扱い
    in.screenBrightness = 9;
    TEST_ASSERT_EQUAL(kActiveBaseLoadMa, estimateLoadMa(in));
}

void test_compensation_reference_load_is_identity(void) {
    TEST_ASSERT_EQUAL(3800, compensateMv(3800, kCurveLoadMa));
}

void test_compensation_heavier_load_adds_voltage(void) {
    // 通常時（約145 mA）は基準（52 mA）より 93 mA 多い → 0.7 Ω で約 +65 mV
    TEST_ASSERT_EQUAL(3865, compensateMv(3800, 145));
}

// ====================
// 推定
// ====================

static BatteryEstimator::Reading onBattery(uint32_t nowMs, int mv, bool dimmed = false) {
    BatteryEstimator::Reading r;
    r.nowMs = nowMs;
    r.mv = mv;
    r.load.dimmed = dimmed;
    r.load.screenBrightness = 1;
    return r;
}

void test_estimator_first_reading(void) {
    BatteryEstimator e;
    // 減光中は補正なし
    TEST_ASSERT_EQUAL(42, e.update(onBattery(0, 3800, true)));
}

void test_estimator_active_load_is_compensated(void) {
    BatteryEstimator e;
    // 通常時に 3800 mV なら、基準の負荷では 3865 mV 相当
    TEST_ASSERT_EQUAL(percentFromCurveMv(3865), e.update(onBattery(0, 3800)));
}

void test_estimator_does_not_rise_on_battery(void) {
    BatteryEstimator e;
    int first = e.update(onBattery(0, 3800, true));
    TEST_ASSERT_EQUAL(first, e.update(onBattery(10000, 3850, true)));
}

void test_estimator_holds_after_load_change(void) {
    BatteryEstimator e;
    int dimmed = e.update(onBattery(0, 3800, true));
    // 減光から復帰した直後は、電圧が下がっても前の値を保つ
    TEST_ASSERT_EQUAL(dimmed, e.update(onBattery(10000, 3700)));
    TEST_ASSERT_EQUAL(dimmed, e.update(onBattery(10000 + BatteryEstimator::kSettleMs - 1, 3700)));
    // 落ち着いたら新しい負荷で読み直す
    TEST_ASSERT_EQUAL(percentFromCurveMv(compensateMv(3700, kActiveBaseLoadMa)),
                      e.update(onBattery(10000 + BatteryEstimator::kSettleMs, 3700)));
}

void test_estimator_averages_readings(void) {
    BatteryEstimator e;
    e.update(onBattery(0, 3800, true));
    // 1回だけ大きく下がっても、平均でならされる
    int p = e.update(onBattery(10000, 3700, true));
    TEST_ASSERT_EQUAL(percentFromCurveMv(3750), p);
}

void test_estimator_full_when_charger_idle(void) {
    BatteryEstimator e;
    BatteryEstimator::Reading r = onBattery(0, 4150);
    r.powered = true;
    r.charging = false;
    TEST_ASSERT_EQUAL(100, e.update(r));
}

void test_estimator_charging_caps_at_99(void) {
    BatteryEstimator e;
    BatteryEstimator::Reading r = onBattery(0, 4300);
    r.powered = true;
    r.charging = true;
    TEST_ASSERT_EQUAL(99, e.update(r));
}

void test_estimator_charging_can_rise(void) {
    BatteryEstimator e;
    BatteryEstimator::Reading r = onBattery(0, 3800);
    r.powered = true;
    r.charging = true;
    int first = e.update(r);
    for (int i = 1; i <= BatteryEstimator::kAverageCount; ++i) {
        r.nowMs = i * 10000;
        r.mv = 3900;
        e.update(r);
    }
    TEST_ASSERT_TRUE(e.getShown() > first);
}

void test_estimator_unplug_resets_floor(void) {
    BatteryEstimator e;
    e.update(onBattery(0, 3500));
    BatteryEstimator::Reading r = onBattery(10000, 4150);
    r.powered = true;
    // 給電が変わった直後は保持
    TEST_ASSERT_EQUAL(e.getShown(), e.update(r));
    r.nowMs = 10000 + BatteryEstimator::kSettleMs;
    TEST_ASSERT_EQUAL(100, e.update(r));
    // 抜いた後は、電池駆動中の値が前の電池駆動中の値より高くてもよい
    uint32_t unplug = r.nowMs + 10000;
    e.update(onBattery(unplug, 4000));
    int p = e.update(onBattery(unplug + BatteryEstimator::kSettleMs, 4000));
    TEST_ASSERT_EQUAL(percentFromCurveMv(compensateMv(4000, kActiveBaseLoadMa)), p);
}

void test_estimator_failed_read_keeps_value(void) {
    BatteryEstimator e;
    TEST_ASSERT_EQUAL(-1, e.update(onBattery(0, 0)));
    int p = e.update(onBattery(10000, 3800, true));
    TEST_ASSERT_EQUAL(p, e.update(onBattery(20000, 0, true)));
}

void runBatteryTests() {
    RUN_TEST(test_curve_points_and_clamp);
    RUN_TEST(test_curve_interpolates);
    RUN_TEST(test_curve_is_monotonic);
    RUN_TEST(test_load_dimmed_ignores_brightness);
    RUN_TEST(test_load_screen_brightness);
    RUN_TEST(test_compensation_reference_load_is_identity);
    RUN_TEST(test_compensation_heavier_load_adds_voltage);
    RUN_TEST(test_estimator_first_reading);
    RUN_TEST(test_estimator_active_load_is_compensated);
    RUN_TEST(test_estimator_does_not_rise_on_battery);
    RUN_TEST(test_estimator_holds_after_load_change);
    RUN_TEST(test_estimator_averages_readings);
    RUN_TEST(test_estimator_full_when_charger_idle);
    RUN_TEST(test_estimator_charging_caps_at_99);
    RUN_TEST(test_estimator_charging_can_rise);
    RUN_TEST(test_estimator_unplug_resets_floor);
    RUN_TEST(test_estimator_failed_read_keeps_value);
}
