# RP2350 與 Pico SDK：讀懂題目的開發環境

[← 回考場筆記入口](../README.md) · [ISA](ISA與指令編碼.md) · [PIO](PIO.md)

## 從 GitLab 到板子：先看這條路

**GitLab 放原始碼；考場電腦下載、編譯；板子執行寫入的韌體。** 以題目給微控制器的 C/C++ 專案為前提，通常是這個流程：

```text
GitLab → clone 到電腦 → 找板端專案 → build 產生韌體 → flash／upload 到板子
                         主機程式留在電腦 ←→ USB／UART等指定連線 ←→ 板子
```

你以前用 PlatformIO 按 Build／Upload，就是在做其中兩步。**git push 是交回 GitLab，並不會把程式寫進板子。**

### 1 先把 repo 下載到考場電腦

在 GitLab 專案按 Code，複製提供的 Clone URL；登入方式、分支依現場說明。以下 `<...>` 都要替換：

```bash
git clone "<從GitLab複製的網址>"
cd "<下載後的專案資料夾>"
```

若題目已把專案放在電腦／容器，直接用指定目錄即可。先讀 README、題目 PDF，找 **在哪個環境建置、哪個板型、怎麼寫入、怎麼測試**；有現成 build／flash 腳本就讀並沿用。

### 2 分清哪些在電腦、哪些在板子

假設題目長這樣，名稱只是例子：

```text
problem/
├── README.md
├── host/client.py          主機端：送測資、接答案
└── firmware/
    ├── main.c              板端：初始化、收資料、計算、回結果
    └── CMakeLists.txt      指定板端來源、SDK與工具鏈
```

**判斷依據是 README 的啟動／建置目標，不是只看副檔名。** 主機也能有 main.c，ESP-IDF也用CMake；看到 CMakeLists.txt 不代表一定是 Pico SDK。

你原本的 processor.py、演練 engine.c 是在 Linux／Docker 執行的程式，不能直接當成 RP2350 韌體燒入。題目要它們接板子時，需有相符的板端專案與通訊介面。

### 3 先用提供的流程 build，再改指定處

先確認原專案能否產生預期韌體，建立「環境與工具鏈可用」的基準。原版若有 TODO，功能還沒通過不一定是建置問題。

修改通常在 **收到資料 → 處理函式 → 回傳** 之中的指定位置，不必把給定的驅動、通訊和模型全部重寫。

例如上面的 firmware/ **確定使用 Pico SDK、板子確定為 Pico 2，且現場 SDK／工具鏈已備好**，一般 CMake 命令可能長這樣：

```bash
cmake -S firmware -B build -DPICO_BOARD=pico2
cmake --build build
```

這是在假想 problem/ 根目錄執行的示意，正式參數照 README；有 preset、指定架構或建置腳本時依原流程。建置輸出或 README 會指出產物，例如 `build/app.uf2`；app 是目標名例子，不是固定答案。

### 4 把產物寫進指定板子

| 題目專案／方式 | 把程式送進板子的方法 |
|---|---|
| Pico 2，採 BOOTSEL／UF2 | 按住 BOOTSEL 插入資料 USB 線，放開後出現 RP2350 磁碟；把對應 UF2 複製進去，寫入完成通常會自動重新啟動 |
| Pico SDK，採 picotool／probe／腳本 | 按題目指定的產物與裝置選項寫入 |
| ESP-IDF | 在實際專案用 `idf.py -p "<實際port>" flash` |
| PlatformIO | 在對應專案用 `pio run -e "<env>" -t upload --upload-port "<實際port>"` |

BOOTSEL磁碟是寫入入口：複製 UF2 時，板子的寫入機制把內容存進 Flash；不是把整個 repo 當一般資料夾搬進去。Flash保存韌體，板子重啟後按其啟動流程執行。

寫入環境要看得到USB。若在容器編譯、主機才看得到板子，就由題目提供的方式取出產物並寫入；bind mount 可共享檔案，但不等於USB也已接進容器。

### 5 再由主機送測資，確認板子回結果

```text
主機 client／ROS橋接程式 → 題目指定協定 → 板端接收 → 算法 → 回傳 → 主機核對
```

UF2寫入與平常送測資是兩種操作。韌體啟用USB串口時可能出現 ttyACM 裝置；也可能使用其他介面，以題目為準。host程式用哪個topic／port、封包格式和啟動命令，都從README或程式查。

同一個串口不要同時讓monitor與正式接收程式讀，避免資料被讀走。多板要先對清序號／裝置與實際板子，再分派帶 job_id 的工作。

### 6 改完以後，更新哪一邊

| 你改了什麼 | 更新流程 |
|---|---|
| 主機 Python／ROS程式 | 依專案方式建置（若需要），重啟主機程序 |
| 板子 C/C++／.pio | 存檔→重新build→flash／upload→重新連線→核對測資 |
| 要交回考試 GitLab | 依規定 git add／commit／push，等待指定評分結果 |

本機編譯成功、板子執行新韌體、GitLab收到作答是三個需要分開確認的結果。

### 不知道 repo 哪一份該燒：貼給考場模型

```text
以下是題目README、目錄與建置設定：[貼內容]
請分出主機程式與板端韌體，指出判斷依據。
找出現成建置命令、產物、寫入方式及送測資方法。
每步列出執行位置與成功時應看到什麼。
板型、架構、分支、port缺資訊就列待確認，不要猜或另建專案。
```

來源：[GitLab clone](https://docs.gitlab.com/topics/git/clone/)、[Pico SDK建置](https://github.com/raspberrypi/pico-sdk/blob/master/README.md)、[BOOTSEL／UF2](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html)、[ESP-IDF flash](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/start-project.html)、[PlatformIO upload](https://docs.platformio.org/en/latest/core/userguide/cmd_run.html)。

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
