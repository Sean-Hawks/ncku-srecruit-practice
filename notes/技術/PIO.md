# PIO：看資料從哪裡移到哪裡

[← 回考場筆記入口](../README.md) · [ISA 編碼](ISA與指令編碼.md) · [RP2350 專案](RP2350與PicoSDK.md)

**PIO 是能自行執行短指令的硬體周邊。** CPU 設定它，再送資料、收結果；PIO 按自己的指令控制接腳或移動 bits。

官方演練的位元／PIO 節（section2-2） 提供 echo 程式，並特別提醒查閱 PIO 章節。本頁按 RP2350／Pico SDK 說明。

## 你問過的 FIFO、RX、push

| 名稱 | 是什麼 | 本例的作用 |
|---|---|---|
| TX FIFO | CPU 送給 PIO 的先進先出佇列 | 暫存待處理資料 |
| OSR | 32-bit 輸出移位暫存器 | out 從這裡拿 bits |
| X、Y | 兩個 32-bit 暫存器 | 暫放數字 |
| ISR | 32-bit 輸入移位暫存器 | in 把 bits 放進這裡 |
| RX FIFO | PIO 交給 CPU 的先進先出佇列 | 暫存結果 |
| push | 一條指令 | ISR → RX FIFO |

**FIFO 是 queue，硬體用有限儲存空間實作。** 不是任意索引的 RAM。RX/TX 在此按 CPU 收／送理解。push 只把結果放入佇列，CPU 還要讀取才拿得到。

一般每個狀態機有 TX/RX 各 4 個 32-bit 項目；FIFO join／特殊模式可能改變用途與容量，依設定查。不要假設能無限塞資料。

## 官方 echo，逐行已解讀

以下假設 CPU 已載入程式、啟動狀態機，且設好 32-bit autopull：

```pio
.program echo
.wrap_target
    out x, 32    ; OSR 的 32 bits → X
    in  x, 32    ; X 的 32 bits → ISR
    push         ; ISR → RX FIFO
.wrap
```

```text
CPU 寫 21 → TX FIFO → OSR → X=21 → ISR=21 → RX FIFO → CPU 讀到 21
            autopull     out       in        push
```

32 是位數，並不是把 X 設成數值 32。`.wrap_target`／`.wrap` 是組譯器的迴圈邊界標記，不會各執行一條額外指令。

## 指令與會等待的地方

| 指令／機制 | 意思 | 等待條件 |
|---|---|---|
| pull／autopull | TX FIFO → OSR | 需要資料而 TX 空時，阻塞形式會等 |
| out | OSR → 指定目的，例如 X／pins | 啟用 autopull 時還受補資料影響 |
| in | 指定來源 → ISR | 啟用 autopush 時還受 RX 空間影響 |
| push | ISR → RX FIFO，並清 ISR／其移位計數 | 預設阻塞，RX 滿會等 |
| mov x, y | Y 的值複製到 X | 不會消耗 FIFO 項目 |
| set x, 7 | X 設為小常數 7 | 立即數只有 5 bits，0..31 |
| wait | 等指定接腳／IRQ 條件 | 條件尚未成立 |
| jmp | 按條件決定下一條指令 | 條件形式看手冊 |

autopull 是自動補 OSR；autopush 是自動送出 ISR。**手動 push 與 autopush 若沒配合，可能多送一筆或送出非預期資料。**

## 改哪一行，會變成什麼

以下以每筆處理完整 32 bits、其他設定相同為前提，是已解讀的對照：

| 指令片段 | 送 21 的結果 | 原因 |
|---|---|---|
| `out x,32; in x,32; push` | 21 | 寫 X 再讀 X |
| `out y,32; in y,32; push` | 21 | 兩邊一起改成 Y |
| `out y,32; in x,32; push` | X 的舊值 | 寫 Y，卻從 X 讀 |
| `out x,32; set x,7; in x,32; push` | 7 | 輸入被消耗，但 X 被常數覆蓋 |
| 只把 32 改成 8 | 要看完整設定 | 只移 8 bits，方向、累積量與 threshold 都會影響結果 |

分號在這張表用來分隔指令；實際 `.pio` 要一行一條，分號後的內容是註解。

## CPU 端常見 API：不是寫了 .pio 就會自己跑

| 階段 | Pico SDK API／設定 | 用途 |
|---|---|---|
| 選資源 | `pio_claim_unused_sm`、`pio_add_program` | 取得狀態機、載入指令 |
| 取得設定 | `echo_program_get_default_config(offset)` | 由組譯產生，名稱隨 .program 改變 |
| 移位設定 | `sm_config_set_out_shift(&c, true, true, 32)` | 此例：右移、autopull、threshold 32 |
| 移位設定 | `sm_config_set_in_shift(&c, true, false, 32)` | 此例：右移、不 autopush，配手動 push |
| 初始化／啟用 | `pio_sm_init`、`pio_sm_set_enabled` | 裝入設定、開始執行 |
| CPU 送資料 | `pio_sm_put_blocking(pio, sm, value)` | 寫 TX，滿了會等 |
| CPU 讀結果 | `pio_sm_get_blocking(pio, sm)` | 讀 RX，空的會等 |

上表是 API 對照，並非獨立完整程式。`pio`、`sm`、`offset` 都來自實際專案的資源配置。

CMake 的 `pico_generate_pio_header` 把 `.pio` 組成 C 可引用的標頭；主程式還需 include 產生的 `.pio.h`、連結 `hardware_pio`。若控制接腳，另外查 pin mapping、方向與時鐘設定。

## PIO 的編碼和 CPU 不同

`out x, 32` 在沒有額外 delay／side-set 時編成 `0x6020`：

| bits | 含義 |
|---|---|
| 15..13 = 011 | OUT |
| 12..8 = 0 | 此例沒有 delay／side-set |
| 7..5 = 001 | 目的地 X |
| 4..0 = 00000 | OUT 的數量欄位中，0 編碼代表 32 bits |

PIO 指令是 16 bits；中間 5 bits 如何分配給 delay／side-set 取決於設定。JMP 的目標是 PIO 指令位址，不能套 CPU 的 byte 位址規則。

## 卡住時查表

| 症狀 | 先查 |
|---|---|
| get 一直不回來 | 狀態機有啟動？TX 有資料？有 push／autopush？ |
| 送幾筆就停止 | RX 是否滿？CPU 是否只送不讀？ |
| bits 順序或數量錯 | shift 方向、threshold、每次 in/out 位數、ISR 累積 |
| 多一筆／少一筆 | 手動 push 與 autopush、條件式 push、實際迴圈次數 |
| 接腳沒反應 | GPIO 功能選擇、pin mapping、方向、clock、wait 條件 |

來源：[RP2350 datasheet：搜尋 PIO、Instruction Set、FIFO](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf)、[PIO API](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#group_hardware_pio)、[設定與 FIFO 定義](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_pio/include/hardware/pio.h)、[指令編碼](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_pio/include/hardware/pio_instructions.h)。資料流與指令編碼已核對；未做實板驗證。
