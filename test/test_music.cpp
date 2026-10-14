#include <unity.h>
#include "../src/Chord.h"
#include "../src/Scale.h"
#include "../src/Modifier.h"
#include "../src/CustomKeySettings.h"
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
    std::vector<uint8_t> expected = {48, 50, 55}; // C3, D3, G3
    TestUtil::assertVectorEquals(expected, c.toMidiNoteNumbers(), "Csus2");
}

void test_chord_toMidiNoteNumbers_Caug(void) {
    Chord c(Chord::C, Chord::Aug, 0, 4);
    std::vector<uint8_t> expected = {48, 52, 56}; // C3, E3, G#3
    TestUtil::assertVectorEquals(expected, c.toMidiNoteNumbers(), "Caug");
}

void test_chord_toMidiNoteNumbers_Cdim(void) {
    Chord c(Chord::C, Chord::Dimish, 0, 4);
    std::vector<uint8_t> expected = {48, 51, 54}; // C3, Eb3, Gb3
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
// Scale: getDiatonicDegree() テスト
// ====================

void test_scale_getDiatonicDegree_major_V7(void) {
    Scale s(0); // C Major
    s.currentScale = s.getAvailableScales()[0].get();
    DegreeChord d = s.getDiatonicDegree(4, true); // V7
    TEST_ASSERT_EQUAL(DegreeChord::V, d.root);
    TEST_ASSERT_EQUAL(Chord::Seventh, d.option);
}

void test_scale_getDiatonicDegree_minor_i(void) {
    Scale s(0); // C Minor
    s.currentScale = s.getAvailableScales()[1].get();
    DegreeChord d = s.getDiatonicDegree(0, false); // i
    TEST_ASSERT_EQUAL(DegreeChord::I, d.root);
    TEST_ASSERT_EQUAL(Chord::Minor, d.option);
}

// getDiatonicDegree → degreeToChord の結果が getDiatonic と一致する
void test_scale_getDiatonicDegree_matches_getDiatonic(void) {
    for (int scaleIndex = 0; scaleIndex < 2; scaleIndex++) {
        Scale s(2); // D
        s.currentScale = s.getAvailableScales()[scaleIndex].get();
        for (uint8_t number = 0; number <= 6; number++) {
            for (int seventh = 0; seventh <= 1; seventh++) {
                Chord expected = s.getDiatonic(number, seventh != 0);
                Chord actual = s.degreeToChord(s.getDiatonicDegree(number, seventh != 0));
                TEST_ASSERT_EQUAL(expected.root, actual.root);
                TEST_ASSERT_EQUAL(expected.option, actual.option);
                TEST_ASSERT_EQUAL(expected.inversion, actual.inversion);
            }
        }
    }
}

// ====================
// Scale: degreeToChord() の bass 持ち越しテスト
// ====================

void test_scale_degreeToChord_carriesBass(void) {
    Scale s(2); // D Major
    s.currentScale = s.getAvailableScales()[0].get();
    DegreeChord d(DegreeChord::I, 0);
    d.setBass(2); // 度数空間で全音上のbass
    Chord c = s.degreeToChord(d);
    TEST_ASSERT_EQUAL(Chord::D, c.root);
    TEST_ASSERT_EQUAL(Chord::E, c.bass); // key(2) + bass(2)
}

void test_scale_degreeToChord_defaultBassStaysDefault(void) {
    Scale s(2); // D Major
    s.currentScale = s.getAvailableScales()[0].get();
    Chord c = s.degreeToChord(DegreeChord(DegreeChord::I, 0));
    TEST_ASSERT_EQUAL(Chord::BASS_DEFAULT, c.bass);
}

// ====================
// Scale: realizeChord() テスト
// ====================

void test_scale_realizeChord_matches_degreeToChord_plus_calcInversion(void) {
    Scale s(7); // G Major
    s.currentScale = s.getAvailableScales()[0].get();
    DegreeChord d = s.getDiatonicDegree(4, true);

    Chord expected = s.degreeToChord(d);
    expected.calcInversion(60);

    Chord actual = s.realizeChord(d, 60);
    TEST_ASSERT_EQUAL(expected.root, actual.root);
    TEST_ASSERT_EQUAL(expected.option, actual.option);
    TEST_ASSERT_EQUAL(expected.inversion, actual.inversion);
    TEST_ASSERT_EQUAL(expected.octave, actual.octave);
}

// ====================
// Modifier: 度数空間適用とChord空間適用の等価性テスト
// ====================

// 従来のKeyMap処理（Chord空間で修飾→calcInversion）と、
// 新しい処理（DegreeChord空間で修飾→realizeChord）が同じ結果になることを確認する
static void assertDegreeSpaceModifierEquivalence(Scale& s, uint8_t number, bool seventh,
                                                 void (*chordMod)(Chord*),
                                                 void (*degreeMod)(DegreeChord*),
                                                 const char* label) {
    // 旧パス: Chord空間で修飾
    Chord expected = s.getDiatonic(number, seventh);
    chordMod(&expected);
    expected.calcInversion(60);

    // 新パス: 度数空間で修飾してから具現化
    DegreeChord d = s.getDiatonicDegree(number, seventh);
    degreeMod(&d);
    Chord actual = s.realizeChord(d, 60);

    TEST_ASSERT_EQUAL_MESSAGE(expected.root, actual.root, label);
    TEST_ASSERT_EQUAL_MESSAGE(expected.option, actual.option, label);
    TEST_ASSERT_EQUAL_MESSAGE(expected.inversion, actual.inversion, label);
    TEST_ASSERT_EQUAL_MESSAGE(expected.octave, actual.octave, label);
    TEST_ASSERT_EQUAL_MESSAGE(expected.bass, actual.bass, label);
}

void test_modifier_degreeSpace_thirdInvert(void) {
    Scale s(2); // D Major
    s.currentScale = s.getAvailableScales()[0].get();
    assertDegreeSpaceModifierEquivalence(s, 0, false, thirdInvert<Chord>, thirdInvert<DegreeChord>, "thirdInvert");
}

void test_modifier_degreeSpace_pitchUp_wrap(void) {
    Scale s(2); // D Major
    s.currentScale = s.getAvailableScales()[0].get();
    // number 6 (VII, 度数root=11) でpitchUpのラップを跨ぐケース
    assertDegreeSpaceModifierEquivalence(s, 6, false, pitchUp<Chord>, pitchUp<DegreeChord>, "pitchUp wrap");
}

void test_modifier_degreeSpace_pitchDown_wrap(void) {
    Scale s(2); // D Major
    s.currentScale = s.getAvailableScales()[0].get();
    // number 0 (I, 度数root=0) でpitchDownのラップを跨ぐケース
    assertDegreeSpaceModifierEquivalence(s, 0, false, pitchDown<Chord>, pitchDown<DegreeChord>, "pitchDown wrap");
}

void test_modifier_degreeSpace_blackAdder(void) {
    Scale s(9); // A Major (bassの持ち越し込みで一致すること)
    s.currentScale = s.getAvailableScales()[0].get();
    assertDegreeSpaceModifierEquivalence(s, 4, false, blackAdder<Chord>, blackAdder<DegreeChord>, "blackAdder");
}

void test_modifier_degreeSpace_combined(void) {
    Scale s(2); // D Major
    s.currentScale = s.getAvailableScales()[0].get();

    // 複合修飾: seventh + thirdInvert + ninth + pitchUp
    Chord expected = s.getDiatonic(4, true);
    thirdInvert(&expected);
    ninth(&expected);
    pitchUp(&expected);
    expected.calcInversion(60);

    DegreeChord d = s.getDiatonicDegree(4, true);
    thirdInvert(&d);
    ninth(&d);
    pitchUp(&d);
    Chord actual = s.realizeChord(d, 60);

    TEST_ASSERT_EQUAL(expected.root, actual.root);
    TEST_ASSERT_EQUAL(expected.option, actual.option);
    TEST_ASSERT_EQUAL(expected.inversion, actual.inversion);
    TEST_ASSERT_EQUAL(expected.octave, actual.octave);
    TEST_ASSERT_EQUAL(expected.bass, actual.bass);
}

// ====================
// Scale: getDiatonicNumber() テスト
// ====================

void test_scale_getDiatonicNumber_major(void) {
    Scale s(0); // C Major
    s.currentScale = s.getAvailableScales()[0].get();
    // ダイアトニック音: pitch = {0,2,4,5,7,9,11}
    TEST_ASSERT_EQUAL(0, s.getDiatonicNumber(0));
    TEST_ASSERT_EQUAL(1, s.getDiatonicNumber(2));
    TEST_ASSERT_EQUAL(4, s.getDiatonicNumber(7));
    TEST_ASSERT_EQUAL(6, s.getDiatonicNumber(11));
    // 非ダイアトニック音
    TEST_ASSERT_EQUAL(-1, s.getDiatonicNumber(1));
    TEST_ASSERT_EQUAL(-1, s.getDiatonicNumber(3));
    TEST_ASSERT_EQUAL(-1, s.getDiatonicNumber(10));
}

void test_scale_getDiatonicNumber_minor(void) {
    Scale s(0); // C Minor
    s.currentScale = s.getAvailableScales()[1].get();
    // ダイアトニック音: pitch = {0,2,3,5,7,8,10}
    TEST_ASSERT_EQUAL(2, s.getDiatonicNumber(3));
    TEST_ASSERT_EQUAL(6, s.getDiatonicNumber(10));
    // 非ダイアトニック音
    TEST_ASSERT_EQUAL(-1, s.getDiatonicNumber(4));
    TEST_ASSERT_EQUAL(-1, s.getDiatonicNumber(11));
}

// ====================
// CustomKeyAssignment: applySeventhType() テスト
// ====================

void test_applySeventhType_addToTriad(void) {
    DegreeChord d(DegreeChord::IISharp, 0); // bIIIトライアド
    applySeventhType(d, CustomKeyAssignment::SeventhType::Seventh);
    TEST_ASSERT_EQUAL(Chord::Seventh, d.option);
}

void test_applySeventhType_minorTriadBecomesM7chord(void) {
    // マイナートライアド + b7 = m7（3度の質は保存コード由来）
    DegreeChord d(DegreeChord::II, Chord::Minor);
    applySeventhType(d, CustomKeyAssignment::SeventhType::Seventh);
    TEST_ASSERT_EQUAL(Chord::Minor | Chord::Seventh, d.option);
}

void test_applySeventhType_replacesExistingSeventh(void) {
    DegreeChord d(DegreeChord::I, Chord::MajorSeventh);
    applySeventhType(d, CustomKeyAssignment::SeventhType::Seventh);
    TEST_ASSERT_EQUAL(Chord::Seventh, d.option);

    applySeventhType(d, CustomKeyAssignment::SeventhType::MajorSeventh);
    TEST_ASSERT_EQUAL(Chord::MajorSeventh, d.option);
}

// ====================
// CustomKeyAssignment: シリアライズテスト
// ====================

void test_customKeyAssignment_serialize_roundtrip(void) {
    CustomKeyAssignment original(
        DegreeChord(DegreeChord::VISharp, Chord::MajorSeventh),
        CustomKeyAssignment::SeventhType::MajorSeventh);

    OutputArchive oa;
    oa("customKey", original);
    std::string json = oa.toJSON();

    InputArchive ia;
    ia.fromJSON(json.c_str());
    CustomKeyAssignment restored;
    ia("customKey", restored);

    TEST_ASSERT_TRUE(original == restored);
}

void test_customKeyAssignment_deserialize_invalidSeventhType_fallsBack(void) {
    // 範囲外の値（旧DiatonicSeventh=2など）はデフォルトにフォールバック
    InputArchive ia;
    ia.fromJSON("{\"customKey\":{\"chord\":{\"Root\":3,\"Option\":0},\"seventhType\":2}}");
    CustomKeyAssignment restored;
    ia("customKey", restored);

    TEST_ASSERT_EQUAL(3, restored.chord.root);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(CustomKeyAssignment::SeventhType::Seventh),
                      static_cast<uint8_t>(restored.seventhType));
}

