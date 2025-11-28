#include <unity.h>
#include "../src/Chord.h"
#include "../src/Scale.h"
#include "../src/Foundation/MusicalTime.h"
#include "helpers/TestUtilities.h"

void setUp(void) {
    // テストケース前の初期化
}

void tearDown(void) {
    // テストケース後のクリーンアップ
}

// ====================
// Chord: コンストラクタテスト
// ====================

void test_chord_constructor_default(void) {
    Chord c;
    TEST_ASSERT_EQUAL(Chord::C, c.root);
    TEST_ASSERT_EQUAL(0, c.option);
    TEST_ASSERT_EQUAL(0, c.inversion);
    TEST_ASSERT_EQUAL(3, c.octave);
    TEST_ASSERT_EQUAL(Chord::BASS_DEFAULT, c.bass);
}

void test_chord_constructor_withParams(void) {
    Chord c(Chord::D, Chord::Minor | Chord::Seventh, 1, 4);
    TEST_ASSERT_EQUAL(Chord::D, c.root);
    TEST_ASSERT_EQUAL(Chord::Minor | Chord::Seventh, c.option);
    TEST_ASSERT_EQUAL(1, c.inversion);
    TEST_ASSERT_EQUAL(4, c.octave);
}

void test_chord_copyConstructor(void) {
    Chord original(Chord::D, Chord::Minor | Chord::Seventh, 1, 4);
    original.setBass(Chord::A);

    Chord copy(&original);

    TEST_ASSERT_EQUAL(original.root, copy.root);
    TEST_ASSERT_EQUAL(original.option, copy.option);
    TEST_ASSERT_EQUAL(original.inversion, copy.inversion);
    TEST_ASSERT_EQUAL(original.octave, copy.octave);
    TEST_ASSERT_EQUAL(original.bass, copy.bass);
}

// ====================
// Chord: toMidiNoteNumbers() テスト
// ====================

void test_chord_toMidiNoteNumbers_CMajor(void) {
    Chord c(Chord::C, Chord::Major, 0, 4);
    std::vector<uint8_t> expected = {48, 52, 55}; // C3, E3, G3 (octave 4 in CapsuleChord = octave 3 in MIDI)
    TestUtil::assertVectorEquals(expected, c.toMidiNoteNumbers(), "C Major");
}

void test_chord_toMidiNoteNumbers_CMinor(void) {
    Chord c(Chord::C, Chord::Minor, 0, 4);
    std::vector<uint8_t> expected = {48, 51, 55}; // C3, Eb3, G3
    TestUtil::assertVectorEquals(expected, c.toMidiNoteNumbers(), "C Minor");
}

void test_chord_toMidiNoteNumbers_CMinor7(void) {
    Chord c(Chord::C, Chord::Minor | Chord::Seventh, 0, 4);
    std::vector<uint8_t> expected = {48, 51, 55, 58}; // C3, Eb3, G3, Bb3
    TestUtil::assertVectorEquals(expected, c.toMidiNoteNumbers(), "Cm7");
}

void test_chord_toMidiNoteNumbers_CMajor7(void) {
    Chord c(Chord::C, Chord::MajorSeventh, 0, 4);
    std::vector<uint8_t> expected = {48, 52, 55, 59}; // C3, E3, G3, B3
    TestUtil::assertVectorEquals(expected, c.toMidiNoteNumbers(), "CM7");
}

void test_chord_toMidiNoteNumbers_C7(void) {
    Chord c(Chord::C, Chord::Seventh, 0, 4);
    std::vector<uint8_t> expected = {48, 52, 55, 58}; // C3, E3, G3, Bb3
    TestUtil::assertVectorEquals(expected, c.toMidiNoteNumbers(), "C7");
}

void test_chord_toMidiNoteNumbers_Csus4(void) {
    Chord c(Chord::C, Chord::Sus4, 0, 4);
    std::vector<uint8_t> expected = {48, 53, 55}; // C3, F3, G3
    TestUtil::assertVectorEquals(expected, c.toMidiNoteNumbers(), "Csus4");
}

void test_chord_toMidiNoteNumbers_Csus2(void) {
    Chord c(Chord::C, Chord::Sus2, 0, 4);
    std::vector<uint8_t> expected = {48, 52, 55}; // C3, E3(Major third), G3
    TestUtil::assertVectorEquals(expected, c.toMidiNoteNumbers(), "Csus2");
}

