# PIO：追清楚每筆資料放在哪裡

**PIO 是獨立執行短程式的硬體狀態機。CPU 負責設定、餵資料、收結果；PIO 執行自己的指令。** 它不是 Python，也不能直接寫 C 的 `x & 0xFF`。

## 你問過的名詞

| 名稱 | 把它想成什麼 |
|---|---|
| TX FIFO | CPU 要交給 PIO 的資料佇列 |
| OSR | PIO 準備用 out 取走資料的 32-bit 移位暫存器 |
| X、Y | PIO 中途暫放數字的兩個暫存器 |
| ISR | in 放進資料、準備交出去的 32-bit 移位暫存器 |
| RX FIFO | PIO 放結果、等 CPU 讀的資料佇列 |
| push | 指令：把 ISR 的資料放進 RX FIFO，不是 FIFO 本身 |

FIFO 是 First In, First Out，先進先出。硬體有小型儲存空間實作這個 queue；不是一般可任意索引的 C 陣列，也不是 PIO 可以自由配置的 RAM。

RX/TX 這裡按 CPU 收／送的方向理解。`push` 只先存入 RX FIFO；CPU 還要執行讀取才拿到。

## 原程式：送 21，原樣回 21

[demo_pio.pio](../../demo/section2-2/demo_pio.pio) 假設 autopull 等設定完成：

```pio
.program echo
.wrap_target
    out x, 32    ; OSR → X，取 32 bits
    in  x, 32    ; X → ISR，收 32 bits
    push         ; ISR → RX FIFO
.wrap
```

```text
CPU → TX FIFO → OSR → X → ISR → RX FIFO → CPU
           autopull   out   in    push       read
```

32 是 bit 數，不是數值 32。`.wrap_target`／`.wrap` 是組譯器的迴圈邊界標記；`.wrap` 本身不額外執行一條跳躍指令。

## 指令怎麼查

| 指令 | 資料流／用途 |
|---|---|
| `pull` | TX FIFO → OSR；本例用 autopull 自動補資料 |
| `out x, 32` | OSR → X |
| `in x, 32` | X → ISR |
| `push` | ISR → RX FIFO；未寫 noblock 時為阻塞式 |
| `mov x, y` | 將 Y 複製到 X，來源 Y 仍在 |
| `set x, 7` | 設 X 為小常數 7；set 的立即數只有 5 bits |
| `jmp 標籤` | 跳到標籤；其他條件形式依指令手冊 |

out/in 也可用其他來源目的，但不是任意組合都合法。位元截取題須一起看 shift 方向、bit 數、threshold、autopull/autopush。

## 實作練習：改程式，不只背名稱

**任務 A：改用 Y 暫存，輸入輸出不變。** 把兩行一起改：

```pio
out y, 32
in  y, 32
push
```

只改 out y、保留 in x，就是「寫 Y、卻讀 X」。若 X 原本是 7，送 99 也會回 7。

**任務 B：每筆輸入都回固定值 7。** 保留 out 消耗輸入，然後改要回傳的 X：

```pio
out x, 32
set x, 7
in  x, 32
push
```

測試輸入 `0、21、99`，都應回 7。本 repo 只有 .pio，沒有硬體啟動程式，所以目前可編輯／逐行追蹤；真正執行需下方 SDK 接線與板子環境。

<details>
<summary>考場有 Pico SDK 時：CPU 端怎麼接 echo</summary>

以下是**片段**，放入已正確建置的 Pico SDK 專案。需要 include `hardware/pio.h` 和組譯產生的 `demo_pio.pio.h`，並連結 hardware_pio。

```c
PIO pio = pio0;
uint sm = pio_claim_unused_sm(pio, true);
uint offset = pio_add_program(pio, &echo_program);
pio_sm_config c = echo_program_get_default_config(offset);
sm_config_set_out_shift(&c, true, true, 32);  // 右移、autopull、32-bit
sm_config_set_in_shift(&c, true, false, 32);  // 右移、不自動 push
pio_sm_init(pio, sm, offset, &c);
pio_sm_set_enabled(pio, sm, true);

pio_sm_put_blocking(pio, sm, 21u);           // CPU 寫 TX FIFO
uint32_t answer = pio_sm_get_blocking(pio, sm); // CPU 讀 RX FIFO
// 原版 answer=21；任務 B answer=7。
```

CMake 對**實際 executable target**補上，這裡假設 target 名叫 app：

```cmake
pico_generate_pio_header(app ${CMAKE_CURRENT_LIST_DIR}/demo_pio.pio)
target_link_libraries(app pico_stdlib hardware_pio)
```

這不是完整專案；建置與 UF2 見 [Pico SDK](RP2350與PicoSDK.md)。未在實體板驗證。

</details>

## 卡住時看誰在等誰

| 症狀 | 檢查 |
|---|---|
| CPU get 一直等 | PIO 有啟動？有輸入？有 push 或 autopush？ |
| PIO 在等輸入 | TX FIFO 有資料？OSR 補資料設定是否正確？ |
| 送幾筆後停住 | RX FIFO 滿了，CPU 是否一直送卻沒讀？ |
| 少了、重複了輸出 | 手動 push 與 autopush 是否同時啟用？實際產生幾筆？ |
| 改 32 為 8 結果怪 | 移位方向、threshold、ISR 累積與 OSR 消耗是否配套？ |

先採「送一筆 → 收一筆」驗證；批次傳輸時要安排同時排空輸出，避免有限 FIFO 互等。

來源：[RP2350 datasheet，PIO 章節](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf)、[SDK PIO API](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#group_hardware_pio)、[pio.h 設定與收送原始碼](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_pio/include/hardware/pio.h)。

[回技術目錄](README.md)
