# 上機考技術速查

**不用從頭背。先找到正在做的工作，再進對應檔案複製指令。**

沒有板子、想先實作一輪：[第一次完整開發](../../demo/first-development/README.md)。用獨立 C 程序練 binary 收送，再接 ROS 2；不需要 PlatformIO，不模擬 MCU 硬體。

要把各技術串成作答流程：[拿到 RP2350 → 單板 eBNN → 量瓶頸 → 多板分派與雙核心](RP2350-eBNN從單板到多板.md)。包含環境、模型介面、封包、驗證與優化順序。

這是準備清單，不是考題預告。現有材料直接涉及 Docker、ROS 2、PIO、位元、執行緒與搬運；ESP32、eBNN 是歷屆延伸。考場依實際專案、板子與預裝工具操作。

## 按技術找

| 技術 | 什麼時候開這份 | 可以在哪裡練 |
|---|---|---|
| [Docker](Docker.md) | 進容器、查掛載、外面改了裡面沒變 | 現有 srecruit-demo |
| [ROS 2](ROS2.md) | 查 node/topic、收送訊息、改 callback | 現有 processor.py |
| [ESP32](ESP32.md) | 判斷 ESP-IDF／PlatformIO／Arduino、編譯燒錄 | 需對應專案、工具與實體板 |
| [eBNN](eBNN.md) | 接模型輸入輸出、理解推論、二值運算 | 本 repo 沒附模型；先練位元相似度 |
| [PIO](PIO.md) | out/in/push、FIFO、CPU 與 PIO 接線 | 現有 .pio 可閱讀；執行需 SDK／板子 |
| [平行運算](平行運算.md) | 分工、共享變數、鎖、join | 現有 pthread 範例與獨立加總題 |
| [資料搬運](資料搬運.md) | count、size、位址遞增、DMA | 現有 transfer-engine |
| [位元運算](位元運算.md) | mask、欄位替換、打包拆包 | 現有 demo_asm.c 與獨立實作題 |
| [C 記憶體與指標](C記憶體與指標.md) | sizeof、陣列、指標、memcpy | 一般 Linux C 編譯器 |
| [編譯與 Make](編譯與Make.md) | main.c、engine.c、engine.h 怎麼一起用 | 現有 Makefile |
| [Git 與提交](Git與提交.md) | 拉筆記、tracking 錯誤、提交前檢查 | 目前 Git repo |
| [序列通訊與封包](序列通訊與封包.md) | USB serial、文字／binary、拆包、request ID | Python 標準函式庫封包題；串口需板子 |
| [效能量測與除錯](效能量測與除錯.md) | 正確後怎麼變快、卡在哪、用什麼工具 | Linux；部分工具需已預裝 |
| [RP2350 與 Pico SDK](RP2350與PicoSDK.md) | 插板子、編譯、寫 UF2、USB 看結果 | 附完整自編專案；需 SDK、工具鏈與板子 |

## 按症狀找

| 現在遇到什麼 | 先看 |
|---|---|
| 改程式沒生效 | Docker → 存檔、掛載、重啟／重編 |
| 沒收到結果 | ROS 2 → 輸入輸出觀察；PIO → FIFO 等待 |
| 數值看起來被顛倒 | 序列通訊 → byte 順序；位元運算 → shift/mask |
| 結果偶爾錯 | 平行運算 → 共享資料；C → 越界／生命週期 |
| 結果對，但速度慢 | 效能量測 → 分段計時；再選平行／搬運 |
| 不知道用哪個編譯指令 | 編譯與 Make → 先辨識專案 |

## 先做這個整合練習

1. 用 [ROS 2 的打包整數題](ROS2.md#實作題收到打包整數回傳兩欄位的和)，讓 `773 → 8`。
2. 用 CLI 輸入 `0、255、256、65535`，確認結果。
3. 用 [位元運算](位元運算.md) 的 C 程式練同樣拆欄位。
4. 再讀 PIO，指出相同資料經過 OSR、X、ISR、RX FIFO 的位置。

## 指令標示

- **主機**：archlinux 終端機。
- **容器**：已進入 srecruit-demo 的終端機。
- **板子專案**：進入題目給的韌體專案根目錄；不是任意貼在 /demo 就能用。
- `<...>`：替換成題目實際值後再執行；不要連角括號一起貼。

硬體部分提供設定與 API 速查，未在本 repo 的容器中燒錄驗證。每份末尾有官方來源；stable/master 文件可能比考場版本新，API 以現場 SDK 為準。

## 已驗證的範例（2026-11-13）

- 五支完整 C 程式：位元比較、欄位打包、記憶體複製、pthread 分工加總、64 筆 16-bit 填值，皆實際編譯執行並通過內附 assert。
- Python 封包：8-byte 打包與拆包，含 ID 與數值，實際執行通過。
- ROS 2：在既有容器的獨立 ROS domain 驗證實際收送，`773→8、0→0、255→255、256→1、65535→510`；沒有改掉目前 processor 的 `3*x+1`。
- 新增多 worker：第一次開發專案的兩個／三個 C worker 各完成 100 筆，按全域工作 ID 還原並與單 worker 比較；不代表 eBNN 或板子效能。
- ESP32、Pico、PIO 與 DMA 硬體 API 以官方文件核對；沒有實體板燒錄與硬體效能測試。

[回練習入口](../README.md) · [原有操作清單](../操作清單.md) · [程式註解導讀](../程式註解導讀.md)