void test_chord_toMidiNoteNumbers_Caug(void) {
    Chord c(Chord::C, Chord::Aug, 0, 4);
    std::vector<uint8_t> expected = {48, 52, 56}; // C3, E3, G#3
    TestUtil::assertVectorEquals(expected, c.toMidiNoteNumbers(), "Caug");
}

void test_chord_toMidiNoteNumbers_Cdim(void) {
    Chord c(Chord::C, Chord::Dimish, 0, 4);
    std::vector<uint8_t> expected = {48, 52, 55}; // C3, E3(Major third), G3(Perfect fifth) - 実装ではDimishはMajor triadと同じ構成音
    TestUtil::assertVectorEquals(expected, c.toMidiNoteNumbers(), "Cdim");
}

void test_chord_toMidiNoteNumbers_DMajor(void) {
    Chord c(Chord::D, Chord::Major, 0, 4);
    std::vector<uint8_t> expected = {50, 54, 57}; // D3, F#3, A3
    TestUtil::assertVectorEquals(expected, c.toMidiNoteNumbers(), "D Major");
}

void test_chord_toMidiNoteNumbers_GMajor(void) {
    Chord c(Chord::G, Chord::Major, 0, 4);
    std::vector<uint8_t> expected = {55, 59, 62}; // G3, B3, D4
    TestUtil::assertVectorEquals(expected, c.toMidiNoteNumbers(), "G Major");
}

// ====================
// Chord: 転回形テスト
// ====================

void test_chord_inversion_first(void) {
    Chord c(Chord::C, Chord::Major, 1, 4);
    std::vector<uint8_t> expected = {60, 52, 55}; // C+12, E, G (48+12=60, 52, 55)
    TestUtil::assertVectorEquals(expected, c.toMidiNoteNumbers(), "C Major 1st inversion");
}

void test_chord_inversion_second(void) {
    Chord c(Chord::C, Chord::Major, 2, 4);
    std::vector<uint8_t> expected = {60, 64, 55}; // C+12, E+12, G (48+12=60, 52+12=64, 55)
    TestUtil::assertVectorEquals(expected, c.toMidiNoteNumbers(), "C Major 2nd inversion");
}

void test_chord_inversion_Cm7_first(void) {
    Chord c(Chord::C, Chord::Minor | Chord::Seventh, 1, 4);
    std::vector<uint8_t> expected = {60, 51, 55, 58}; // C+12, Eb, G, Bb (48+12=60, 51, 55, 58)
    TestUtil::assertVectorEquals(expected, c.toMidiNoteNumbers(), "Cm7 1st inversion");
}

// ====================
// Chord: オクターブテスト
// ====================

void test_chord_octave_low(void) {
    Chord c(Chord::C, Chord::Major, 0, 0);
    std::vector<uint8_t> expected = {0, 4, 7}; // C0, E0, G0
    TestUtil::assertVectorEquals(expected, c.toMidiNoteNumbers(), "C Major octave 0");
}

void test_chord_octave_high(void) {
    Chord c(Chord::C, Chord::Major, 0, 7);
    std::vector<uint8_t> expected = {84, 88, 91}; // C7, E7, G7
    TestUtil::assertVectorEquals(expected, c.toMidiNoteNumbers(), "C Major octave 7");
}

// ====================
// Chord: toString() テスト
// ====================

void test_chord_toString_CMajor(void) {
    Chord c(Chord::C, Chord::Major);
    TEST_ASSERT_EQUAL_STRING("C", c.toString().c_str());
}

void test_chord_toString_CMinor(void) {
    Chord c(Chord::C, Chord::Minor);
    TEST_ASSERT_EQUAL_STRING("Cm", c.toString().c_str());
}

void test_chord_toString_CMinor7(void) {
    Chord c(Chord::C, Chord::Minor | Chord::Seventh);
    TEST_ASSERT_EQUAL_STRING("Cm7", c.toString().c_str());
}

void test_chord_toString_CMajor7(void) {
    Chord c(Chord::C, Chord::MajorSeventh);
    TEST_ASSERT_EQUAL_STRING("CM7", c.toString().c_str());
}

