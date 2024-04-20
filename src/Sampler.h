#ifndef _SAMPLER_H_
#define _SAMPLER_H_

#include <M5Unified.h>
#include <driver/i2s.h>
#include <ml_reverb.h>

// #define USE_HEADPHONE

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
#define DATA_SIZE 1024

#define SAMPLE_BUFFER_SIZE 64
#define SAMPLE_RATE 48000

#define MAX_SOUND 12 // 最大同時発音数

class MidiSampler
{
public:
    enum SampleAdsr
    {
        attack,
        decay,
        sustain,
        release,
    };
    struct Sample
    {
        const int16_t *sample;
        uint32_t length;
        uint8_t root;
        uint32_t loopStart;
        uint32_t loopEnd;

        bool adsrEnabled;
        float attack;
        float decay;
        float sustain;
        float release;
    };
    struct SamplePlayer
    {
        SamplePlayer(struct Sample *sample, uint8_t noteNo, float volume)
            : sample{sample}, noteNo{noteNo}, volume{volume}, createdAt{micros()} {}
        SamplePlayer() : sample{nullptr}, noteNo{60}, volume{1.0f}, playing{false}, createdAt{micros()} {}
        struct Sample *sample;
        uint8_t noteNo;
        float pitchBend = 0;
        float volume;
        unsigned long createdAt = 0;
        uint32_t pos = 0;
        float pos_f = 0.0f;
        bool playing = true;
        bool released = false;
        float adsrGain = 0.0f;
        enum SampleAdsr adsrState = SampleAdsr::attack;
    };

    float masterVolume = 0.5f;

    void SendNoteOn(uint8_t noteNo, uint8_t velocity, uint8_t channnel);
    void SendNoteOff(uint8_t noteNo,  uint8_t velocity, uint8_t channnel);
    void HandleMidiMessage(uint8_t *message);
    void begin();
    void terminate();

private:
    static constexpr uint32_t AUDIO_LOOP_INTERVAL = (uint32_t)(SAMPLE_BUFFER_SIZE * 1000000 / SAMPLE_RATE); // micro seconds
    static const int16_t piano_sample[32000];
    static struct Sample piano;

    float revBuffer[REV_BUFF_SIZE];
    SamplePlayer players[MAX_SOUND] = {SamplePlayer()};
    unsigned long nextAudioLoop = 0;
    uint32_t audioProcessTime = 0; // プロファイリング用 一回のオーディオ処理にかかる時間

    inline float PitchFromNoteNo(float noteNo, float root);
    inline void UpdateAdsr(SamplePlayer *player);
    void AudioLoop();
    static void StartAudioLoop(void* _this);
};

extern MidiSampler Sampler;

#endif