/* 自編練習，不是官方考題。
 * 此檔在 RP2350 的 CPU 執行；echo.pio 在 PIO 狀態機執行。
 * 電腦只從 USB 看 printf。輸入 21 是下方程式固定的值，不是 ROS 訊息。
 */
#include <stdint.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "echo.pio.h"  // CMake 透過 pioasm 產生，不用手寫這份 header。

int main(void) {
    stdio_init_all();  // 配合 CMake 的 USB stdio，讓 printf 從 USB 送給電腦。
    sleep_ms(2000);    // 留時間給 USB 初始化；輸出會每秒重複，不怕漏第一筆。

    PIO pio = pio0;  // 使用第一個 PIO 硬體單位。
    uint sm = pio_claim_unused_sm(pio, true);  // 取得一個未使用的狀態機。
    uint offset = pio_add_program(pio, &echo_program);  // 載入 PIO 指令。
    pio_sm_config config = echo_program_get_default_config(offset);

    // 三個參數：右移、自動從 TX FIFO 補 OSR、每 32 bits 補一筆。
    sm_config_set_out_shift(&config, true, true, 32);
    // 不開 autopush，因為 echo.pio 裡已經手動 push。
    sm_config_set_in_shift(&config, true, false, 32);
    pio_sm_init(pio, sm, offset, &config);
    pio_sm_set_enabled(pio, sm, true);  // 狀態機開始執行，等 CPU 餵資料。

    while (true) {
        uint32_t input = 21;  // 練習 1：只把這裡改成 99。

        // 這個乘法、加法由 CPU 計算，與 echo.pio 裡的指令分開。
        uint32_t cpu_answer = input * 2u + 1u;
        printf("CPU: input=%u result=%u\n",
               (unsigned)input, (unsigned)cpu_answer);

        pio_sm_put_blocking(pio, sm, input);  // CPU → TX FIFO，PIO 才拿得到。
        printf("PIO: sent=%u, waiting for RX FIFO...\n", (unsigned)input);
        fflush(stdout);  // 如果下面一直等待，先讓上一行輸出可見。

        // RX FIFO 空的時候，CPU 在這裡等；必須等 PIO push 才有結果。
        uint32_t pio_answer = pio_sm_get_blocking(pio, sm);
        printf("PIO: received=%u\n", (unsigned)pio_answer);

        sleep_ms(1000);  // 每秒重做一次；拔掉 USB 會失去這裡的電源。
    }
}
