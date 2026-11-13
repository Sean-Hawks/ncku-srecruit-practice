# 拿到 RP2350 題目後：從單板 eBNN 跑通到多板優化

**照這個順序做：原版建置 → 單板已知答案 → 一筆完整往返 → 一批正確結果 → 分段計時 → 多板分派 → 核心與搬運優化。**

以下以「題目提供 eBNN 模型與程式，要求板子做推論」為情境，不表示今年一定考 eBNN。本 repo 沒有正式考試模型；標成片段的程式要接上題目真正的介面，不能直接當完整韌體跑。

## 先看完整工作長什麼樣

```text
電腦：讀圖片 → 前處理 → 分派工作 → 傳輸 → 收回分類 → 按編號交答案
                                      ↕
板子：                       收完整圖片 → eBNN 推論 → 回傳分類
```

你通常要寫的是接線、格式轉換、分工與效能優化。模型已給好時，先呼叫它；不從訓練模型開始。

| 部分 | 在哪裡執行 | 先找哪份檔案 |
|---|---|---|
| 收題目輸入、發布結果 | 電腦上的 ROS node／主機程式 | Python callback、host/、bridge/ |
| 板子通訊、呼叫模型 | RP2350 CPU | main.c、protocol、infer/predict |
| eBNN 計算與權重 | 通常在板子 CPU／flash、RAM | 模型 header、ebnn_compute 宣告 |
| PIO／DMA | 有題目需求或量到瓶頸才使用 | .pio、pio 初始化、dma 設定 |

PIO 不是 ROS 2 的替代品；eBNN 也不會因為放上 RP2350 就自動使用 PIO。

## 1 拿到 repo，先填六格

先讀題目、README、評分／測試指令。遇到 README 與題目規格不一致，依現場說明確認，不憑去年心得猜今年規則。

```text
板型與 CPU 架構：________，例如 Pico 2 / Arm（不能只寫 RP2350）
建置／寫入指令：________，優先沿用專案現成設定
輸入：________ 個元素，型別 ________，總 bytes ________
前處理：________，正規化、二值化、排列與 endian
輸出：________，例如類別編號／分數陣列
正確答案與評分方式：________，逐筆正確性、延遲／吞吐量
```

在**題目 repo 根目錄**：

```bash
pwd
ls
rg --files -g 'README*' -g '*CMakeLists.txt' -g 'platformio.ini' -g '*test*' -g '*model*'
rg -n 'ebnn_compute|predict|inference|PICO_BOARD|PICO_PLATFORM' .
```

沒有 rg 就用編輯器搜尋。找到的是原始碼檔案，還要看函式宣告與呼叫範例，不只看搜尋到的名稱。

## 2 建環境：先用提供的，再考慮自己從零裝

### 考場／已有專案

確認題目指定在哪個 shell／container 建置；USB 在哪台電腦，就在能存取它的環境做寫入與收送。現在 ROS Docker 的存在，不代表它也有 Pico 工具鏈或 USB 裝置。

在**指定建置環境**檢查：

```bash
command -v cmake
command -v arm-none-eabi-gcc
printf '%s\n' "$PICO_SDK_PATH"
```

這組工具適用 Arm 例子；RISC-V 專案依其工具鏈，不硬改成 Arm。若已有 build 配置，先按原指令建置，常見形式：

```bash
cmake --build build -j 2
```

不是所有 repo 都叫 build；從零建置且**已確認是 Pico 2、預設 Arm**才用：

```bash
cmake -S . -B build-pico2 -DPICO_BOARD=pico2
cmake --build build-pico2 -j 2
```

先讓未修改版本編譯通過，記下產物名稱。工具錯、include 錯、模型計算錯、USB 錯要分開處理。

<details>
<summary>只供自己的 Arch Linux 新環境：安裝與 SDK 範例，不在考場執行</summary>

這會更新系統並安裝工具。在自己的主機、允許安裝時：

```bash
sudo pacman -Syu --needed base-devel cmake ninja git arm-none-eabi-gcc arm-none-eabi-newlib python-pyserial
```

使用支援 RP2350 的 SDK。以下固定 2.3.1 作為教學版本，不覆蓋題目指定版本；目標目錄不存在時才 clone：

```bash
git clone --branch 2.3.1 --depth 1 --recurse-submodules \
  https://github.com/raspberrypi/pico-sdk.git ~/Development/pico-sdk
export PICO_SDK_PATH="$HOME/Development/pico-sdk"
arm-none-eabi-gcc --version
```

export 只設定這個 shell 的 SDK 路徑。USB stdio 要有 TinyUSB 等 SDK 相依內容；缺少時先檢查 SDK 是否完整，不亂改題目程式。

現在使用的 ROS image 沒包這套 Arm 編譯器；不要把主機安裝套件和容器安裝套件當成同一件事。本次沒有替你安裝或改掉系統環境。

</details>

## 3 單板第一版：先拿一筆已知資料，直接呼叫模型

先不接 ROS、不做兩板。使用題目提供的範例輸入，在板子上呼叫模型，檢查輸出等於標準答案。

