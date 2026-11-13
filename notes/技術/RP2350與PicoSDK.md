# RP2350 實際操作：改程式 → 編譯 → 寫入板子 → 看結果

若要接 eBNN、做效能優化或複數板子，接著看 [從單板到多板的作答流程](RP2350-eBNN從單板到多板.md)。本頁先把一片板子的編譯、寫入與輸出講清楚。

**你在電腦上編輯程式，把編好的程式寫進板子；RP2350 自己執行，電腦透過 USB 看輸出。** 不用 SSH 進板子，也不是在板子裡開 Linux。

```text
電腦：main.c + echo.pio + CMakeLists.txt
                 ↓ 編譯
電腦：pico_pio_walkthrough.uf2
                 ↓ 用 USB 寫入
板子：CPU 執行 main.c，PIO 執行 echo.pio
                 ↓ USB printf
電腦終端機：看見計算結果
```

## 0 先確認現在能做哪一段

| 你手上有什麼 | 現在能做什麼 |
|---|---|
| 只有這份 repo | 看程式、改範例、理解每一步 |
| 再有 Pico SDK 與 ARM 交叉編譯器 | 編出給 Pico 2 執行的 UF2，不必先插板 |
| 再有 Pico 2、USB 資料線 | 寫入、執行、從 USB 看結果 |

目前檢查 archlinux：沒有找到 arm-none-eabi-gcc、picotool 或 /dev/serial/by-id。**這份教學不代表目前已連上板子；原本 Docker 也沒有 ARM 編譯器。**

下方用 **Pico 2、預設 Arm 架構、Linux 主機**舉例。RP2350 是晶片，Pico 2 是使用它的開發板；其他板型、RISC-V 或考場指定流程，採題目設定。

## 1 先打開完整範例，認識三份檔案

新增的 [自編練習專案](../../demo/rp2350-pio-walkthrough/README.md) 與官方 section2-2 分開，沒有改掉官方原始範例。

在**主機終端機**：

```bash
cd ~/Development/ncku-srecruit-practice/demo/rp2350-pio-walkthrough
nvim main.c
```

| 檔案 | 做什麼 | 你通常改哪裡 |
|---|---|---|
| [main.c](../../demo/rp2350-pio-walkthrough/main.c) | CPU 的程式 | input、CPU 公式、PIO 設定與收送 |
| [echo.pio](../../demo/rp2350-pio-walkthrough/echo.pio) | PIO 的程式 | out、in、push 等指令 |
| [CMakeLists.txt](../../demo/rp2350-pio-walkthrough/CMakeLists.txt) | 告訴工具怎麼建置 | library、USB 輸出、PIO header 產生 |

**輸入 21 是 main.c 裡固定的數字，這個小專案沒有接 ROS。** 先讓 CPU、PIO、USB 三者跑通，之後才能接題目提供的外部輸入。

## 2 認清同一塊晶片裡的兩個執行者

CPU 執行 main.c 這一行：

```c
uint32_t cpu_answer = input * 2u + 1u;
```

input=21，所以 CPU 算出 43。`u` 表示 unsigned 常數，並不改變這裡乘二加一的意思。

PIO 執行 echo.pio：

```pio
out x, 32    ; 把輸入放進 X
in  x, 32    ; 把 X 放進 ISR
push        ; 放進 RX FIFO，等 CPU 讀
```

沒有計算，所以 PIO 回 21。main.c 裡的 CPU 會呼叫：

```c
pio_sm_put_blocking(pio, sm, input);  // CPU 把 21 放到 TX FIFO
uint32_t pio_answer = pio_sm_get_blocking(pio, sm); // 等 RX FIFO 的結果
```

這些是**晶片內部 CPU 與 PIO 的通訊**，不是 USB 上的資料傳輸。USB 只負責把 printf 的文字送到電腦。

## 3 確認編譯工具；這裡不用先插板子