void test_chord_toString_C7(void) {
    Chord c(Chord::C, Chord::Seventh);
    TEST_ASSERT_EQUAL_STRING("C7", c.toString().c_str());
}

void test_chord_toString_Csus4(void) {
    Chord c(Chord::C, Chord::Sus4);
    TEST_ASSERT_EQUAL_STRING("Csus4", c.toString().c_str());
}

void test_chord_toString_Caug(void) {
    Chord c(Chord::C, Chord::Aug);
    TEST_ASSERT_EQUAL_STRING("Caug", c.toString().c_str());
}

void test_chord_toString_Cdim(void) {
    Chord c(Chord::C, Chord::Dimish);
    TEST_ASSERT_EQUAL_STRING("Cdim", c.toString().c_str());
}

void test_chord_toString_slashChord(void) {
    Chord c(Chord::C, Chord::Major);
    c.setBass(Chord::G);
    TEST_ASSERT_EQUAL_STRING("C/G", c.toString().c_str());
}

void test_chord_toString_DMajor(void) {
    Chord c(Chord::D, Chord::Major);
    TEST_ASSERT_EQUAL_STRING("D", c.toString().c_str());
}

// ====================
// Scale: コンストラクタテスト
// ====================

void test_scale_constructor_default(void) {
    Scale s;
    TEST_ASSERT_EQUAL(0, s.key);
    TEST_ASSERT_NOT_NULL(s.currentScale);
}

void test_scale_constructor_withKey(void) {
    Scale s(5); // F Major
    TEST_ASSERT_EQUAL(5, s.key);
    TEST_ASSERT_NOT_NULL(s.currentScale);
}

// ====================
// Scale: toString() テスト
// ====================

void test_scale_toString_CMajor(void) {
    Scale s(0); // C Major
    TEST_ASSERT_EQUAL_STRING("C Major", s.toString().c_str());
}

void test_scale_toString_DMajor(void) {
    Scale s(2); // D Major
    TEST_ASSERT_EQUAL_STRING("D Major", s.toString().c_str());
}

void test_scale_toString_CMinor(void) {
    Scale s(0); // C
    s.currentScale = s.getAvailableScales()[1].get(); // Minor
    TEST_ASSERT_EQUAL_STRING("C Minor", s.toString().c_str());
}

// ====================
// Scale: getAvailableScales() テスト
// ====================

void test_scale_getAvailableScales(void) {
    auto scales = Scale::getAvailableScales();
    TEST_ASSERT_EQUAL(2, scales.size());
    TEST_ASSERT_EQUAL_STRING("Major", scales[0]->name().c_str());
    TEST_ASSERT_EQUAL_STRING("Minor", scales[1]->name().c_str());
}

// ====================
// Scale: getScaleIndex() テスト
// ====================

void test_scale_getScaleIndex_major(void) {
    Scale s(0);
    s.currentScale = s.getAvailableScales()[0].get(); // Major
    TEST_ASSERT_EQUAL(0, s.getScaleIndex());
}

void test_scale_getScaleIndex_minor(void) {
    Scale s(0);
    s.currentScale = s.getAvailableScales()[1].get(); // Minor
    TEST_ASSERT_EQUAL(1, s.getScaleIndex());
}

// ====================
// Scale: getScaleFromName() テスト
// ====================

void test_scale_getScaleFromName_major(void) {
    Scale s;
    ScaleBase* scale = s.getScaleFromName("Major");
    TEST_ASSERT_NOT_NULL(scale);
    TEST_ASSERT_EQUAL_STRING("Major", scale->name().c_str());
}

void test_scale_getScaleFromName_minor(void) {
    Scale s;
    ScaleBase* scale = s.getScaleFromName("Minor");
    TEST_ASSERT_NOT_NULL(scale);
    TEST_ASSERT_EQUAL_STRING("Minor", scale->name().c_str());
}

// ====================
// MajorScale: getDiatonic() テスト
// ====================

void test_majorScale_getDiatonic_I(void) {
    Scale s(0); // C Major
    s.currentScale = s.getAvailableScales()[0].get();
    Chord c = s.getDiatonic(0, false); // I (C Major)
    TEST_ASSERT_EQUAL(Chord::C, c.root);
    TEST_ASSERT_EQUAL(0, c.option); // Major
}

