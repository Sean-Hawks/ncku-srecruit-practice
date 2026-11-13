# 第一次完整開發：沒有板子也能先跑資料往返（自編）

> 舊程式／練習留存。考場查閱從 [考場筆記入口](../../notes/README.md) 開始。


**目標：你親手完成編譯、傳資料、收結果、修改、重編，再接 ROS 2。** 不用 PlatformIO，不需要安裝新套件；用現在已有的 Docker 環境。

C worker 是獨立 Linux 程序，有自己的記憶體。本例用管道取代實體 USB 連線；不模擬 ESP32／RP2350 指令集、PIO、USB 時序或硬體效能，也沒有真的燒錄。

```text
電腦端 Python → 八-byte binary 請求 → C worker 計算
電腦端 Python ← 八-byte binary 回覆 ← C worker
```

## 這一輪只讀三個位置

| 檔案 | 先看哪裡 |
|---|---|
| [worker.c](worker.c) | compute()：你要改的公式 |
| [host.py](host.py) | exchange()：送工作、收結果，第一輪不用背 |
| [bridge.py](bridge.py) | on_msg()：把 ROS 輸入交給 worker，再發布答案 |

## 第一階段：15 分鐘，先跑通原版

在**主機**：

```bash
docker start srecruit-demo
docker exec -it srecruit-demo bash
```

進入後，以下在**容器**：

```bash
cd /demo/first-development
make
python3 host.py 21
```

預期：

```text
id=1 input=21 result=43
```

- make：把 worker.c 編成 bin/worker。
- host.py：開啟 worker，送 21，等它回 43；不是自己算答案。
- id=1：這個 Python 程序送出的第一份工作編號。

不要直接 python3 worker.c，也不要只執行 bin/worker：worker 在等 binary 輸入，單獨開著沒有文字輸出是正常的。

接著送多筆，使用同一個 worker：

```bash
python3 host.py 0 1 21 1000000
make check
```

預期結果依序是 `1、3、43、2000001`。make check 檢查的是原規格 `2*x+1`。

## 第二階段：15 分鐘，自己改公式，體驗重編

在**主機另一個終端機**：

```bash
cd ~/Development/ncku-srecruit-practice
nvim demo/first-development/worker.c
```

compute 裡只改一行：

```c
return input + 1u;
```

用 `:w` 存檔。回**容器**，先故意不重編：

```bash
python3 host.py 21
```

仍是 **43**，因為你改的是原始碼，bin/worker 還是舊版本。

再：

```bash
make
python3 host.py 21
```

現在應是 **22**。這次你真的經歷了「改原始碼 → 編譯 → 新程式執行」。主機存檔會掛進容器，但不會自動編譯 C。

此時 make check 會失敗：它還在驗舊規格 2*x+1。失敗是合理的，先不改測試；把 compute 恢復成：

```c
return input * 2u + 1u;
```

存檔後 make check 應再次通過。

## 第三階段：20 分鐘，把 ROS 接上來

你自己的舊 bridge 若有啟動，先 Ctrl-C 停掉，避免兩份同時回覆。三個終端機都先在主機執行：

```bash
docker exec -it srecruit-demo bash
```

每個容器 shell 都先：

```bash
source /opt/ros/jazzy/setup.bash
cd /demo/first-development
```

**A：編譯並開橋接程式，保持執行。**

```bash
make
python3 bridge.py
```

**B：先開輸出觀察，保持執行。**

```bash
ros2 topic echo /practice/out
```

**C：送一筆。**

```bash
ros2 topic pub --once /practice/in std_msgs/msg/Int32 '{data: 21}'
```

B 應收到 data: 43。這次的路徑是：

```text
ros2 pub → /practice/in → bridge.py
                             ↓ binary
                         C worker：2*x+1
                             ↓ binary
ros2 echo ← /practice/out ← bridge.py
```

有兩段通訊：ROS topic 與 Python/C 的 binary 管道，不要把它們當同一個通道。

## 第四階段：10 分鐘，找一次問題

在 C 故意送到不同名字：

```bash
ros2 topic pub --once /practice/wrong std_msgs/msg/Int32 '{data: 21}'
```

可能停在等訂閱者；用 Ctrl-C 結束。B 不會有對應輸出。接著查：

```bash
ros2 node info /development_bridge
ros2 topic list -t
ros2 topic info /practice/in
```

找出 bridge 訂閱的是 /practice/in，再改回正確發布命令，驗證 43。

改 C 公式後要更新 ROS 裡的 worker：**存檔 → A 的 bridge Ctrl-C → make → 重開 bridge → C 重送**。正在執行的舊程序不會自動換成新二進位檔。

## 第五階段：看懂八-byte 封包，先不用背收送程式

每個請求、回覆都固定八 bytes：

| bytes | 請求 | 回覆 |
|---|---|---|
| 0..3 | 工作 ID | 相同工作 ID |
| 4..7 | input | answer |

型別都是 unsigned 32-bit，little-endian。輸入限制 0..1000000，答案放得進 ROS Int32。

host.py 用 struct.pack/unpack；worker.c 用 load_le32/store_le32。未來換真板子，傳輸端點改成串口／指定介面，兩邊仍要約好封包格式；不能把 Linux stdin/stdout 程式直接燒進板子。

這個 baseline 一次送一件、等一個答案；bridge 等 worker 時會占住 callback。先確認功能，再練佇列、非同步與兩個 worker，不把這版當高吞吐量設計。

## 做到這裡就算完成第一次開發

- 你親手把 21→43 改成 21→22，再恢復。
- 你看過不重編為什麼還跑舊結果。
- 你從 ROS 送資料，真的經過另一個程序的 C 計算才拿到答案。
- 你找出一次接錯 topic 的問題。

## 下一輪：兩個／三個 worker 分工

完成上面步驟後，在**容器的 /demo/first-development**：

```bash
make
python3 parallel.py --workers 2 --jobs 100
python3 parallel.py --workers 3 --jobs 100
```

[parallel.py](parallel.py) 會先跑單 worker 基準，再開多個 C worker：哪個空了就拿下一筆，回傳帶全域工作 ID，最後逐筆比較答案。每個連線只由一條 thread 使用。worker.c 仍跑你目前編譯的公式，這個命令不會改它。

應看到 PASS、各 worker 完成筆數與耗時。回傳順序不必一致，還原後的答案必須一致；工作很短時，多 worker 可能更慢。耗時不含 worker 程序建立／關閉，但包含主機 thread 排程，不能當成板子或 eBNN 的效能。

真板子與 eBNN 接法看 [完整作答流程](../../notes/技術/RP2350-eBNN從單板到多板.md)。[回技術目錄](../../notes/技術/README.md)

## 已驗證（2026-11-13）

在既有容器實際執行：make check、binary 往返、工作 ID、邊界值與不完整封包；在副本驗證「沒重編仍回 43，重編後回 22」。ROS 在獨立 domain 完成 `0→1、1→3、21→43、99→199、1000000→2000001`，並驗證非法輸入不發布答案、結束後回收 worker。原本 section2-1 的 processor 沒有修改。

多 worker：兩個與三個 C worker 各跑 100 筆，與單 worker 逐筆一致；另驗證全域 ID、奇數筆數、工作少於 worker 數，以及替身連線的速度差、錯誤傳遞與清理。