// ====================
// 不正・不完全な保存データへの耐性テスト
// ====================

// chordが欠けていてもネストスタックが壊れず、後続の設定を正しく読める
void test_customKeyAssignment_deserialize_missingChord_keepsNestStack(void) {
    InputArchive ia;
    ia.fromJSON("{\"customKey1\":{\"seventhType\":1},\"after\":5}");
    CustomKeyAssignment restored;
    ia("customKey1", restored);

    // chordはデフォルト値のまま、seventhTypeは正しい階層から読まれる
    TEST_ASSERT_EQUAL(DegreeChord::IISharp, restored.chord.root);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(CustomKeyAssignment::SeventhType::MajorSeventh),
                      static_cast<uint8_t>(restored.seventhType));

    // 後続の項目がルート階層から読める（スタックが壊れていない）
    int after = 0;
    ia("after", after);
    TEST_ASSERT_EQUAL(5, after);
}

void test_customKeyAssignment_deserialize_emptyChord_keepsNestStack(void) {
    InputArchive ia;
    ia.fromJSON("{\"customKey1\":{\"chord\":{},\"seventhType\":1},\"after\":5}");
    CustomKeyAssignment restored;
    ia("customKey1", restored);

    TEST_ASSERT_EQUAL(static_cast<uint8_t>(CustomKeyAssignment::SeventhType::MajorSeventh),
                      static_cast<uint8_t>(restored.seventhType));
    int after = 0;
    ia("after", after);
    TEST_ASSERT_EQUAL(5, after);
}