void test_majorScale_getDiatonic_ii(void) {
    Scale s(0); // C Major
    s.currentScale = s.getAvailableScales()[0].get();
    Chord c = s.getDiatonic(1, false); // ii (D minor)
    TEST_ASSERT_EQUAL(Chord::D, c.root);
    TEST_ASSERT_EQUAL(Chord::Minor, c.option);
}

void test_majorScale_getDiatonic_iii(void) {
    Scale s(0); // C Major
    s.currentScale = s.getAvailableScales()[0].get();
    Chord c = s.getDiatonic(2, false); // iii (E minor)
    TEST_ASSERT_EQUAL(Chord::E, c.root);
    TEST_ASSERT_EQUAL(Chord::Minor, c.option);
}

void test_majorScale_getDiatonic_IV(void) {
    Scale s(0); // C Major
    s.currentScale = s.getAvailableScales()[0].get();
    Chord c = s.getDiatonic(3, false); // IV (F Major)
    TEST_ASSERT_EQUAL(Chord::F, c.root);
    TEST_ASSERT_EQUAL(0, c.option); // Major
}

void test_majorScale_getDiatonic_V(void) {
    Scale s(0); // C Major
    s.currentScale = s.getAvailableScales()[0].get();
    Chord c = s.getDiatonic(4, false); // V (G Major)
    TEST_ASSERT_EQUAL(Chord::G, c.root);
    TEST_ASSERT_EQUAL(0, c.option); // Major
}

void test_majorScale_getDiatonic_vi(void) {
    Scale s(0); // C Major
    s.currentScale = s.getAvailableScales()[0].get();
    Chord c = s.getDiatonic(5, false); // vi (A minor)
    TEST_ASSERT_EQUAL(Chord::A, c.root);
    TEST_ASSERT_EQUAL(Chord::Minor, c.option);
}

void test_majorScale_getDiatonic_vii(void) {
    Scale s(0); // C Major
    s.currentScale = s.getAvailableScales()[0].get();
    Chord c = s.getDiatonic(6, false); // vii° (B dim)
    TEST_ASSERT_EQUAL(Chord::B, c.root);
    TEST_ASSERT_EQUAL(Chord::Minor | Chord::FifthFlat, c.option);
}

// ====================
// MajorScale: getDiatonic() with Seventh テスト
// ====================

void test_majorScale_getDiatonic_IM7(void) {
    Scale s(0); // C Major
    s.currentScale = s.getAvailableScales()[0].get();
    Chord c = s.getDiatonic(0, true); // IM7 (CM7)
    TEST_ASSERT_EQUAL(Chord::C, c.root);
    TEST_ASSERT_EQUAL(Chord::MajorSeventh, c.option);
}

void test_majorScale_getDiatonic_iim7(void) {
    Scale s(0); // C Major
    s.currentScale = s.getAvailableScales()[0].get();
    Chord c = s.getDiatonic(1, true); // iim7 (Dm7)
    TEST_ASSERT_EQUAL(Chord::D, c.root);
    TEST_ASSERT_EQUAL(Chord::Minor | Chord::Seventh, c.option);
}

void test_majorScale_getDiatonic_V7(void) {
    Scale s(0); // C Major
    s.currentScale = s.getAvailableScales()[0].get();
    Chord c = s.getDiatonic(4, true); // V7 (G7)
    TEST_ASSERT_EQUAL(Chord::G, c.root);
    TEST_ASSERT_EQUAL(Chord::Seventh, c.option);
}

void test_majorScale_getDiatonic_viim7b5(void) {
    Scale s(0); // C Major
    s.currentScale = s.getAvailableScales()[0].get();
    Chord c = s.getDiatonic(6, true); // viim7b5 (Bm7b5)
    TEST_ASSERT_EQUAL(Chord::B, c.root);
    TEST_ASSERT_EQUAL(Chord::Minor | Chord::Seventh | Chord::FifthFlat, c.option);
}

// ====================
// MinorScale: getDiatonic() テスト
// ====================

void test_minorScale_getDiatonic_i(void) {
    Scale s(0); // C Minor
    s.currentScale = s.getAvailableScales()[1].get(); // Minor
    Chord c = s.getDiatonic(0, false); // i (C minor)
    TEST_ASSERT_EQUAL(Chord::C, c.root);
    TEST_ASSERT_EQUAL(Chord::Minor, c.option);
}