**輸入格式要讀宣告，不能由 eBNN 這個名字推定。** 原作者的一個測試模型宣告是 `ebnn_compute(float *input, uint8_t *output)`，不是 uint8_t 圖片輸入；其他模型可以不同。

| 假設 28×28 圖片 | 每張大小（此平台） |
|---|---|
| 每像素 uint8_t | 784 bytes |
| 每像素 float32 | 3136 bytes |
| 已定義成每像素一個 bit | 98 bytes，但必須是模型真正支援的輸入格式 |

**改成 binary 傳輸，不等於可以把模型需要的 float32 擅自壓成一個 bit。** 先保持輸入內容與格式一致。

在**題目已有正確 input/output buffer 的 C 程式**裡，計時片段像這樣：

```c
uint64_t started = time_us_64();      // 需 pico/stdlib.h／對應時間 API
ebnn_compute(input, output);         // 參數大小與型別依題目模型
uint64_t compute_us = time_us_64() - started;
```

這是放入現有模型程式的片段，不會自動建立 input/output。暫時可用 USB 文字印結果；正式改 binary 後，文字 log 要和資料通道分開。

若模型是 generated header，CMake 要包含它的位置；若還有 .c，需把實作連結進目標。不要因為 #include header 就假設所有實作都已編進去。

編好後按 [RP2350 寫 UF2 與看 USB 輸出](RP2350與PicoSDK.md) 操作。這一步過關：**同一筆輸入，在板子得到題目預期答案。**

## 4 接主機：一筆圖片完整往返，再接 ROS

先用題目提供的 host 程式或一支小腳本送同一筆已知圖片，板子回相同答案。若這時變錯，優先查傳輸／前處理，不先改模型。

題目已有封包就沿用。沒有時，自訂協定至少要講清楚：

| 欄位 | 用途 |
|---|---|
| magic／類型 | 識別封包開頭、請求或回覆 |
| job_id | 哪張圖片的工作；回傳須帶相同編號 |
| payload_bytes | 資料長度，單位是 bytes |
| payload | 完整圖片或分類結果，型別、排列、endian 已約好 |
| compute_us（可選） | 板子回報這筆純推論耗時 |

發送端先將資料打包成 bytes；接收端累積到完整長度，檢查長度符合模型才計算。一個 read 不保證收到一整張圖片。

本 repo 第一次開發的八-byte 封包只裝一個整數，**不能原封不動拿來塞一張圖片**。看 [封包速查](序列通訊與封包.md) 再按題目格式修改。

一筆往返過關後，接 ROS callback：收訊息 → 轉成模型輸入 → 指定板子傳輸 → 收答案 → 發布結果。現在 Int32 範例只用於一個整數；圖片需題目指定的訊息型別與欄位，先用 ros2 interface show 查。

## 5 建立一批正確答案，才開始優化

固定一批 N 張測資。先把單板答案按 job_id 存成基準。每次修改後，確認：

- 回來剛好 N 筆，沒有漏、重複或未知 ID。
- 每個 job_id 的答案與標準／原先驗證正確的版本一致。
- 分類題的合併是 `answers[job_id] = class_id`，**不是把類別編號相加**。

記錄總時間，量的是從送工作到收到全部結果；板子內另外量純 compute。主機與板子時鐘不同，不直接拿兩端的絕對時間戳相減。

```text
測資：相同的 N 張圖片
正確：N/N；每個 ID 的答案相同
總耗時：主機送出前 → 收完全部
板上推論：每筆 compute_us
修改：這次只改哪一項
```

不要先宣稱瓶頸在哪裡。以下數字只是判讀例子：

| 量到的情況 | 先做什麼 |
|---|---|
| 每張傳輸 80ms、推論 5ms | 查格式、封包大小、往返次數；分核心未必有用 |
| 傳輸 2ms、推論 80ms | 看運算／多板，先維持相同答案 |
| 每送一筆就等很久才送下一筆 | 查主機是否全程序列等待，能否讓不同板子同時有工作 |
| 接訊息時被長計算卡住 | 考慮收送與運算分核心、以 queue 交工作 |

## 6 第二塊板：先讓兩塊各自獨立跑通

每塊板有自己的 flash、RAM 與程式。**若用相同板型做同樣推論，兩塊可寫相同韌體與模型。** 若混 ESP32／RP2350，要各自建置、寫入自己的韌體，輸入輸出協定與模型版本一致。

在**USB 連著的主機**辨識裝置：

```bash
ls -l /dev/serial/by-id/
ls -l /dev/serial/by-path/
```

若有 pySerial：

```bash
python3 -m serial.tools.list_ports -v
```

記下「板 A → 哪個實際裝置、板 B → 哪個實際裝置」。有唯一序號時可用 by-id；沒有時依 USB 位置／逐片插拔辨識。不要假設每次 ttyACM0 都是同一塊板。

初次寫入最容易掌握的方式：一次只接要寫的那片，確認其板型與 UF2，完成後再接另一片。每塊都單獨用同一筆測資驗證，再同時接上。

