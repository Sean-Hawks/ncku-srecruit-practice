# RP2350 與 Pico SDK：CPU 程式、PIO、DMA 怎麼接

**RP2350 是晶片；Pico 2 是使用它的開發板之一；Pico SDK 是寫 C/C++ 韌體時使用的工具與 API。** 板型不同，設定也可能不同。

```text
CPU 的 C/C++：設定、分工、處理資料
PIO 程式：獨立執行短指令序列
DMA：依設定搬運資料
```

三者不是同一個執行者。題目如果用這套，先找已有初始化與 CMake，不要重新造專案。

## 看到這些檔案，去哪裡找

| 檔案／符號 | 用途 |
|---|---|
| pico_sdk_import.cmake、pico_sdk_init() | 引入並初始化 SDK |
| main.c／main.cpp | CPU 程式入口 |
| .pio | PIO 指令程式 |
| .pio.h | pioasm／CMake 產生，通常不用手改 |
| hardware/pio.h | CPU 設定、收送 PIO 的 API |
| hardware/dma.h | DMA API |
| pico/multicore.h | 多核心啟動與通訊 API |
| .uf2／.elf | 可供部署／除錯的產物，依建置設定 |

## 建置：工具已預裝、題目 SDK 專案已完整時

在**板子專案根目錄**，若已配置 build 目錄：

```bash
cmake --build build -j 2
```

首次 configure 以題目指定板型與架構為準。**僅確定是 Pico 2、一般 SDK 專案時**可用：

```bash
cmake -S . -B build -DPICO_BOARD=pico2
cmake --build build -j 2
```

PICO_BOARD、SDK 路徑與工具鏈必須正確；若題目已設定 RISC-V 或其他板型，不用這條覆蓋。改板型／架構可用新的 build 目錄，避免混用舊 cache。

在支援且已設定 BOOTSEL/UF2 的情況，部署可能是進 BOOTSEL 後把正確 .uf2 複製到板子的磁碟；也可能指定 picotool／debug probe，**跟題目流程**，不是執行 `./app` 就在板子跑。

## CMake 常見要補的幾行

假設題目 executable target 已叫 app：

```cmake
target_link_libraries(app pico_stdlib hardware_pio hardware_dma pico_multicore)
pico_generate_pio_header(app ${CMAKE_CURRENT_LIST_DIR}/demo_pio.pio)
pico_add_extra_outputs(app)
```

只連實際需要的 library。上面是既有專案的補充，不是可獨立建置的完整 CMakeLists。

## 查 API 時先知道目的

| 要做什麼 | 搜尋 API |
|---|---|
| 一般啟動與時間 | stdio_init_all、sleep_ms、time_us_64 |
| 啟動另一核心 | multicore_launch_core1 |
| 跨核心共享資料保護 | SDK mutex／critical_section／queue API |
| PIO 配置與收送 | [PIO 速查](PIO.md) |
| DMA 設定與等待 | [資料搬運速查](資料搬運.md) |

多核心 FIFO 與 PIO TX/RX FIFO 是不同周邊，名字相似不能混用。跨核心傳數字還是指標也要分清；傳指標時，指向的資料須保持有效並同步。

## RISC-V 指令集需要時怎麼查

題目若提供組語／機器碼：先確認它使用哪個 ISA、32/64-bit、是否有壓縮指令，再讀 opcode、欄位與定址方式。不要把 PIO 的 out/in 當成 RISC-V CPU 指令。先看考場專案與指定手冊，現有材料沒有要求你先寫一整套 assembler。

來源：[Pico SDK 建置說明](https://github.com/raspberrypi/pico-sdk/blob/master/README.md)、[官方 PIO CMake 範例](https://github.com/raspberrypi/pico-examples/blob/master/pio/hello_pio/CMakeLists.txt)、[Pico 高階 API](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html)。

[回技術目錄](README.md)