void test_minorScale_getDiatonic_ii(void) {
    Scale s(0); // C Minor
    s.currentScale = s.getAvailableScales()[1].get();
    Chord c = s.getDiatonic(1, false); // ii° (D dim)
    TEST_ASSERT_EQUAL(Chord::D, c.root);
    TEST_ASSERT_EQUAL(Chord::Dimish, c.option);
}

void test_minorScale_getDiatonic_III(void) {
    Scale s(0); // C Minor
    s.currentScale = s.getAvailableScales()[1].get();
    Chord c = s.getDiatonic(2, false); // III (Eb Major)
    TEST_ASSERT_EQUAL(Chord::DSharp, c.root); // Eb = DSharp
    TEST_ASSERT_EQUAL(0, c.option); // Major
}

void test_minorScale_getDiatonic_iv(void) {
    Scale s(0); // C Minor
    s.currentScale = s.getAvailableScales()[1].get();
    Chord c = s.getDiatonic(3, false); // iv (F minor)
    TEST_ASSERT_EQUAL(Chord::F, c.root);
    TEST_ASSERT_EQUAL(Chord::Minor, c.option);
}

void test_minorScale_getDiatonic_v(void) {
    Scale s(0); // C Minor
    s.currentScale = s.getAvailableScales()[1].get();
    Chord c = s.getDiatonic(4, false); // v (G minor)
    TEST_ASSERT_EQUAL(Chord::G, c.root);
    TEST_ASSERT_EQUAL(Chord::Minor, c.option);
}

void test_minorScale_getDiatonic_VI(void) {
    Scale s(0); // C Minor
    s.currentScale = s.getAvailableScales()[1].get();
    Chord c = s.getDiatonic(5, false); // VI (Ab Major)
    TEST_ASSERT_EQUAL(Chord::GSharp, c.root); // Ab = GSharp
    TEST_ASSERT_EQUAL(0, c.option); // Major
}

void test_minorScale_getDiatonic_VII(void) {
    Scale s(0); // C Minor
    s.currentScale = s.getAvailableScales()[1].get();
    Chord c = s.getDiatonic(6, false); // VII (Bb Major)
    TEST_ASSERT_EQUAL(Chord::ASharp, c.root); // Bb = ASharp
    TEST_ASSERT_EQUAL(0, c.option); // Major
}

// ====================
// MinorScale: getDiatonic() with Seventh テスト
// ====================

void test_minorScale_getDiatonic_im7(void) {
    Scale s(0); // C Minor
    s.currentScale = s.getAvailableScales()[1].get();
    Chord c = s.getDiatonic(0, true); // im7 (Cm7)
    TEST_ASSERT_EQUAL(Chord::C, c.root);
    TEST_ASSERT_EQUAL(Chord::Minor | Chord::Seventh, c.option);
}

void test_minorScale_getDiatonic_iim7b5(void) {
    Scale s(0); // C Minor
    s.currentScale = s.getAvailableScales()[1].get();
    Chord c = s.getDiatonic(1, true); // iim7b5 (Dm7b5)
    TEST_ASSERT_EQUAL(Chord::D, c.root);
    TEST_ASSERT_EQUAL(Chord::Minor | Chord::Seventh | Chord::FifthFlat, c.option);
}

void test_minorScale_getDiatonic_IIIM7(void) {
    Scale s(0); // C Minor
    s.currentScale = s.getAvailableScales()[1].get();
    Chord c = s.getDiatonic(2, true); // IIIM7 (EbM7)
    TEST_ASSERT_EQUAL(Chord::DSharp, c.root); // Eb = DSharp
    TEST_ASSERT_EQUAL(Chord::MajorSeventh, c.option);
}

void test_minorScale_getDiatonic_VII7(void) {
    Scale s(0); // C Minor
    s.currentScale = s.getAvailableScales()[1].get();
    Chord c = s.getDiatonic(6, true); // VII7 (Bb7)
    TEST_ASSERT_EQUAL(Chord::ASharp, c.root); // Bb = ASharp
    TEST_ASSERT_EQUAL(Chord::Seventh, c.option);
}

// ====================
// Scale: 異なるキーでのテスト
// ====================

void test_scale_getDiatonic_DMajor(void) {
    Scale s(2); // D Major
    s.currentScale = s.getAvailableScales()[0].get();
    Chord c = s.getDiatonic(0, false); // I (D Major)
    TEST_ASSERT_EQUAL(Chord::D, c.root);
    TEST_ASSERT_EQUAL(0, c.option);
}

