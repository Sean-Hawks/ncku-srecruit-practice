# RP2350：怎麼讓你寫的程式真的在板子上跑

**先只追一件事：把程式裡的 21 改成 99，讓板子真的算出新答案。**

你目前沒有板子，可以先讀懂流程；要立即動手，用 [沒有板子的完整開發練習](../../demo/first-development/README.md)。下面的實際寫入步驟，需要 Pico 2、USB 資料線，以及已備妥的 Pico SDK／Arm 編譯器。

## 先接回你用過的 PlatformIO

以前你按的按鈕，其實做了這些事：

```text
改程式 → Build → Upload → Serial Monitor
          編譯     寫進板子    在電腦看輸出
```

這份教學做同樣的事，只是改用命令列與 UF2 檔案：

```text
改 main.c → cmake 建置 → 把 UF2 放進板子 → 開 USB 串口看輸出
```

**程式在板子執行；編輯器、編譯工具與觀察輸出的終端機在電腦。**

## 先認得四個名字就好

| 名字 | 它幫你做什麼 |
|---|---|
| RP2350／Pico 2 | RP2350 是晶片；Pico 2 是裝了它、USB 與供電電路的開發板 |
| Pico SDK | 一套現成程式，讓你呼叫函式使用板子的功能，例如設定 USB 文字輸出 |
| Arm 編譯器 | 把 C 與用到的 SDK 程式翻成這塊板子能執行的指令 |
| CMake | 讀建置設定，安排用哪個編譯器、哪些檔案與 SDK 功能 |

SDK 提供程式功能；CMake 安排建置；真正翻譯 C 的是編譯器。你以前由 PlatformIO 幫忙安排這些工作，現在把步驟拆開看。

## 1 在電腦改一行 C

先用這份 [自編小專案](../../demo/rp2350-pio-walkthrough/README.md)。以下假設 **Pico 2、預設 Arm 架構、Linux 電腦**；考場若給其他設定，以題目 README 為準。

在**電腦的主機終端機**，不是目前的 ROS Docker 裡：

```bash
cd ~/Development/ncku-srecruit-practice/demo/rp2350-pio-walkthrough
nvim main.c
```

先只找 [main.c](../../demo/rp2350-pio-walkthrough/main.c) 裡這兩行：

```c
uint32_t input = 21;
uint32_t cpu_answer = input * 2u + 1u;
```

意思是：準備數字 21，算 21×2+1，所以答案是 43。uint32_t 是 unsigned 32-bit 整數；這裡的 u 表示 unsigned 常數。

把第一行的 21 改成 99，用 `:w` 存檔。**現在只改到電腦上的文字檔，板子還沒收到新程式。**

## 2 在電腦編譯，得到 UF2

前提：這個 shell 已備妥 Pico SDK 與 Arm 編譯器。缺環境時先看後面的「編譯找不到工具」，不需要先插板子才能編譯。

仍在剛才的專案目錄，首次建置：

```bash
cmake -S . -B build-pico2 -DPICO_BOARD=pico2
cmake --build build-pico2
```

- 第一行：讀目前目錄的設定，指定 Pico 2，準備 build-pico2 建置目錄。
- 第二行：依設定編譯、連結，產生給板子使用的程式。

成功後應有這個檔案：

```text
build-pico2/pico_pio_walkthrough.uf2
```

**UF2 就是稍後要交給板子的程式檔。** 到這一步，它仍在電腦裡；不會因為編譯成功，就自動改掉板子正在跑的程式。

下次只改 C 程式，通常重新執行第二行就夠了。

## 3 用 USB 把 UF2 寫進板子

這一步才需要實體 Pico 2 與能傳資料的 USB 線。

1. 拔掉板子的 USB。
2. 按住板子的 **BOOTSEL** 按鈕，同時把 USB 接回電腦，再放開按鈕。
3. 電腦會看到名為 **RP2350** 的磁碟；這是板子提供的寫入入口。
4. 用電腦的檔案管理器，把剛編出的 **pico_pio_walkthrough.uf2** 複製進去。
5. 寫入完成後磁碟消失，板子重新啟動，開始執行新程式。

這個動作也叫「燒錄」。原始碼通常留在電腦，寫進板子的是編好的程式。只需這條 USB 線，本例不用接其他電路。

## 4 在電腦看板子印出的答案

程式執行後，這份範例讓同一條 USB 線提供文字輸出通道，叫 **USB serial／串口**。這是以前 Serial Monitor 在看的那類輸出。

在**接著板子的電腦終端機**：

```bash
ls /dev/ttyACM*
```

如果確認板子出現在 /dev/ttyACM0，就貼：

```bash
stty -F /dev/ttyACM0 115200 raw -echo
cat /dev/ttyACM0
```

第一行設定讀取方式，第二行持續顯示收到的文字。裝置名稱不同時，要改成實際名稱。

這份程式約每秒重複執行。你把 input 改成 99 後，找這一行：

```text
CPU: input=99 result=199
```

**看到它，就完成「修改 → 編譯 → 寫入 → 板子執行 → 電腦驗證」。** 程式還會印 PIO 的兩行；第一輪先確認上面這行即可，後面再解釋 PIO。