在**準備好 Pico 工具鏈的主機終端機**：

```bash
command -v cmake
command -v arm-none-eabi-gcc
printf '%s\n' "$PICO_SDK_PATH"
```

前兩行應列出程式路徑；第三行應是已安裝的 Pico SDK 目錄。SDK 裡應有 external/pico_sdk_import.cmake。

SDK 路徑沒設定時，依環境文件指定；例如 SDK **真的位於 /opt/pico-sdk** 才使用：

```bash
export PICO_SDK_PATH=/opt/pico-sdk
```

這只是指定位置，不會安裝 SDK。考場用現場預裝的環境，不需要為了筆記另裝工具；缺工具與編譯失敗是兩個不同問題。

## 4 編譯：產生要交給板子的檔案

仍在 rp2350-pio-walkthrough 目錄：

```bash
cmake -S . -B build-pico2 -DPICO_BOARD=pico2
cmake --build build-pico2 -j 2
```

| 指令／參數 | 做什麼 |
|---|---|
| 第一行 | 設定這份專案要怎麼建，目標是 Pico 2 |
| -S . | 原始碼就在目前目錄 |
| -B build-pico2 | 編譯產物放這個目錄 |
| 第二行 | 真正開始建置 |
| -j 2 | 最多同時執行兩個建置工作，不是設定板子雙核心 |

建置過程會把 echo.pio 組譯成 echo.pio.h，讓 C 程式能載入那段 PIO 指令；再編 main.c、連結 SDK，產生 UF2。

檢查產物：

```bash
ls -l build-pico2/pico_pio_walkthrough.uf2
```

**這一步只在電腦產生檔案，板子裡的程式還沒改。** 不要在 Linux 執行 ./pico_pio_walkthrough.elf；它是給目標晶片的程式。

## 5 把程式寫進 Pico 2

這一步要有實體板。使用可以傳資料的 USB 線；本例不用另外接感測器、LED 或 UART 轉接器。

1. 拔掉板子的 USB。
2. 按住板子的 **BOOTSEL** 按鈕。
3. 按住時，把 USB 接上電腦，再放開按鈕。
4. 電腦應出現名為 **RP2350** 的磁碟。它是燒錄入口，不是可 SSH 的電腦。
5. 在檔案管理器，把剛編出的 **build-pico2/pico_pio_walkthrough.uf2** 複製進該磁碟。
6. 寫入完成後磁碟會消失、板子重新啟動，開始執行新程式。

原始 Pico 的磁碟名稱可能是 RPI-RP2；先確認板型，不要拿 pico2 編出的程式隨便寫給別的板子。

<details>
<summary>已有 picotool、只接著要寫入的那一塊板子時，可以用命令列</summary>

先進 BOOTSEL，再在**連著板子的主機**、本專案目錄執行：

```bash
picotool info
picotool load -x build-pico2/pico_pio_walkthrough.uf2
```

info 用來確認裝置；load 寫入，-x 在寫入後執行。若有多片板子，依現場方式選定目標，不直接寫到未知裝置。這是取代上面拖 UF2 的另一種方式，不是兩種都要做。

</details>

## 6 看結果：USB 不再是磁碟，而是串口

BOOTSEL 模式用來寫程式；這個範例執行時，改提供 USB serial。CMake 已啟用 USB stdio，程式已呼叫 stdio_init_all()，所以 printf 才有這個輸出管道。

在**連著板子的主機**：

```bash
ls /dev/ttyACM*
```

假設找到的板子是 /dev/ttyACM0；先確認這是它，再：

```bash
stty -F /dev/ttyACM0 115200 raw -echo
cat /dev/ttyACM0
```

程式每秒印一次，預期看到：

```text
CPU: input=21 result=43
PIO: sent=21, waiting for RX FIFO...
PIO: received=21
```