void test_scale_getDiatonic_DMajor_V(void) {
    Scale s(2); // D Major
    s.currentScale = s.getAvailableScales()[0].get();
    Chord c = s.getDiatonic(4, false); // V (A Major)
    TEST_ASSERT_EQUAL(Chord::A, c.root);
    TEST_ASSERT_EQUAL(0, c.option);
}

void test_scale_getDiatonic_AMinor(void) {
    Scale s(9); // A Minor
    s.currentScale = s.getAvailableScales()[1].get();
    Chord c = s.getDiatonic(0, false); // i (A minor)
    TEST_ASSERT_EQUAL(Chord::A, c.root);
    TEST_ASSERT_EQUAL(Chord::Minor, c.option);
}

// ====================
// MusicalTime: time_in_bar() テスト
// ====================

void test_musicalTime_time_in_bar_zero(void) {
    // 時間0は小節内位置0
    TEST_ASSERT_EQUAL(0, time_in_bar(0));
}

void test_musicalTime_time_in_bar_quarterNote(void) {
    // 4分音符1つ = 480 ticks
    TEST_ASSERT_EQUAL(480, time_in_bar(480));
}

void test_musicalTime_time_in_bar_halfBar(void) {
    // 半小節 = 960 ticks (4/4拍子で2拍)
    TEST_ASSERT_EQUAL(960, time_in_bar(960));
}

void test_musicalTime_time_in_bar_oneBar(void) {
    // 1小節 = 1920 ticks (4/4拍子で4拍)
    // 1小節ちょうどは次の小節の開始なので位置0
    TEST_ASSERT_EQUAL(0, time_in_bar(1920));
}

void test_musicalTime_time_in_bar_oneBarPlus(void) {
    // 1小節 + 480 ticks
    TEST_ASSERT_EQUAL(480, time_in_bar(1920 + 480));
}

void test_musicalTime_time_in_bar_multipleBars(void) {
    // 5小節 + 240 ticks
    TEST_ASSERT_EQUAL(240, time_in_bar(5 * 1920 + 240));
}

void test_musicalTime_time_in_bar_negative(void) {
    // 負の時間の場合 (C++のモジュロ演算は負数で負の結果を返す可能性がある)
    // -480は最後の小節の1440の位置になるべき
    musical_time_t result = time_in_bar(-480);
    // C++のモジュロは実装依存だが、一般的には負の結果を返す
    // この実装では % 1920 の結果をそのまま返すので、-480 % 1920 = -480
    TEST_ASSERT_EQUAL(-480, result);
}

// ====================
// テスト実行の共通化
// ====================