## 7 分派：不同圖片交給不同板，不先拆神經網路

如果題目允許對多張獨立圖片做推論，先讓每張完整跑一次模型：

```text
待處理圖片：ID 0、1、2、3、4、5…
              ├→ 板 A：圖片 0 → 完成後再拿下一張
              └→ 板 B：圖片 1 → 完成後再拿下一張
電腦：回來一筆，就按 job_id 放回原位置
```

兩塊都要有完整模型。先不把同一張圖片的一半神經網路拆到另一塊；那需要額外的中間層格式、傳輸與同步，若題目指定模型切分才依規格做。

最容易做的排程：**哪塊空了，就交下一筆給它。** 每塊先維持一份進行中工作，回覆後才重用它的輸入 buffer。

要真的同時工作，主機需同時管理兩個連線，例如每個板子各一條 I/O thread；不能「送 A → 等 A 完 → 送 B → 等 B 完」。每個連線一次由一個接收者解析，避免兩條 thread 搶讀同一串 bytes。

100 張可以先做平均分工驗正確；速度不同時再做空閒者取下一張。慢板不應拖住所有快板。

超時的 job 要標記未完成；重送時使用可辨識的 ID／重試狀態，晚到的舊答案不能重複計入。第一版可先在超時時停止報錯，不把缺的答案填成 0。

## 8 同一塊板的兩個核心：先一個收送，一個推論

```text
Core 0：收完整封包 → 工作 queue ─→ Core 1：推論
Core 0：傳回結果   ← 結果 queue ←─ Core 1：完成
```

Pico SDK 的 queue 可以複製固定大小元素，配合 multicore_launch_core1 啟動第二核心。傳的是完整資料還是 buffer 指標，要分清楚：**queue 複製指標不會複製整張圖片**，推論結束前不能覆寫／釋放該 buffer。

不要直接兩個核心同時呼叫同一個模型，先查是否有共用可修改暫存區。原作者 onelayer.h 就有全域 temp1；這種介面需調整成各自的暫存區或避免同時推論。

這樣分工能重疊收送與運算，但仍須處理 queue 滿、結果排空與 buffer 歸還。若 Core 0 塞滿工作後一直等空位、卻不收走結果，也可能互等。

## 9 PIO／DMA 最後再按需求加

| 量到的需求 | 考慮 |
|---|---|
| 特定位元流解析、接腳輸入輸出時序 | PIO |
| 大量記憶體／支援的周邊資料搬運 | DMA；確認資料寬度、count、對齊與完成通知 |
| 模型計算占大多數時間 | 先看模型運算、工作分配；PIO 不會自動加速整個 eBNN |

USB 傳輸不會因為程式多建一個 DMA channel 就自動變快。先查實際資料路徑與 SDK／驅動的支援，再考慮這類改動。

## 現在沒有板子，能先做什麼

先完成 [第一次開發](../../demo/first-development/README.md)，再在**容器 /demo/first-development**執行新增的多 worker 練習：

```bash
make
python3 parallel.py --workers 2 --jobs 100
python3 parallel.py --workers 3 --jobs 100
```

它真的啟動多個獨立 C 程序、分派全域 job_id、回收並逐筆比較單 worker 基準。運算仍是 worker.c 裡的簡單公式，**不是 eBNN 或硬體模擬**；它測的是 Linux thread／程序／管道的成本，不拿來推測板子能快幾倍。

多 worker 不保證更快，因為現在每件工作非常短。終端會列出各 worker 完成筆數與批次耗時；這批次時間不含啟動／關閉 worker，但包含主機排程開銷。先學正確分派與合併，再換真正的傳輸與推論。

## 依據與驗證範圍

以上是實作路線，沒有考場韌體／模型，也未做 RP2350 燒錄與實板效能測試。教學版多 worker 已在目前 Docker 驗證兩個／三個 worker 各 100 筆、全域 ID 與單 worker 結果比對；另檢查不同回覆速度、奇數批次、錯誤回報與資源回收。正式 eBNN 的輸入輸出與正確答案以題目為準。

來源：[Pico SDK 建置](https://github.com/raspberrypi/pico-sdk/blob/master/README.md)、[Arch Arm 編譯器](https://archlinux.org/packages/extra/x86_64/arm-none-eabi-gcc/)、[newlib](https://archlinux.org/packages/extra/any/arm-none-eabi-newlib/)、[pySerial 裝置辨識](https://pyserial.readthedocs.io/en/latest/tools.html#serial.tools.list_ports.ListPortInfo)、[eBNN 生成模型的實際宣告與暫存區](https://github.com/kunglab/ebnn/blob/70b2797229d573498651e9be6b07b2ecc3f573d2/c/tests/onelayer.h)、[Pico 多核心](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#group_pico_multicore)、[queue 原始碼](https://github.com/raspberrypi/pico-sdk/blob/master/src/common/pico_util/include/pico/util/queue.h)。

[回技術目錄](README.md) · [單板實際寫入步驟](RP2350與PicoSDK.md)
