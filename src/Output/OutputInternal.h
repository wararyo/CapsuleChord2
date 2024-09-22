#ifndef _SAMPLER_H_
#define _SAMPLER_H_

#include <M5Unified.h>
#include <driver/i2s.h>
#include <Sampler.h>

#define PIN_I2S_BCK_SPK GPIO_NUM_34
#define PIN_I2S_BCK_HP GPIO_NUM_6
#define PIN_I2S_LRCK_SPK GPIO_NUM_33
#define PIN_I2S_LRCK_HP GPIO_NUM_0
#define PIN_I2S_DATA GPIO_NUM_13
#define PIN_I2S_DATA_IN GPIO_NUM_14

#define I2S_NUM_SPK I2S_NUM_1
#define I2S_NUM_HP I2S_NUM_0

#define PIN_EN_HP GPIO_NUM_9

#define MODE_MIC 0
#define MODE_SPK 1

using capsule::sampler::Sampler;
using capsule::sampler::Sample;
using capsule::sampler::Timbre;

extern const int16_t piano_data[24000];
extern const int16_t aguitar_data[96000];
extern const int16_t bass_data[24000];
extern const int16_t epiano_data[124800];
extern const int16_t kick_data[12000];
extern const int16_t rimknock_data[10000];
extern const int16_t snare_data[12000];
extern const int16_t hihat_data[3200];
extern const int16_t crash_data[38879];
extern const int16_t metronome_tick_data[8000];
extern const int16_t metronome_tick_bar_data[8000];

// 内蔵音源
class OutputInternal
{
public:
    enum AudioOutput
    {
        headphone = 0,
        speaker
    };

    float masterVolume = 0.25f;

    void begin(AudioOutput output);
    void NoteOn(uint8_t noteNo, uint8_t velocity, uint8_t channel);
    void NoteOff(uint8_t noteNo, uint8_t velocity, uint8_t channel);
    void PitchBend(int16_t pitchBend, uint8_t channel);
    void terminate();

private:
    // ピアノティンバー
    struct Sample pianoSample = Sample{
        piano_data, 24000, 60,
        21608, 21975,
        true, 1.0f, 0.998000f, 0.1f, 0.985000f};
    Timbre piano = Timbre({{&pianoSample, 0, 127, 0, 127}});
    // アコギティンバー
    struct Sample aguitarSample = Sample{
        aguitar_data, 96000, 60,
        83347, 83714,
        true, 1.0f, 0.9985f, 0.03f, 0.97f};
    Timbre aguitar = Timbre({{&aguitarSample, 0, 127, 0, 127}});
    // ベースティンバー
    struct Sample bassSample = Sample{
        bass_data, 24000, 36,
        21714, 22448,
        true, 1.0f, 0.999000f, 0.25f, 0.970000f};
    Timbre bass = Timbre({{&bassSample, 0, 127, 0, 127}});
    // エレピティンバー
    struct Sample epianoSample = Sample{
        epiano_data, 124800, 60,
        120048, 120415,
        true, 1.0f, 0.98f, 0.5f, 0.95f};
    Timbre epiano = Timbre({{&epianoSample, 0, 127, 0, 127}});
    // ドラムティンバー
    struct Sample kickSample = Sample{
        kick_data, 11000, 36,
        0, 0,
        false, 0, 0, 0, 0};
    struct Sample rimknockSample = Sample{
        rimknock_data, 9000, 37,
        0, 0,
        false, 0, 0, 0, 0};
    struct Sample snareSample = Sample{
        snare_data, 11000, 38,
        0, 0,
        false, 0, 0, 0, 0};
    struct Sample hihatSample = Sample{
        hihat_data, 2400, 42,
        0, 0,
        false, 0, 0, 0, 0};
    struct Sample crashSample = Sample{
        crash_data, 38000, 49,
        0, 0,
        false, 0, 0, 0, 0};
    Timbre drumset = Timbre({
        {&kickSample, 36, 36, 0, 127},
        {&rimknockSample, 37, 37, 0, 127},
        {&snareSample, 38, 38, 0, 127},
        {&hihatSample, 42, 42, 0, 127},
        {&crashSample, 49, 49, 0, 127}
    });
    // システム音ティンバー
    struct Sample metronomeTickSample = Sample{
        metronome_tick_data, 7000, 24,
        0, 0,
        false, 1.0f, 1.0f, 1.0f, 1.0f};
    struct Sample metronomeTickBarSample = Sample{
        metronome_tick_bar_data, 7000, 25,
        0, 0,
        false, 1.0f, 1.0f, 1.0f, 1.0f};
    Timbre system = Timbre({
        {&metronomeTickSample, 24, 24, 0, 127},
        {&metronomeTickBarSample, 25, 25, 0, 127}
    });
        
    Sampler sampler;

    AudioOutput audioOutput = AudioOutput::headphone;

    unsigned long nextAudioLoop = 0;
    uint32_t audioProcessTime = 0; // プロファイリング用 一回のオーディオ処理にかかる時間
    TaskHandle_t audioLoopHandler = nullptr;

    void AudioLoop();
    static void StartAudioLoop(void* _this);
};

#endif