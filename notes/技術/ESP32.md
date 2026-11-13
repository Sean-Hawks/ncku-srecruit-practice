# ESP32：歷屆延伸查閱

[← 回考場筆記入口](../README.md) · [延伸技術目錄](README.md#歷屆情境有出現才查)

**歷屆心得提過 ESP32，今年未確認使用。** 題目真的給 ESP32 專案時，才依這頁辨識工具與分工。

## 和 PlatformIO 的關係

ESP32 是晶片／板子系列；PlatformIO 是安排建置、上傳、監看的開發工具；ESP-IDF／Arduino 是可能使用的框架。使用 PlatformIO 時，真正框架看 platformio.ini 的 framework。

| 看到的檔案 | 常見工具 | 程式入口 |
|---|---|---|
| platformio.ini | PlatformIO | src/；入口依 framework |
| sdkconfig、引入 ESP-IDF 的 CMake | ESP-IDF | app_main() |
| .ino | Arduino | setup()、loop() |

`pio` CLI 是 PlatformIO，和 RP2350 的 PIO 硬體是不同名詞。不同 ESP32 型號的 ISA、核心數、周邊不同，不能一律當雙核心。

## 已提供環境時的命令表

在實際專案根目錄，使用與專案相符的一套；`<...>` 必須替換。

| 目的 | ESP-IDF | PlatformIO |
|---|---|---|
| 建置 | `idf.py build` | `pio run -e "<環境名>"` |
| 找串口 | 查 /dev/serial/by-id/ 等 | `pio device list` |
| 寫入指定板子 | `idf.py -p "<port>" flash` | `pio run -e "<環境名>" -t upload --upload-port "<port>"` |
| 監看 | `idf.py -p "<port>" monitor` | `pio device monitor -p "<port>" -b "<baud>"` |

ESP-IDF monitor 用 Ctrl+] 離開；其他 monitor 依其提示。只有一個 PlatformIO env 時，可依專案省略 -e。找不到 idf.py 先查現場 SDK 的 export 環境；不換一套 SDK。

Arduino IDE 對應 Verify／Upload／Serial Monitor，Board 與 Port 使用題目指定值。不要把 idf.py、pio run 混用在不相符的專案。

## 收送與運算怎麼安排

```text
主機 → 題目指定連線 → 收完整一筆 → 計算 → 回傳結果
```

ESP-IDF 的 FreeRTOS task 可安排不同工作。若需要把收訊與運算分開，可透過 queue 交接，先確認 buffer 所有權。`xTaskCreatePinnedToCore` 是指定核心的 API，須符合該型號與 SDK 設定；開兩個 task 不保證更快。

| 看到什麼 | 意思／注意 |
|---|---|
| setup／loop | Arduino 的一次初始化／重複工作 |
| app_main | ESP-IDF 應用入口 |
| xTaskCreate | 建立 FreeRTOS task |
| vTaskDelay | 讓出執行，延遲單位是 tick，常用 pdMS_TO_TICKS 轉換 |
| queue／mutex | 工作交接／保護共享資料 |

跨核心時，只關本核心中斷不足以保護另一核心的共享存取。演算法／模型若用共享暫存區，同時呼叫前要確認是否可重入。

來源：[ESP-IDF build／flash／monitor](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/start-project.html)、[IDF FreeRTOS](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html)、[PlatformIO run](https://docs.platformio.org/en/latest/core/userguide/cmd_run.html)。
