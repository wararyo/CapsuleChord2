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
    struct Sample pianoSample = Sample{
        piano_data, 24000, 60,
        21608, 21975,
        true, 1.0f, 0.998000f, 0.1f, 0.985000f};
    Timbre piano = Timbre({{&pianoSample, 0, 127, 0, 127}});
    Sampler sampler;

    AudioOutput audioOutput = AudioOutput::headphone;

    unsigned long nextAudioLoop = 0;
    uint32_t audioProcessTime = 0; // プロファイリング用 一回のオーディオ処理にかかる時間
    TaskHandle_t audioLoopHandler = nullptr;

    void AudioLoop();
    static void StartAudioLoop(void* _this);
};

#endif