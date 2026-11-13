# RP2350 實際操作小專案（自編）

> 舊程式／練習留存。考場查閱從 [考場筆記入口](../../notes/README.md) 開始。


**這是新增的教學專案，不是官方考題。需要 Pico 2 或設定相符的 RP2350 開發板。**

操作依據：[RP2350 新題目的通用流程](../../notes/技術/RP2350與PicoSDK.md)，文末有本專案的參數與預期結果對照。

| 檔案 | 你要改什麼 |
|---|---|
| [main.c](main.c) | CPU 計算、固定輸入、設定 PIO、從 FIFO 讀結果 |
| [echo.pio](echo.pio) | PIO 每一筆資料怎麼處理 |
| [CMakeLists.txt](CMakeLists.txt) | 交叉編譯、產生 .pio.h、啟用 USB printf、輸出 UF2 |

預期 USB 輸出每秒重複：

```text
CPU: input=21 result=43
PIO: sent=21, waiting for RX FIFO...
PIO: received=21
```

main.c 的乘二加一由 CPU 做；echo.pio 只搬資料，所以 PIO 回 21。電腦只看文字輸出，此專案沒有接 ROS，也沒有從 USB 接收任意數字。

現有 srecruit-demo 沒有 ARM 交叉編譯器，不是進去直接 make 就能編。使用已備妥 Pico SDK／工具鏈的環境；考場依題目提供的專案、板型與建置方式操作。

驗證：原版與固定回 7 的 PIO 程式已用官方 pioasm 組譯通過；尚未完整交叉編譯與實板驗證，以上為預期輸出。