按 Ctrl-C 會停止電腦的 cat，板子仍在跑。本例只用 USB 供電，拔線後失去這路電源；再接上時，會重新執行已寫入的程式。

## 5 再改一次，練熟同一條流程

這次把 input 改回 21，再把 CPU 公式改成：

```c
uint32_t cpu_answer = input * 3u + 1u;
```

照剛才的順序：**存檔 → 建置 → 重新進 BOOTSEL → 複製新 UF2 → 開串口觀察**。預期：

```text
CPU: input=21 result=64
```

| 你目前做到哪裡 | 新程式在哪裡 |
|---|---|
| 只存檔 | 電腦上的 main.c |
| 編譯成功 | 電腦上的 UF2 |
| UF2 寫入成功 | 板子裡，重新啟動後執行 |

**改了卻還看到舊答案，就沿這三格找漏掉哪一步。** Docker 掛載也只影響檔案位置，不會替你把新程式寫進板子。

## 跑通後，再展開需要的細節

<details>
<summary>專案的三份檔案，各自負責什麼？SDK 在哪裡用到？</summary>

| 檔案 | 在哪裡處理／執行 | 用途 |
|---|---|---|
| main.c | 電腦編譯；板子的 CPU 執行 | 計算、設定 PIO、收結果、輸出文字 |
| echo.pio | 電腦組譯；板子的 PIO 執行 | 本例只把送進來的數字原樣送回 |
| CMakeLists.txt | 電腦上的 CMake 讀取 | 指定來源、SDK 功能、USB 輸出與 UF2 產生 |

main.c 呼叫 SDK 的 stdio_init_all()，配合 CMake 啟用 USB stdio，printf 才能透過 USB 把文字交給電腦。你不用先自己實作 USB 驅動。

CMake 還會把 echo.pio 組譯成 echo.pio.h，讓 CPU 的 C 程式能載入那段 PIO 指令；產生的 header 不用手改。

</details>

<details>
<summary>為什麼還有 PIO 的答案？它怎麼和 CPU 分開？</summary>

main.c 的 CPU 公式與 echo.pio 是兩段不同工作：

```text
CPU 算式：21 → 乘二加一 → 43
CPU 另送一份 21 → PIO 原樣回傳 → CPU 收到 21
CPU 用 printf 把兩份結果送到電腦
```

CPU 這樣送一筆、收一筆：

```c
pio_sm_put_blocking(pio, sm, input);  // CPU 放進 TX FIFO
uint32_t pio_answer = pio_sm_get_blocking(pio, sm); // CPU 等 RX FIFO
```

PIO 執行：

```pio
out x, 32   ; OSR → X
in  x, 32   ; X → ISR
push        ; ISR → RX FIFO，CPU 才能讀到
```

autopull 等設定已在 main.c 寫好。CPU 與 PIO 這段是晶片內部通訊；USB 則把 printf 文字送到外面的電腦。

如果要練改 PIO，在 echo.pio 的 out 與 in 之間加 `set x, 7`，再重新建置、寫入：PIO 就會回 7。它不會改掉 CPU 的乘法公式。看 [PIO 資料流](PIO.md)。

</details>

<details>
<summary>編譯找不到工具、SDK，怎麼辦？</summary>

在準備做建置的電腦／指定環境查：

```bash
command -v cmake
command -v arm-none-eabi-gcc
printf '%s\n' "$PICO_SDK_PATH"
```

前兩個應列出工具路徑；PICO_SDK_PATH 是這份自編專案用來找 SDK 的目錄設定。SDK 中應有 external/pico_sdk_import.cmake。

假設現場 SDK **真的在 /opt/pico-sdk**，才執行：

```bash
export PICO_SDK_PATH=/opt/pico-sdk
```

這條只指定位置，不會安裝 SDK。考場沿用提供的環境；自己從零準備可看 [環境建置](RP2350-eBNN從單板到多板.md)。目前的 ROS Docker 不是已備妥的 Pico 工具鏈環境。

</details>

<details>
<summary>常見卡點：先找自己停在哪一步</summary>

| 症狀 | 先查 |
|---|---|
| CMake 找不到 SDK／編譯器 | 建置環境與 SDK 路徑，還沒到 USB 問題 |
| 沒有 UF2 | 編譯是否成功；看第一個錯誤 |
| 按 BOOTSEL 沒出現磁碟 | 是否按住再插、線能否傳資料、板型是否正確 |
| 找不到 ttyACM | 是否仍在 BOOTSEL；是否已寫入這份啟用 USB serial 的程式 |
| 有 PIO sent，沒有 received | CPU 等不到 PIO 回覆；查 echo.pio 是否 push |
| 還是舊數字 | 存檔、重新建置、複製的是不是新 UF2 |

</details>

下一階段才接 [eBNN、ROS 與複數板子的完整作答流程](RP2350-eBNN從單板到多板.md)。

來源：[官方 Pico 板子與 BOOTSEL](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html)、[Pico SDK](https://github.com/raspberrypi/pico-sdk/blob/master/README.md)、[USB stdio](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_stdio_usb/include/pico/stdio_usb.h)。PIO 程式已用官方 pioasm 組譯；完整韌體與 USB 輸出尚未實板驗證，以上為預期結果。

[回技術目錄](README.md)
