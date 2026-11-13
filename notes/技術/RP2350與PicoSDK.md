# RP2350 與 Pico SDK：讀懂題目的開發環境

[← 回考場筆記入口](../README.md) · [ISA](ISA與指令編碼.md) · [PIO](PIO.md)

## 第一性原理：程式必須變成目標硬體能執行的指令

主機上的原始碼只是文字。要在板子上執行，必須依目標 CPU 編譯成機器碼，把程式、資料與入口安排到正確記憶體位置，再透過指定方式寫入。CPU 要控制接腳、PIO 或 DMA，則需要按晶片規格操作周邊。**RP2350 提供執行與周邊硬體，Pico SDK 把許多硬體操作包成可呼叫的 C/C++ 介面。**

因此，完整路徑是「原始碼 → 目標工具鏈編譯／連結 → 韌體產物 → 寫入 → 板子執行」。CMake 記錄哪些檔案與函式庫參與建置；SDK 提供介面；兩者都不會讓尚未寫入的修改自動出現在板子上。

### 要解決什麼問題

把硬體能力接到可建置、可執行的程式，並避免每個專案都從暫存器細節重新寫起。遇到「編譯成功但行為沒變」，就要追查產物、板型、寫入對象與實際執行版本。參考 [SDK 硬體 API 說明](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html) 與 [Pico 韌體開發說明](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html)。

### 上機考可能怎麼出現

- **演練線索：**[section2-2](../../demo/section2-2/README.md) 連到 RP2350 PIO 文件；正式板型、CPU 架構與建置流程仍要讀現場專案。
- **可能任務（推測）：**在既有韌體中補一段演算法、啟用 PIO／DMA、連接輸入輸出，再建置並寫入指定板子。
- **具體例子：**改了 `.pio`，要讓組譯產生的標頭參與 C/C++ 建置，產生新韌體，再寫進板子；只儲存檔案或重啟主機 Python，板子仍可能跑舊版本。
- **動手順序：**讀 README／建置設定 → 確認入口與收送位置 → 改題目要求的處理處 → 建置／寫入 → 用固定測資核對回傳。

---

**RP2350 是執行韌體的微控制器；Pico SDK 是讓 C/C++ 程式操作它的工具庫。** 主機負責編譯、寫入、送測資，板子負責執行寫進去的程式。

演練參考資料連到 RP2350 的 PIO 文件；正式板型、CPU 架構與演算法仍依題目。本頁供看到 RP2350／Pico SDK 專案時查閱。

## 分清硬體與工具

RP2350 是微控制器，Pico 2 是搭載它的板子；與跑 Linux 的 Pi 4／5 不同。它可選雙 Arm Cortex-M33 或雙 Hazard3 RISC-V 的 CPU 組合，架構依題目設定。

Pico SDK 提供周邊 API；CMake 決定建置來源與連結庫；交叉編譯器產生板子能執行的指令。你熟悉的 PlatformIO Build／Upload／Monitor 對應同樣三步，工具不同但仍需分清。

## 拿到新專案，從哪個檔案找答案

| 要知道什麼 | 去哪找 |
|---|---|
| 怎麼建置／提交 | 題目 PDF、README、腳本 |
| 板型、CPU 架構 | CMake、preset、toolchain；常見關鍵字 PICO_BOARD／PICO_PLATFORM |
| 程式入口 | main、callback、題目標出的 TODO |
| 哪些 .c 真的會被編譯 | add_executable、target_sources |
| PIO 有沒有用上 | .pio、pico_generate_pio_header、主程式載入與啟用處 |
| 哪裡接收與回傳 | USB/UART／ROS 橋接程式、封包解析函式 |
| 輸入與答案格式 | 型別、元素數、byte 數、endian、工作 ID、參考測資 |

## 建置、寫入、執行是三件事

```text
主機原始碼 → 編譯／連結 → .elf／.uf2 → 寫入板子 → 板子執行
```

| 東西 | 是什麼 |
|---|---|
| .elf | 目標執行檔，可含符號／除錯資訊 |
| .uf2 | 供支援的 bootloader 寫入的韌體格式 |
| USB 文字 log | 韌體有啟用時才提供的觀察介面 |
| USB／UART 輸入 | 依題目協定收資料，和寫 UF2 不是同一步 |

**改主機檔案不會直接更新板子。** 韌體改動要重新建置、寫入；主機 Python 改動通常只需重啟該程序。Docker 掛載只同步檔案，不替你做編譯或燒錄。

## Pico SDK 常見命令形狀

優先使用題目指定的腳本／preset。以下 `<...>` 都要替換為現場值，使用已提供的環境：

```bash
cmake --build "<建置目錄>"
```

尚未配置，而且題目採一般 CMake 流程時，才需要 configure：

```bash
cmake -S "<來源目錄>" -B "<建置目錄>" -DPICO_BOARD="<實際板型>"
cmake --build "<建置目錄>"
```

題目指定的架構／toolchain 參數也要帶上；不能只靠 PICO_BOARD 猜全部。需要定位既有 SDK 時可查 `PICO_SDK_PATH`，但空白不代表專案沒透過其他方式引入 SDK。

## 寫入板子：依現場指定方式

| 方式 | 需要知道 |
|---|---|
| BOOTSEL／UF2 | Pico 2 按住 BOOTSEL 插入資料 USB 線，出現 RP2350 磁碟後放入對應 UF2；完成後重啟 |
| picotool／現成腳本 | 用題目指定的目標裝置、參數與產物 |
| Debug probe | 用提供的 server／probe／載入設定 |

多板時，確認每個序號／實際連線對應哪塊板子，不憑 ttyACM0 的數字猜。燒錄環境要能存取 USB，進了 ROS 容器不代表具備這些裝置或工具。

編譯／連結與改檔沒生效查 [操作清單](../操作清單.md)；輸入、運算、回傳的排錯查 [效能與除錯](效能量測與除錯.md)。

來源：[RP2350](https://www.raspberrypi.com/products/rp2350/)、[Pico SDK 建置說明](https://github.com/raspberrypi/pico-sdk/blob/master/README.md)、[官方 Pico／BOOTSEL 文件](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html)、[SDK API 分類](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html)。此頁為查閱流程，未做實板驗證。