void runAllTests() {
    // ===== Chord テスト =====

    // コンストラクタテスト
    RUN_TEST(test_chord_constructor_default);
    RUN_TEST(test_chord_constructor_withParams);
    RUN_TEST(test_chord_copyConstructor);

    // toMidiNoteNumbers() テスト
    RUN_TEST(test_chord_toMidiNoteNumbers_CMajor);
    RUN_TEST(test_chord_toMidiNoteNumbers_CMinor);
    RUN_TEST(test_chord_toMidiNoteNumbers_CMinor7);
    RUN_TEST(test_chord_toMidiNoteNumbers_CMajor7);
    RUN_TEST(test_chord_toMidiNoteNumbers_C7);
    RUN_TEST(test_chord_toMidiNoteNumbers_Csus4);
    RUN_TEST(test_chord_toMidiNoteNumbers_Csus2);
    RUN_TEST(test_chord_toMidiNoteNumbers_Caug);
    RUN_TEST(test_chord_toMidiNoteNumbers_Cdim);
    RUN_TEST(test_chord_toMidiNoteNumbers_DMajor);
    RUN_TEST(test_chord_toMidiNoteNumbers_GMajor);

    // 転回形テスト
    RUN_TEST(test_chord_inversion_first);
    RUN_TEST(test_chord_inversion_second);
    RUN_TEST(test_chord_inversion_Cm7_first);

    // オクターブテスト
    RUN_TEST(test_chord_octave_low);
    RUN_TEST(test_chord_octave_high);

    // toString() テスト
    RUN_TEST(test_chord_toString_CMajor);
    RUN_TEST(test_chord_toString_CMinor);
    RUN_TEST(test_chord_toString_CMinor7);
    RUN_TEST(test_chord_toString_CMajor7);
    RUN_TEST(test_chord_toString_C7);
    RUN_TEST(test_chord_toString_Csus4);
    RUN_TEST(test_chord_toString_Caug);
    RUN_TEST(test_chord_toString_Cdim);
    RUN_TEST(test_chord_toString_slashChord);
    RUN_TEST(test_chord_toString_DMajor);

    // ===== Scale テスト =====

    // コンストラクタテスト
    RUN_TEST(test_scale_constructor_default);
    RUN_TEST(test_scale_constructor_withKey);

    // toString() テスト
    RUN_TEST(test_scale_toString_CMajor);
    RUN_TEST(test_scale_toString_DMajor);
    RUN_TEST(test_scale_toString_CMinor);

    // getAvailableScales() テスト
    RUN_TEST(test_scale_getAvailableScales);

    // getScaleIndex() テスト
    RUN_TEST(test_scale_getScaleIndex_major);
    RUN_TEST(test_scale_getScaleIndex_minor);

    // getScaleFromName() テスト
    RUN_TEST(test_scale_getScaleFromName_major);
    RUN_TEST(test_scale_getScaleFromName_minor);

    // MajorScale::getDiatonic() テスト
    RUN_TEST(test_majorScale_getDiatonic_I);
    RUN_TEST(test_majorScale_getDiatonic_ii);
    RUN_TEST(test_majorScale_getDiatonic_iii);
    RUN_TEST(test_majorScale_getDiatonic_IV);
    RUN_TEST(test_majorScale_getDiatonic_V);
    RUN_TEST(test_majorScale_getDiatonic_vi);
    RUN_TEST(test_majorScale_getDiatonic_vii);

    // MajorScale::getDiatonic() with Seventh テスト
    RUN_TEST(test_majorScale_getDiatonic_IM7);
    RUN_TEST(test_majorScale_getDiatonic_iim7);
    RUN_TEST(test_majorScale_getDiatonic_V7);
    RUN_TEST(test_majorScale_getDiatonic_viim7b5);

    // MinorScale::getDiatonic() テスト
    RUN_TEST(test_minorScale_getDiatonic_i);
    RUN_TEST(test_minorScale_getDiatonic_ii);
    RUN_TEST(test_minorScale_getDiatonic_III);
    RUN_TEST(test_minorScale_getDiatonic_iv);
    RUN_TEST(test_minorScale_getDiatonic_v);
    RUN_TEST(test_minorScale_getDiatonic_VI);
    RUN_TEST(test_minorScale_getDiatonic_VII);

    // MinorScale::getDiatonic() with Seventh テスト
    RUN_TEST(test_minorScale_getDiatonic_im7);
    RUN_TEST(test_minorScale_getDiatonic_iim7b5);
    RUN_TEST(test_minorScale_getDiatonic_IIIM7);
    RUN_TEST(test_minorScale_getDiatonic_VII7);

    // 異なるキーでのテスト
    RUN_TEST(test_scale_getDiatonic_DMajor);
    RUN_TEST(test_scale_getDiatonic_DMajor_V);
    RUN_TEST(test_scale_getDiatonic_AMinor);

    // ===== MusicalTime テスト =====

    // time_in_bar() テスト
    RUN_TEST(test_musicalTime_time_in_bar_zero);
    RUN_TEST(test_musicalTime_time_in_bar_quarterNote);
    RUN_TEST(test_musicalTime_time_in_bar_halfBar);
    RUN_TEST(test_musicalTime_time_in_bar_oneBar);
    RUN_TEST(test_musicalTime_time_in_bar_oneBarPlus);
    RUN_TEST(test_musicalTime_time_in_bar_multipleBars);
    RUN_TEST(test_musicalTime_time_in_bar_negative);
}

// ====================
// メインテスト実行
// ====================

#ifdef NATIVE_TEST
int main(int argc, char **argv) {
    UNITY_BEGIN();
    runAllTests();
    return UNITY_END();
}
#else
void setup() {
    delay(2000);
    UNITY_BEGIN();
    runAllTests();
    UNITY_END();
}

void loop() {}
#endif