void test_scale_deserialize_missingNest_keepsNestStack(void) {
    InputArchive ia;
    ia.fromJSON("{\"after\":5}");
    Scale s(2);
    ia("scale", s);
    TEST_ASSERT_EQUAL(2, s.key);  // 現在値を保持

    int after = 0;
    ia("after", after);
    TEST_ASSERT_EQUAL(5, after);
}

void test_inputArchive_popNest_neverPopsRoot(void) {
    InputArchive ia;
    ia.fromJSON("{\"after\":5}");
    ia.popNest();  // 対応するpushNestなし
    int after = 0;
    ia("after", after);
    TEST_ASSERT_EQUAL(5, after);
}

// 範囲外のBass/Rootは採用せずデフォルト値（現在値）を保持する
void test_degreeChord_deserialize_invalidBass_fallsBack(void) {
    const char* jsons[] = {
        "{\"chord\":{\"Root\":3,\"Option\":0,\"Bass\":12}}",
        "{\"chord\":{\"Root\":3,\"Option\":0,\"Bass\":-2}}",
    };
    for (const char* json : jsons) {
        InputArchive ia;
        ia.fromJSON(json);
        DegreeChord restored;
        ia("chord", restored);
        TEST_ASSERT_EQUAL_MESSAGE(3, restored.root, json);
        TEST_ASSERT_EQUAL_MESSAGE(DegreeChord::BASS_DEFAULT, restored.bass, json);
    }
}

