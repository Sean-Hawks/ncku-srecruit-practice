# RP2350 與 Pico SDK：讀懂題目的開發環境

[← 回考場筆記入口](../README.md) · [ISA](ISA與指令編碼.md) · [PIO](PIO.md)

**RP2350 是執行韌體的微控制器；Pico SDK 是讓 C/C++ 程式操作它的工具庫。** 主機負責編譯、寫入、送測資，板子負責執行寫進去的程式。

演練參考資料連到 RP2350 的 PIO 文件；正式板型、CPU 架構與演算法仍依題目。本頁供看到 RP2350／Pico SDK 專案時查閱。

## 和你用過的東西怎麼對照

| 名稱 | 角色 | 你會碰到什麼 |
|---|---|---|
| Raspberry Pi 4／5 等單板電腦 | 能跑 Linux 的電腦 | 登入 OS、執行程序 |
| RP2350／搭載它的 Pico 2 | 微控制器／開發板 | 編譯韌體、寫入板子、操作周邊 |
| ESP32 系列 | 另一種微控制器系列 | 用其 SDK／框架；功能與核心數看型號 |
| Pico SDK | RP 系列的 C/C++ 函式庫 | pio、dma、multicore、stdio 等 API |
| CMake | 建置規則工具 | 決定來源檔、SDK、目標與連結庫 |
| 交叉編譯器 | 把主機上的 C/C++ 轉成板子 CPU 的指令 | 目標為 Arm 或 RISC-V，不能直接拿主機 cc 代替 |
| PlatformIO | 整合建置／上傳／監看的開發工具 | 你以前按的 Build、Upload、Monitor 仍是分開的步驟 |

RP2350 可選雙 Arm Cortex-M33 或雙 Hazard3 RISC-V 核心的組合，不是四顆核心一起拿來跑。實際架構看題目、工具鏈與專案設定。

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

最有用的一行筆記是：**實際輸入位置 → 處理函式 → 實際輸出位置**。這行適用於推論、排序、校驗或資料搬運，不必預設 eBNN。

在題目根目錄，工具已提供時可查：

```bash
rg --files -g 'README*' -g 'CMakeLists.txt' -g 'CMakePresets.json' -g 'Makefile' -g '*.pio'
rg -n 'main[[:space:]]*\(|TODO|add_executable|target_sources|PICO_BOARD|PICO_PLATFORM' .
```

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

| 訊息 | 查哪裡 |
|---|---|
| 編譯器找不到 | 指定 shell／container、工具鏈環境 |
| pico/xxx.h 找不到 | SDK 路徑與 include 設定 |
| undefined reference | 實作來源、target_link_libraries、SDK 模組 |
| 新增 .c 卻沒效果 | 有沒有加入 target_sources／實際 executable |
| 改架構後 build 很怪 | 原建置目錄的設定快取是否仍是另一架構 |

## 寫入板子：依現場指定方式

| 方式 | 需要知道 |
|---|---|
| BOOTSEL／UF2 | Pico 2 按住 BOOTSEL 插入資料 USB 線，出現 RP2350 磁碟後放入對應 UF2；完成後重啟 |
| picotool／現成腳本 | 用題目指定的目標裝置、參數與產物 |
| Debug probe | 用提供的 server／probe／載入設定 |

多板時，確認每個序號／實際連線對應哪塊板子，不憑 ttyACM0 的數字猜。燒錄環境要能存取 USB，進了 ROS 容器不代表具備這些裝置或工具。

## 程式已執行，卻沒有結果

| 停在哪一層 | 查什麼 |
|---|---|
| 入口都沒到 | 是否寫入正確產物、啟動方式與初始化 |
| 等輸入 | 來源有送？型別、長度、封包邊界對嗎？ |
| 運算數字錯 | 前處理、位元順序、有效範圍、處理函式 |
| 算對但回不來 | 回傳格式、job_id、輸出佇列、阻塞等待 |
| 偶爾錯 | 越界、共享資料、buffer 提早重用、沒等完成 |
| 正確但慢 | [分段計時](效能量測與除錯.md)，確認瓶頸才選多核心／DMA／PIO |

正式 binary 資料與 debug 文字分開，避免 printf 被當成答案解析。串口只讓安排好的接收程式讀，額外的 monitor 可能讀走資料。

來源：[RP2350](https://www.raspberrypi.com/products/rp2350/)、[Pico SDK 建置說明](https://github.com/raspberrypi/pico-sdk/blob/master/README.md)、[官方 Pico／BOOTSEL 文件](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html)、[SDK API 分類](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html)。此頁為查閱流程，未做實板驗證。
