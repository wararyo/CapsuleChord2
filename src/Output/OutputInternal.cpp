#include "OutputInternal.h"
#include "TimbreLoader.h"
#include <esp_log.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>

static const char* LOG_TAG = "OutputInternal";

// ESP-IDF GPIO helpers
static inline void esp_pinMode(gpio_num_t pin, gpio_mode_t mode) {
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << pin);
    io_conf.mode = mode;
    io_conf.pull_up_en = (mode == GPIO_MODE_INPUT) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
}

static inline void esp_digitalWrite(gpio_num_t pin, int level) {
    gpio_set_level(pin, level);
}

static inline int esp_digitalRead(gpio_num_t pin) {
    return gpio_get_level(pin);
}

static inline void esp_delay(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void OutputInternal::NoteOn(uint8_t noteNo, uint8_t velocity, uint8_t channel)
{
    sampler->NoteOn(noteNo, velocity, channel);
}

void OutputInternal::NoteOff(uint8_t noteNo, uint8_t velocity, uint8_t channel)
{
    sampler->NoteOff(noteNo, velocity, channel);
}

void OutputInternal::PitchBend(int16_t pitchBend, uint8_t channel)
{
    sampler->PitchBend(pitchBend, channel);
}

void OutputInternal::AudioLoop()
{
    int16_t output[4][SAMPLE_BUFFER_SIZE] = {0};
    uint8_t buf_idx = 0;
    while (true)
    {
        unsigned long startTime = capsule::sampler::micros();

        sampler->Process(output[buf_idx]);
        
        unsigned long endTime = capsule::sampler::micros();
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

bool OutputInternal::loadTimbres()
{
    // TODO: Phase 5でesp_littlefsを使用した実装に置き換える
    // 現在はTimbreLoaderがスタブ実装のため、ティンバーは読み込まれない
    ESP_LOGI(LOG_TAG, "Loading timbres...");

    // TimbreLoaderを使用してティンバーを読み込む（現在はスタブ）
    aguitar = Loader.loadTimbre("/aguitar/aguitar.json");
    if (aguitar) ESP_LOGI(LOG_TAG, "  aguitar loaded");

    bass = Loader.loadTimbre("/ebass/ebass.json");
    if (bass) ESP_LOGI(LOG_TAG, "  bass loaded");

    epiano = Loader.loadTimbre("/epiano/epiano.json");
    if (epiano) ESP_LOGI(LOG_TAG, "  epiano loaded");

    piano = Loader.loadTimbre("/piano/piano.json");
    if (piano) ESP_LOGI(LOG_TAG, "  piano loaded");

    supersaw = Loader.loadTimbre("/supersaw/supersaw.json");
    if (supersaw) ESP_LOGI(LOG_TAG, "  supersaw loaded");

    drumset = Loader.loadTimbre("/popdrumkit/popdrumkit.json");
    if (drumset) ESP_LOGI(LOG_TAG, "  drumset loaded");

    ESP_LOGI(LOG_TAG, "Timbres loading complete");
    return true;
}

void OutputInternal::unloadTimbres()
{
    // shared_ptrをnullptrにすることでメモリを解放
    // ティンバー内のSampleが持つデータはheap_caps_mallocで確保されているため
    // Sampleのデストラクタで解放される必要がある
    piano = nullptr;
    aguitar = nullptr;
    bass = nullptr;
    epiano = nullptr;
    supersaw = nullptr;
    drumset = nullptr;

    ESP_LOGI(LOG_TAG, "Timbres unloaded");
}

void OutputInternal::stopAudioLoop()
{
    if (audioLoopHandler != nullptr)
    {
        vTaskDelete(audioLoopHandler);
        audioLoopHandler = nullptr;
    }
    // WDTを再有効化（audioLoopタスク終了後）
    esp_task_wdt_add(xTaskGetIdleTaskHandleForCPU(0));
}

void OutputInternal::initAudioOutput(AudioOutput output)
{
    // 既にAudioLoopが起動している場合は終了
    stopAudioLoop();

    // I2Sの初期化
    if (audioOutput == AudioOutput::headphone) i2s_driver_uninstall(I2S_NUM_HP);
    else i2s_driver_uninstall(I2S_NUM_SPK);

    audioOutput = output;

    esp_err_t err = ESP_OK;

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
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

    if (err == ESP_OK) ESP_LOGI(LOG_TAG, "i2s ok");
    else ESP_LOGE(LOG_TAG, "i2s ng");

    if (output == AudioOutput::headphone) {
        esp_pinMode(PIN_EN_HP, GPIO_MODE_OUTPUT);
        esp_digitalWrite(PIN_EN_HP, 1);
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
        // aw88298_write_reg(0x0C, 0x1064); // volume setting (-6db)
        aw88298_write_reg(0x0C, 0x1664); // volume setting (-9db)
        // aw88298_write_reg(0x0C, 0x2064); // volume setting (-12db)
        esp_pinMode(PIN_EN_HP, GPIO_MODE_OUTPUT);
        esp_digitalWrite(PIN_EN_HP, 0);
    }

    i2s_zero_dma_buffer(output == AudioOutput::headphone ? I2S_NUM_HP : I2S_NUM_SPK);
    esp_delay(100);

    // Core0でタスク起動
    xTaskCreatePinnedToCore(
        StartAudioLoop,
        "audioLoop",
        8192,
        this,
        1,
        &audioLoopHandler,
        0);
    // ウォッチドッグからCore0アイドルタスクを除外（オーディオ処理を妨げないため）
    esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(0));
}

void OutputInternal::begin()
{
    // イヤホン端子スイッチのピン設定
    esp_pinMode(PIN_HP_DETECT, GPIO_MODE_INPUT);

    // ティンバーを読み込む
    if (!loadTimbres()) {
        ESP_LOGW(LOG_TAG, "Failed to load timbres");
    }

    // ティンバーをサンプラーに設定
    if (aguitar) sampler->SetTimbre(0x0, aguitar);
    if (bass) sampler->SetTimbre(0x1, bass);
    if (epiano) sampler->SetTimbre(0x3, epiano);
    if (drumset) sampler->SetTimbre(0x9, drumset);
    sampler->SetTimbre(0xF, system);

    // サンプラーの設定
    sampler->masterVolume = masterVolume;

    // 現在のヘッドフォン接続状態を取得してI2S初期化
    isHeadphonePreviously = esp_digitalRead(PIN_HP_DETECT);
    initAudioOutput(isHeadphonePreviously ? AudioOutput::headphone : AudioOutput::speaker);
}

void OutputInternal::end()
{
    // AudioLoopを停止
    stopAudioLoop();

    // ティンバーを解放してメモリを節約
    unloadTimbres();
}

void OutputInternal::update()
{
    // ヘッドフォン抜き差しを検出
    bool isHeadphone = esp_digitalRead(PIN_HP_DETECT);
    if (isHeadphonePreviously != isHeadphone) {
        initAudioOutput(isHeadphone ? AudioOutput::headphone : AudioOutput::speaker);
        isHeadphonePreviously = isHeadphone;
    }
}

void OutputInternal::loadPiano()
{
    sampler->SetTimbre(0x0, piano);
}

void OutputInternal::loadAGuitar()
{
    sampler->SetTimbre(0x0, aguitar);
}

void OutputInternal::loadEPiano()
{
    sampler->SetTimbre(0x0, epiano);
}

void OutputInternal::loadSuperSaw()
{
    sampler->SetTimbre(0x0, supersaw);
}