void test_degreeChord_deserialize_invalidRoot_fallsBack(void) {
    const char* jsons[] = {
        "{\"chord\":{\"Root\":12,\"Option\":0}}",
        "{\"chord\":{\"Root\":-1,\"Option\":0}}",
    };
    for (const char* json : jsons) {
        InputArchive ia;
        ia.fromJSON(json);
        DegreeChord restored(DegreeChord::V, 0);
        ia("chord", restored);
        TEST_ASSERT_EQUAL_MESSAGE(DegreeChord::V, restored.root, json);
    }
}

void test_chord_deserialize_invalidRoot_fallsBack(void) {
    InputArchive ia;
    ia.fromJSON("{\"chord\":{\"Root\":200,\"Option\":0}}");
    Chord restored(Chord::G, 0);
    ia("chord", restored);
    TEST_ASSERT_EQUAL(Chord::G, restored.root);
}

void test_scale_deserialize_invalidKey_fallsBack(void) {
    InputArchive ia;
    ia.fromJSON("{\"scale\":{\"Key\":12,\"Scale\":\"Minor\"}}");
    Scale s(2);
    ia("scale", s);
    TEST_ASSERT_EQUAL(2, s.key);
    TEST_ASSERT_EQUAL_STRING("Minor", s.currentScale->name().c_str());
}

// ====================
// DegreeChord: bassのシリアライズテスト
// ====================

