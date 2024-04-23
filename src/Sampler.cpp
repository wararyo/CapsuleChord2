#include "Sampler.h"
#include <M5Unified.h>

struct MidiSampler::Sample MidiSampler::piano = MidiSampler::Sample{
    piano_sample,
    32000,
    60,
    26253,
    26436,
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
    float goal;
    if (player->released)
        player->adsrState = release;

    switch (player->adsrState)
    {
    case attack:
        player->adsrGain += sample->attack * player->volume;
        if (player->adsrGain >= player->volume)
        {
            player->adsrGain = player->volume;
            player->adsrState = decay;
        }
        break;
    case decay:
        goal = sample->sustain * player->volume;
        player->adsrGain = (player->adsrGain - goal) * sample->decay + goal;
        if ((player->adsrGain - sample->sustain) < 0.01f)
        {
            player->adsrState = sustain;
            player->adsrGain = goal;
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

            if (sample->adsrEnabled) // adsrEnabledによる場合分けが多いので、まずadsrEnabledで分ける
            {
                for (int n = 0; n < SAMPLE_BUFFER_SIZE; n++)
                {
                    // 波形を読み込む&線形補完
                    float val = (sample->sample[player->pos+1] * player->pos_f) + (sample->sample[player->pos] * (1.0f-player->pos_f));
                    val *= player->adsrGain;
                    data[n] += val;

                    // 次のサンプルへ移動
                    int32_t pitch_u = pitch;
                    player->pos_f += pitch - pitch_u;
                    player->pos += pitch_u;
                    if(player->pos_f >= 1.0f) {
                        player->pos++;
                        player->pos_f--;
                    }

                    // ループポイントが設定されている場合はループする
                    while (player->pos >= sample->loopEnd)
                        player->pos -= (sample->loopEnd - sample->loopStart);
                }
            }
            else
            {
                for (int n = 0; n < SAMPLE_BUFFER_SIZE; n++)
                {
                    if (player->pos >= sample->length)
                    {
                        player->playing = false;
                        break;
                    }
                    // 波形を読み込む
                    float val = sample->sample[player->pos];
                    val *= player->volume;
                    data[n] += val;

                    // 次のサンプルへ移動
                    int32_t pitch_u = pitch;
                    player->pos_f += pitch - pitch_u;
                    player->pos += pitch_u;
                    int posI = player->pos_f;
                    player->pos += posI;
                    player->pos_f -= posI;
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

        i2s_write(audioOutput == AudioOutput::headphone ? I2S_NUM_HP : I2S_NUM_SPK, (const unsigned char *)dataI, 2 * SAMPLE_BUFFER_SIZE, &bytes_written, portMAX_DELAY);
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

static constexpr uint8_t aw88298_i2c_addr = 0x36;

void aw88298_write_reg(uint8_t reg, uint16_t value)
{
  value = __builtin_bswap16(value);
  M5.In_I2C.writeRegister(aw88298_i2c_addr, reg, (const uint8_t *)&value, 2, 400000);
}

void MidiSampler::begin(AudioOutput output)
{
    if(audioLoopHandler != nullptr) vTaskDelete(audioLoopHandler);
    audioLoopHandler = nullptr;

    if (audioOutput == AudioOutput::headphone) i2s_driver_uninstall(I2S_NUM_HP);
    else i2s_driver_uninstall(I2S_NUM_SPK);

    audioOutput = output;

    esp_err_t err = ESP_OK;

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

    if (output == AudioOutput::headphone) err += i2s_driver_install(I2S_NUM_HP, &i2s_config, 0, NULL);
    else err += i2s_driver_install(I2S_NUM_SPK, &i2s_config, 0, NULL);

    if (output == AudioOutput::headphone) {
        i2s_pin_config_t tx_pin_config;
        tx_pin_config.bck_io_num = PIN_I2S_BCK_HP;
        tx_pin_config.ws_io_num = PIN_I2S_LRCK_HP;
        tx_pin_config.data_out_num = PIN_I2S_DATA;
        tx_pin_config.data_in_num = PIN_I2S_DATA_IN;
        tx_pin_config.mck_io_num = GPIO_NUM_0;
        err += i2s_set_pin(I2S_NUM_HP, &tx_pin_config);
    } else {
        i2s_pin_config_t tx_pin_config;
        tx_pin_config.bck_io_num = PIN_I2S_BCK_SPK;
        tx_pin_config.ws_io_num = PIN_I2S_LRCK_SPK;
        tx_pin_config.data_out_num = PIN_I2S_DATA;
        tx_pin_config.data_in_num = PIN_I2S_DATA_IN;
        tx_pin_config.mck_io_num = GPIO_NUM_0;
        err += i2s_set_pin(I2S_NUM_SPK, &tx_pin_config);
    }

    if (err == ESP_OK) Serial.printf("i2s ok");
    else Serial.printf("i2s ng");

    if (output == AudioOutput::headphone) {
        pinMode(PIN_EN_HP, OUTPUT);
        digitalWrite(PIN_EN_HP, 1);
    } else {
        M5.In_I2C.bitOn(aw88298_i2c_addr, 0x02, 0b00000100, 400000);
        /// サンプリングレートに応じてAW88298のレジスタの設定値を変える;
        static constexpr uint8_t rate_tbl[] = {4, 5, 6, 8, 10, 11, 15, 20, 22, 44};
        size_t reg0x06_value = 0;
        size_t rate = (48000 + 1102) / 2205;
        while (rate > rate_tbl[reg0x06_value] && ++reg0x06_value < sizeof(rate_tbl)) {}

        reg0x06_value |= 0x14C0;         // I2SBCK=0 (BCK mode 16*2)
        aw88298_write_reg(0x61, 0x0673); // boost mode disabled
        aw88298_write_reg(0x04, 0x4040); // I2SEN=1 AMPPD=0 PWDN=0
        aw88298_write_reg(0x05, 0x0008); // RMSE=0 HAGCE=0 HDCCE=0 HMUTE=0
        aw88298_write_reg(0x06, reg0x06_value);
        // aw88298_write_reg(0x0C, 0x0064);  // volume setting (full volume)
        aw88298_write_reg(0x0C, 0x1064); // volume setting (-6db)
        // aw88298_write_reg(0x0C, 0x2064); // volume setting (-12db)
        pinMode(PIN_EN_HP, OUTPUT);
        digitalWrite(PIN_EN_HP, 0);
    }

    i2s_zero_dma_buffer(output == AudioOutput::headphone ? I2S_NUM_HP : I2S_NUM_SPK);
    delay(100);

    // size_t bytes_written = 0;
    // i2s_write(output == AudioOutput::headphone ? I2S_NUM_HP : I2S_NUM_SPK, (const unsigned char *)piano_sample, 64000, &bytes_written, portMAX_DELAY);
    // Serial.printf("bytes_written: %d ", bytes_written);
    // delay(100);

    Reverb_Setup(revBuffer);
    Reverb_SetLevel(0, 0.2f);

    // Core0でタスク起動
    xTaskCreatePinnedToCore(
        StartAudioLoop,
        "audioLoop",
        8192,
        this,
        1,
        &audioLoopHandler,
        0);
    // ウォッチドッグ停止
    disableCore0WDT();
}

void MidiSampler::terminate()
{
}

MidiSampler Sampler;