// クラッシュ（パニック）発生時にオーディオ出力を可能な限り黙らせるためのフック。
//
// 背景:
//   内蔵スピーカー(AW88298)・イヤホン出力ともに、クラッシュ後もアンプが通電したまま
//   I2Sのクロック停止やDMAバッファのループにより意図しない音が長時間鳴り続けることがある。
//   リンカの --wrap オプションを使ってesp_panic_handlerをフックすることで、パニック処理の前に出力をミュートさせる。
//   パニックハンドラは「割り込み禁止・FreeRTOS停止・1コアのみ」という特殊な実行コンテキストで動くため、
//   ドライバAPI(i2s_zero_dma_buffer / M5.In_I2C 等)は内部ロックを取るので呼べない。
//   したがって、ここではGPIO/ROM操作のみでミュートを実現する。
//
// 対策内容:
//   1. I2Sデータライン(GPIO13)をペリフェラルから切り離してGPIO LOW固定にする。
//      → 両アンプへ無音が流れ、意図しない持続音を防ぐ。
//   2. イヤホンアンプのenable(GPIO9)を0にする。
//      → イヤホン出力を完全に無効化する。
//
// 注: スピーカー(AW88298)のHMUTEはI2C経由のため、パニック中の確実な実行は難しい。
//     ここではデータラインLOW化で代替している(将来的に生I2Cでの追加も検討)。

#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include "soc/gpio_sig_map.h"

// ピン定義は src/Output/OutputInternal.h と一致させること
#define PANIC_PIN_EN_HP    9   // PIN_EN_HP: イヤホンアンプのenable
#define PANIC_PIN_I2S_DATA 13  // PIN_I2S_DATA: スピーカー/イヤホン共有のI2Sデータ出力

// リンカの --wrap=esp_panic_handler により、本来のハンドラはこの名前で参照できる
extern void __real_esp_panic_handler(void *info);

void __wrap_esp_panic_handler(void *info)
{
    // 1) I2Sデータラインをペリフェラルから切り離してGPIO出力(LOW)に固定する。
    //    esp_rom_gpio_* はROM上の関数でロックを取らないため、パニック中でも安全に呼べる。
    esp_rom_gpio_connect_out_signal(PANIC_PIN_I2S_DATA, SIG_GPIO_OUT_IDX, false, false);
    gpio_set_level((gpio_num_t)PANIC_PIN_I2S_DATA, 0);

    // 2) イヤホンアンプのenableを落とす。
    gpio_set_level((gpio_num_t)PANIC_PIN_EN_HP, 0);

    // 本来のパニック処理(コアダンプ書き込み→リブート等)へ引き継ぐ。
    __real_esp_panic_handler(info);
}
