# ESP32：先辨識開發環境，再編譯燒錄

**ESP32 是晶片／開發板系列；ROS 2 是通訊框架。** 可以由電腦 ROS node 收資料，再透過題目提供的通訊程式交給板子運算。

```text
電腦程式 → USB/UART 或題目指定連線 → 板子韌體 → 結果回電腦
```

歷屆使用過 ESP32 不代表今年一定用。本 repo 沒有 ESP32 韌體，也沒安裝對應 SDK；以下在**題目提供的板子專案**使用。ESP32 不同型號的核心數與功能不同，不要假設全系列都雙核心。

## 第一件事：看專案檔案

| 看到什麼 | 可能的工具 | 找哪個入口 |
|---|---|---|
| sdkconfig、CMakeLists.txt 引入 ESP-IDF | ESP-IDF | `app_main()` |
| platformio.ini | PlatformIO，framework 要看 ini | src/、`setup/loop` 或 app_main |
| .ino | Arduino | `setup()`、`loop()` |

以題目 README 和建置腳本為準。不要把 `idf.py`、`pio run`、Arduino 指令混成同一套流程。

## ESP-IDF：已提供 SDK 與工具鏈時

在**ESP-IDF 專案根目錄**：

```bash
command -v idf.py
idf.py --version
idf.py build
```

找不到 idf.py，先依現場說明 source 已安裝 SDK 的 export.sh。不要為了查筆記另裝一套。

找裝置，在**連著板子的 Linux 主機**：

```bash
ls /dev/serial/by-id/
ls /dev/ttyUSB* /dev/ttyACM*
```

有些系統沒有 by-id。以下以實際裝置是 /dev/ttyUSB0 為例，**先確認這就是要寫入的板子**：

```bash
idf.py -p /dev/ttyUSB0 flash
idf.py -p /dev/ttyUSB0 monitor
```

flash 把程式寫進板子；monitor 看輸出。退出 IDF monitor：**Ctrl+]**。

<details>
<summary>只在題目要求時改目標／設定</summary>

```bash
idf.py menuconfig
idf.py set-target esp32
```

`set-target esp32` 只適合確定目標是經典 ESP32；esp32s3、esp32c3 等需指定自己的 target。它會重新設定建置環境，可能重建 sdkconfig，不要在已有正確設定的專案隨便跑。

</details>

## PlatformIO：只有 platformio.ini 專案才用

```bash
pio project config
pio run
pio device list
pio run -t upload --upload-port /dev/ttyUSB0
```

有多個 `[env:...]` 時，依 ini 選 `pio run -e 實際環境名`；上傳同樣指定該 `-e`。監看 baud 要跟韌體相同，例如韌體 115200 時：

```bash
pio device monitor -p /dev/ttyUSB0 -b 115200
```

**這裡的 `pio` 是 PlatformIO CLI，和 RP2350 的 PIO 硬體不同。**

## Arduino：使用題目指定板型

Arduino IDE 選對 Board 與 Port，再 Verify／Upload／Serial Monitor。若有預裝 CLI：

```bash
arduino-cli board list
arduino-cli core list
```

確認實際 FQBN 和 sketch 路徑後才用：

```bash
arduino-cli compile --fqbn <實際FQBN> <sketch目錄>
arduino-cli upload -p <實際port> --fqbn <實際FQBN> <sketch目錄>
```

## 程式怎麼分工

- `setup()`：Arduino 啟動時做一次設定；`loop()`：重複工作。
- `app_main()`：ESP-IDF 應用入口，可建立 FreeRTOS tasks。
- `xTaskCreate()` 建 task；`vTaskDelay(pdMS_TO_TICKS(10))` 讓 task 等待約 10ms，實際受 tick 與排程影響。
- 需要指定核心時才查 `xTaskCreatePinnedToCore()`，先確認板型與 SDK 設定。建立兩個 task 不保證更快。
- 共享資料用對應的 mutex／queue；跨核心時，單純關本核心中斷不足以保護另一核心。

## 自編實作題

先用題目既有的傳輸框架完成：電腦送 `request_id=1, x=21`，板子回 `request_id=1, y=43`，其中 `y=2*x+1`。再連續送 100 筆，檢查每個 ID 的答案與數量。

遇到問題：先確認燒錄的是新程式 → monitor 看啟動訊息 → 測一筆輸入 → 再接 ROS。串口忙碌時關掉自己另一個 monitor；權限錯先看 `ls -l` 與現場配置，不是先改成全員可寫。

來源：[ESP-IDF build/flash/monitor](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/start-project.html)、[IDF FreeRTOS](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html)、[PlatformIO run](https://docs.platformio.org/en/latest/core/userguide/cmd_run.html)、[Arduino CLI](https://docs.arduino.cc/arduino-cli/getting-started/)。

[回技術目錄](README.md) · [序列通訊](序列通訊與封包.md) · [平行運算](平行運算.md)
