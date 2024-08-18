#include "OutputInternal.h"

void OutputInternal::NoteOn(uint8_t noteNo, uint8_t velocity, uint8_t channel)
{
    sampler.NoteOn(noteNo, velocity, channel);
}

void OutputInternal::NoteOff(uint8_t noteNo, uint8_t velocity, uint8_t channel)
{
    sampler.NoteOff(noteNo, velocity, channel);
}

void OutputInternal::PitchBend(int16_t pitchBend, uint8_t channel)
{
    sampler.PitchBend(pitchBend, channel);
}

void OutputInternal::AudioLoop()
{
    int16_t output[4][SAMPLE_BUFFER_SIZE] = {0};
    uint8_t buf_idx = 0;
    while (true)
    {
        unsigned long startTime = micros();

        sampler.Process(output[buf_idx]);
        
        unsigned long endTime = micros();
        audioProcessTime = endTime - startTime;

        static size_t bytes_written = 0;
        i2s_write(audioOutput == AudioOutput::headphone ? I2S_NUM_HP : I2S_NUM_SPK, (const unsigned char *)output[buf_idx], 2 * SAMPLE_BUFFER_SIZE, &bytes_written, portMAX_DELAY);

        buf_idx = (buf_idx + 1) & 3;
    }
}

void OutputInternal::StartAudioLoop(void *_this)
{
    static_cast<OutputInternal *>(_this)->AudioLoop();
}

static constexpr uint8_t aw88298_i2c_addr = 0x36;

void aw88298_write_reg(uint8_t reg, uint16_t value)
{
  value = __builtin_bswap16(value);
  M5.In_I2C.writeRegister(aw88298_i2c_addr, reg, (const uint8_t *)&value, 2, 400000);
}

void OutputInternal::begin(AudioOutput output)
{
    // 既にAudioLoopが起動している場合は終了
    if(audioLoopHandler != nullptr) vTaskDelete(audioLoopHandler);
    audioLoopHandler = nullptr;

    // ティンバーをセット
    sampler.SetTimbre(0, &piano);

    // I2Sの初期化
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

void OutputInternal::terminate()
{
    // AudioLoopを停止
    if (audioLoopHandler != nullptr)
    {
        vTaskDelete(audioLoopHandler);
        audioLoopHandler = nullptr;
    }
    enableCore0WDT();
}
