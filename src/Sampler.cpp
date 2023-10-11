#include "Sampler.h"
#include <M5Unified.h>

struct MidiSampler::Sample MidiSampler::piano = MidiSampler::Sample{
    piano_sample,
    32000,
    60,
    24120,
    24288,
    true,
    1.0f,
    0.998887f,
    0.1f,
    0.988885f};

inline float MidiSampler::PitchFromNoteNo(float noteNo, float root)
{
    float delta = noteNo - root;
    float f = ((pow(2.0f, delta / 12.0f)));
    return f;
}

inline void MidiSampler::UpdateAdsr(MidiSampler::SamplePlayer *player)
{
    Sample *sample = player->sample;
    if (player->released)
        player->adsrState = release;

    switch (player->adsrState)
    {
    case attack:
        player->adsrGain += sample->attack;
        if (player->adsrGain >= 1.0f)
        {
            player->adsrGain = 1.0f;
            player->adsrState = decay;
        }
        break;
    case decay:
        player->adsrGain = (player->adsrGain - sample->sustain) * sample->decay + sample->sustain;
        if ((player->adsrGain - sample->sustain) < 0.01f)
        {
            player->adsrState = sustain;
            player->adsrGain = sample->sustain;
        }
        break;
    case sustain:
        break;
    case release:
        player->adsrGain *= sample->release;
        if (player->adsrGain < 0.01f)
        {
            player->adsrGain = 0;
            player->playing = false;
        }
        break;
    }
}

void MidiSampler::SendNoteOn(uint8_t noteNo, uint8_t velocity, uint8_t channnel)
{
    uint8_t oldestPlayerId = 0;
    for (uint8_t i = 0; i < MAX_SOUND; i++)
    {
        if (players[i].playing == false)
        {
            players[i] = MidiSampler::SamplePlayer(&MidiSampler::piano, noteNo, velocity / 127.0f);
            return;
        }
        else
        {
            if (players[i].createdAt < players[oldestPlayerId].createdAt)
                oldestPlayerId = i;
        }
    }
    // 全てのPlayerが再生中だった時には、最も昔に発音されたPlayerを停止する
    players[oldestPlayerId] = MidiSampler::SamplePlayer(&MidiSampler::piano, noteNo, velocity / 127.0f);
}
void MidiSampler::SendNoteOff(uint8_t noteNo, uint8_t velocity, uint8_t channnel)
{
    for (uint8_t i = 0; i < MAX_SOUND; i++)
    {
        if (players[i].playing == true && players[i].noteNo == noteNo)
        {
            players[i].released = true;
        }
    }
}

void MidiSampler::AudioLoop()
{
    while (true)
    {
        float data[SAMPLE_BUFFER_SIZE] = {0.0f};

        unsigned long startTime = micros();

        // 波形を生成
        for (uint8_t i = 0; i < MAX_SOUND; i++)
        {
            SamplePlayer *player = &players[i];
            if (player->playing == false)
                continue;
            Sample *sample = player->sample;
            if (sample->adsrEnabled)
                UpdateAdsr(player);
            if (player->playing == false)
                continue;

            float pitch = PitchFromNoteNo(player->noteNo, player->sample->root);

            for (int n = 0; n < SAMPLE_BUFFER_SIZE; n++)
            {
                if (player->pos >= sample->length)
                {
                    player->playing = false;
                    break;
                }
                else
                {
                    // 波形を読み込む
                    float val = sample->sample[player->pos];
                    if (sample->adsrEnabled)
                        val *= player->adsrGain;
                    val *= player->volume;
                    data[n] += val;

                    // 次のサンプルへ移動
                    int32_t pitch_u = pitch;
                    player->pos_f += pitch - pitch_u;
                    player->pos += pitch_u;
                    int posI = player->pos_f;
                    player->pos += posI;
                    player->pos_f -= posI;

                    // ループポイントが設定されている場合はループする
                    if (sample->adsrEnabled && player->pos >= sample->loopEnd)
                        player->pos -= (sample->loopEnd - sample->loopStart);
                }
            }
        }

        Reverb_Process(data, SAMPLE_BUFFER_SIZE);

        int16_t dataI[SAMPLE_BUFFER_SIZE];
        for (uint8_t i = 0; i < SAMPLE_BUFFER_SIZE; i++)
        {
            dataI[i] = int16_t(data[i] * masterVolume);
        }

        unsigned long endTime = micros();
        audioProcessTime = endTime - startTime;

        static size_t bytes_written = 0;
        i2s_write(Speak_I2S_NUMBER, (const unsigned char *)dataI, 2 * SAMPLE_BUFFER_SIZE, &bytes_written, portMAX_DELAY);
    }
}

void MidiSampler::StartAudioLoop(void *_this)
{
    static_cast<MidiSampler *>(_this)->AudioLoop();
}

// 動作確認用機能のため、CH1のみに対応
void MidiSampler::HandleMidiMessage(uint8_t *message)
{
    if (message[0] == 0x90)
    {
        SendNoteOn(message[1], message[2], 1);
    }
    else if (message[0] == 0x80)
    {
        SendNoteOff(message[1], message[2], 1);
    }
}

void MidiSampler::begin()
{
    esp_err_t err = ESP_OK;

    i2s_driver_uninstall(Speak_I2S_NUMBER);

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
    };
    i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    i2s_config.use_apll = false;
    i2s_config.tx_desc_auto_clear = true;

    err += i2s_driver_install(Speak_I2S_NUMBER, &i2s_config, 0, NULL);

    i2s_pin_config_t tx_pin_config;
    tx_pin_config.bck_io_num = CONFIG_I2S_BCK_PIN;
    tx_pin_config.ws_io_num = CONFIG_I2S_LRCK_PIN;
    tx_pin_config.data_out_num = CONFIG_I2S_DATA_PIN;
    tx_pin_config.data_in_num = CONFIG_I2S_DATA_IN_PIN;
    tx_pin_config.mck_io_num = GPIO_NUM_0;
    err += i2s_set_pin(Speak_I2S_NUMBER, &tx_pin_config);

    if (err == ESP_OK)
    {
        Serial.printf("i2s ok");
    }
    else
    {
        Serial.printf("i2s ng");
    }

    i2s_zero_dma_buffer(Speak_I2S_NUMBER);

    delay(100);

    size_t bytes_written = 0;
    i2s_write(Speak_I2S_NUMBER, (const unsigned char *)piano_sample, 64000, &bytes_written, portMAX_DELAY);
    Serial.printf("bytes_written: %d ", bytes_written);
    
    delay(100);

    Reverb_Setup(revBuffer);
    Reverb_SetLevel(0, 0.2f);

    // Core0でタスク起動
    xTaskCreateUniversal(
        StartAudioLoop,
        "audioLoop",
        8192,
        this,
        1,
        NULL,
        0);
    // ウォッチドッグ停止
    disableCore0WDT();
}

void MidiSampler::terminate()
{
}

MidiSampler Sampler;