void test_degreeChord_serialize_bass_roundtrip(void) {
    DegreeChord original(DegreeChord::V, Chord::Aug);
    original.setBass(9);

    OutputArchive oa;
    oa("chord", original);
    std::string json = oa.toJSON();

    InputArchive ia;
    ia.fromJSON(json.c_str());
    DegreeChord restored;
    ia("chord", restored);

    TEST_ASSERT_EQUAL(original.root, restored.root);
    TEST_ASSERT_EQUAL(original.option, restored.option);
    TEST_ASSERT_EQUAL(original.bass, restored.bass);
}

void test_degreeChord_deserialize_missingBass_staysDefault(void) {
    // 旧形式（Bassキーなし）のJSONでもクラッシュせずデフォルト値になる
    InputArchive ia;
    ia.fromJSON("{\"chord\":{\"Root\":3,\"Option\":0}}");
    DegreeChord restored;
    ia("chord", restored);

    TEST_ASSERT_EQUAL(3, restored.root);
    TEST_ASSERT_EQUAL(0, restored.option);
    TEST_ASSERT_EQUAL(DegreeChord::BASS_DEFAULT, restored.bass);
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

    // getDiatonicDegree() テスト
    RUN_TEST(test_scale_getDiatonicDegree_major_V7);
    RUN_TEST(test_scale_getDiatonicDegree_minor_i);
    RUN_TEST(test_scale_getDiatonicDegree_matches_getDiatonic);

    // degreeToChord() の bass 持ち越しテスト
    RUN_TEST(test_scale_degreeToChord_carriesBass);
    RUN_TEST(test_scale_degreeToChord_defaultBassStaysDefault);

    // realizeChord() テスト
    RUN_TEST(test_scale_realizeChord_matches_degreeToChord_plus_calcInversion);

    // Modifier の度数空間適用テスト
    RUN_TEST(test_modifier_degreeSpace_thirdInvert);
    RUN_TEST(test_modifier_degreeSpace_pitchUp_wrap);
    RUN_TEST(test_modifier_degreeSpace_pitchDown_wrap);
    RUN_TEST(test_modifier_degreeSpace_blackAdder);
    RUN_TEST(test_modifier_degreeSpace_combined);

    // getDiatonicNumber() テスト
    RUN_TEST(test_scale_getDiatonicNumber_major);
    RUN_TEST(test_scale_getDiatonicNumber_minor);

    // applySeventhType() テスト
    RUN_TEST(test_applySeventhType_addToTriad);
    RUN_TEST(test_applySeventhType_minorTriadBecomesM7chord);
    RUN_TEST(test_applySeventhType_replacesExistingSeventh);

    // CustomKeyAssignment シリアライズテスト
    RUN_TEST(test_customKeyAssignment_serialize_roundtrip);
    RUN_TEST(test_customKeyAssignment_deserialize_invalidSeventhType_fallsBack);

    // 不正・不完全な保存データへの耐性テスト
    RUN_TEST(test_customKeyAssignment_deserialize_missingChord_keepsNestStack);
    RUN_TEST(test_customKeyAssignment_deserialize_emptyChord_keepsNestStack);
    RUN_TEST(test_scale_deserialize_missingNest_keepsNestStack);
    RUN_TEST(test_inputArchive_popNest_neverPopsRoot);
    RUN_TEST(test_degreeChord_deserialize_invalidBass_fallsBack);
    RUN_TEST(test_degreeChord_deserialize_invalidRoot_fallsBack);
    RUN_TEST(test_chord_deserialize_invalidRoot_fallsBack);
    RUN_TEST(test_scale_deserialize_invalidKey_fallsBack);

    // DegreeChord: bassのシリアライズテスト
    RUN_TEST(test_degreeChord_serialize_bass_roundtrip);
    RUN_TEST(test_degreeChord_deserialize_missingBass_staysDefault);
}

// ====================
// メインテスト実行
// ====================

#ifdef ARDUINO
void setup() {
    delay(2000);
    UNITY_BEGIN();
    runAllTests();
    UNITY_END();
}

void loop() {}
#else
int main(int argc, char **argv) {
    UNITY_BEGIN();
    runAllTests();
    return UNITY_END();
}
#endif