用 Ctrl-C 結束 cat。**這只停止電腦的觀察程式，板子仍在跑。** 本例靠 USB 供電，拔線後才失去這路電源；再次接上會重新執行已寫進 flash 的程式。

沒有 ttyACM 時，查實際 USB 裝置與韌體設定；不是所有板子韌體都有 USB serial。權限不足依現場帳號配置處理，不要亂改所有裝置權限。

## 7 改一行，驗證你改到的是誰

### 練習 A：改輸入，CPU 與 PIO 都會看到新值

main.c：

```c
uint32_t input = 99;
```

存檔 → **重新 build → 重新進 BOOTSEL → 寫新 UF2 → 重新開串口觀察**。預期：

```text
CPU: input=99 result=199
PIO: sent=99, waiting for RX FIFO...
PIO: received=99
```

只改主機檔案或只 build，板子都還是舊程式。Docker 的即時掛載也不會自動更新板子 flash。

### 練習 B：改 PIO，讓它每次固定回 7

main.c 的 input 先改回 21。echo.pio 的 .wrap_target 與 .wrap 之間改成：

```pio
out x, 32
set x, 7    ; 新增這行，蓋掉 X 原本的輸入值
in  x, 32
push
```

存檔、重新 build、重新寫入。預期：

```text
CPU: input=21 result=43
PIO: sent=21, waiting for RX FIFO...
PIO: received=7
```

CPU 算式仍是原本那一行；PIO 的結果由 echo.pio 決定。練完刪掉 set x, 7，可恢復原樣回傳。

## 8 沒成功時，對照停在哪一步

| 停在哪 | 先檢查 |
|---|---|
| CMake 說 Set PICO_SDK_PATH | 指定既有 SDK 的真正路徑 |
| 找不到 arm-none-eabi-gcc | 工具鏈未備妥，或不在現場指定的 shell |
| SDK 說找不到 TinyUSB | SDK USB 相依內容不完整，使用完整的預裝環境 |
| 找不到 UF2 | 先看 build 的第一個錯誤，不先猜 USB 壞了 |
| 按 BOOTSEL 沒出現磁碟 | 線能不能傳資料、是否按住再插、板型是否正確 |
| 看不到 ttyACM | 是否仍在 BOOTSEL、是否寫入這份 USB stdio 韌體 |
| 有 sent，沒有 received | CPU 卡在等 RX FIFO；查 PIO 是否 push、設定是否配套 |
| 修改後仍印舊值 | 是否存檔、重新 build，並把新的 UF2 寫進板子 |
| cat Ctrl-C 後板子沒停 | 正常；cat 是主機觀察者，不是板子的執行程序 |

## 9 遇到正式考場專案時

先讀題目 README，確認板型、Arm／RISC-V、建置與寫入方法。已有正確 build 配置時，通常只需依其指令 rebuild，不重新覆蓋 PICO_BOARD。

看到 main.c／.pio／CMakeLists，依上面的分工找位置。接 ROS 時，另外確認電腦與板子的封包、輸入來源與結果通道；本例固定值回傳只是第一階段的驗證。

下一步：[PIO FIFO 與等待](PIO.md)、[資料搬運／DMA](資料搬運.md)、[平行運算](平行運算.md)、[序列封包](序列通訊與封包.md)。

來源：[官方板子與 BOOTSEL 說明](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html)、[Pico SDK 建置](https://github.com/raspberrypi/pico-sdk/blob/master/README.md)、[USB stdio](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_stdio_usb/include/pico/stdio_usb.h)、[picotool](https://github.com/raspberrypi/picotool)。

驗證範圍：已用官方 Pico SDK 的 pioasm 成功組譯原版與固定回 7 兩版，並確認缺 SDK 時的 CMake 提示、文件連結與 shell 語法。這個環境缺 ARM 工具鏈，本次未做完整交叉編譯、實板寫入與 USB 輸出測試；上方輸出是依程式推導的預期值。

[回技術目錄](README.md)
