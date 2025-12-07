#ifndef _SAMPLER_H_
#define _SAMPLER_H_

#include <M5Unified.h>
#include <driver/i2s.h>
#include <Sampler.h>
#include "IMidiOutput.h"

#define PIN_I2S_BCK_SPK GPIO_NUM_34
#define PIN_I2S_BCK_HP GPIO_NUM_6
#define PIN_I2S_LRCK_SPK GPIO_NUM_33
#define PIN_I2S_LRCK_HP GPIO_NUM_0
#define PIN_I2S_DATA GPIO_NUM_13
#define PIN_I2S_DATA_IN GPIO_NUM_14

#define I2S_NUM_SPK I2S_NUM_1
#define I2S_NUM_HP I2S_NUM_0

#define PIN_EN_HP GPIO_NUM_9
#define PIN_HP_DETECT GPIO_NUM_18

#define MODE_MIC 0
#define MODE_SPK 1

using capsule::sampler::Sampler;
using capsule::sampler::Sample;
using capsule::sampler::Timbre;

// メトロノーム音（システム音）は静的データとして保持
extern const int16_t metronome_tick_data[8000];
extern const int16_t metronome_tick_bar_data[8000];

// 内蔵音源
class OutputInternal : public IMidiOutput
{
public:
    enum AudioOutput
    {
        headphone = 0,
        speaker
    };

    float masterVolume = 0.3f;

    void NoteOn(uint8_t noteNo, uint8_t velocity, uint8_t channel);
    void NoteOff(uint8_t noteNo, uint8_t velocity, uint8_t channel);
    void PitchBend(int16_t pitchBend, uint8_t channel);
    void loadPiano();
    void loadAGuitar();
    void loadEPiano();
    void loadSuperSaw();

    // IMidiOutput interface implementation
    void begin() override;
    void end() override;
    void update() override;
    void noteOn(uint8_t note, uint8_t velocity, uint8_t channel = 0) override {
        NoteOn(note, velocity, channel);
    }
    void noteOff(uint8_t note, uint8_t velocity, uint8_t channel = 0) override {
        NoteOff(note, velocity, channel);
    }
    void pitchBend(int16_t pitchBend, uint8_t channel = 0) override {
        PitchBend(pitchBend, channel);
    }
    bool isAvailable() const override { return true; }
    const char* getName() const override { return "Internal"; }

private:
    // LittleFSから動的に読み込んだティンバー
    std::shared_ptr<Timbre> piano = nullptr;
    std::shared_ptr<Timbre> aguitar = nullptr;
    std::shared_ptr<Timbre> bass = nullptr;
    std::shared_ptr<Timbre> epiano = nullptr;
    std::shared_ptr<Timbre> supersaw = nullptr;
    std::shared_ptr<Timbre> drumset = nullptr;

    // システム音ティンバー（静的データから初期化）
    std::shared_ptr<Sample> metronomeTickSample = std::make_shared<Sample>(
        metronome_tick_data, 7000, 24,
        0, 0,
        false, 1.0f, 1.0f, 1.0f, 1.0f);
    std::shared_ptr<Sample> metronomeTickBarSample = std::make_shared<Sample>(
        metronome_tick_bar_data, 7000, 25,
        0, 0,
        false, 1.0f, 1.0f, 1.0f, 1.0f);
    std::shared_ptr<Timbre> system = std::make_shared<Timbre>(std::vector<Timbre::MappedSample>{
        {metronomeTickSample, 24, 24, 0, 127},
        {metronomeTickBarSample, 25, 25, 0, 127}
    });

    std::shared_ptr<Sampler> sampler = Sampler::Create();

    // LittleFSからティンバーを読み込む
    bool loadTimbres();
    // 読み込んだティンバーを解放する
    void unloadTimbres();

    // I2Sとオーディオループを初期化する（出力先変更時に呼ばれる）
    void initAudioOutput(AudioOutput output);
    // オーディオループを停止する
    void stopAudioLoop();

    AudioOutput audioOutput = AudioOutput::headphone;
    bool isHeadphonePreviously = false;

    unsigned long nextAudioLoop = 0;
    uint32_t audioProcessTime = 0; // プロファイリング用 一回のオーディオ処理にかかる時間
    TaskHandle_t audioLoopHandler = nullptr;

    void AudioLoop();
    static void StartAudioLoop(void* _this);
};

#endif