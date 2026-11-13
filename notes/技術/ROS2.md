# ROS 2：把接收、處理、發布接起來

**node 是執行功能的單位；topic 是訊息通道；message type 規定資料格式。** 一個程式可以有多個 node，node 名稱不等於檔名。

```text
publisher.py → /number → processor.py → /double → echo
                            收到才呼叫 on_msg()
```

先讀 [processor.py](../../demo/section2-1/processor.py)。目前公式是 `3*x+1`，不是原版的 `2*x`；/double 只是名稱。

## 環境

以下都在**容器**。每個終端機先在主機執行 `docker exec -it srecruit-demo bash`。

```bash
source /opt/ros/jazzy/setup.bash
cd /demo/section2-1
```

考場 ROS 版本可能不同，先看 `/opt/ros` 與題目文件。找不到 `ros2`，先查環境是否 source，不是先改 Python。

## 最常用指令

| 想查什麼 | 指令 |
|---|---|
| 哪些 node 在跑 | `ros2 node list` |
| processor 的接線 | `ros2 node info /processor` |
| 哪些 topic、什麼型別 | `ros2 topic list -t` |
| 兩端數量、詳細 QoS | `ros2 topic info /number --verbose` |
| Int32 裡有什麼欄位 | `ros2 interface show std_msgs/msg/Int32` |
| 輸入有沒有資料 | `ros2 topic echo /number` |
| 輸出值 | `ros2 topic echo /double` |
| 發布頻率 | `ros2 topic hz /number` |

`node list` 查執行者；`topic list` 查通道。通道存在，不代表現在有資料。

`node info /processor`：Subscribers 是它接收的來源；Publishers 是它發布的去處。先關注 /number 與 /double；/rosout、parameter_events 與參數服務是附帶功能。

## 固定輸入測試：三個終端機

停止你自己的隨機 publisher，避免干擾；不要同時跑兩份 processor。

**A，處理：**

```bash
python3 processor.py
```

**B，先啟動觀察，保持執行：**

```bash
ros2 topic echo /double
```

**C，送一筆：**

```bash
ros2 topic pub --once /number std_msgs/msg/Int32 '{data: 21}'
```

目前應收到 `data: 64`。沒輸入時 processor 安靜等待是正常的。修改 Python 後：`:w` → A 按 Ctrl-C → 重跑 → C 重送。

`echo /number` 自己就是接收者，不需要 processor。多個接收者都可以收到同一筆，並不是誰先拿就把別人的資料取走。

## 對照 Python 這幾行

```python
self.sub = self.create_subscription(Int32, INPUT_TOPIC, self.on_msg, 10)
self.pub = self.create_publisher(Int32, OUTPUT_TOPIC, 10)

def on_msg(self, received):
    out = Int32()                       # 建一個要送出的訊息物件
    out.data = received.data * 3 + 1    # .data 是物件的整數欄位
    self.pub.publish(out)              # self.pub 已記得目的 topic
```

`rclpy.spin(node)` 讓 executor 持續處理事件，收到訊息才呼叫 callback。這種寫法的 `10` 是訊息歷史深度；不是十秒或乘十，也不是無條件保證每筆都收到。

## 實作題：收到打包整數，回傳兩欄位的和

自編練習，輸入限 0..65535。bits 15..8 是 A，bits 7..0 是 B。**只替換 on_msg 裡的計算段，保留原本 print 與 publish：**

```python
out = Int32()
packed = received.data
a = (packed >> 8) & 0xFF
b = packed & 0xFF
out.data = a + b
self.pub.publish(out)
```

| C 終端機送入 data | 十六進位 | B 應收到 |
|---|---|---|
| 773 | 0x0305 | 8 |
| 0 | 0x0000 | 0 |
| 255 | 0x00FF | 255 |
| 256 | 0x0100 | 1 |
| 65535 | 0xFFFF | 510 |

例如：

```bash
ros2 topic pub --once /number std_msgs/msg/Int32 '{data: 773}'
```

做完要回原公式，就把計算段換回 `out.data = received.data * 3 + 1`。

## 沒結果時，照順序縮小範圍

1. `node list`：程式有跑嗎？
2. `node info`：發送、接收的 topic 完全相同嗎？/number 與 /numbers 不同。
3. `topic list -t`、`interface show`：型別、欄位一致嗎？
4. `echo /number`：輸入到了嗎？有輸入再查 callback、輸出。
5. `topic info --verbose`：是否有不相容 QoS？不同終端的 ROS_DOMAIN_ID 是否相同？

QoS 不要盲改：sensor 類 topic 可能使用 best effort；需對照實際發布者。例如發布者是 best effort 時，可試 `ros2 topic echo /實際topic --qos-reliability best_effort`。兩邊還須在可互相發現的網路環境。

<details>
<summary>題目如果給 ROS package，而非直接跑 .py</summary>

在含 src/ 的 ROS workspace 根目錄，且工具已預裝：

```bash
colcon build --packages-select <package名稱>
source install/setup.bash
ros2 run <package名稱> <executable名稱>
ros2 launch <package名稱> <launch檔名>
```

上面先替換 `<...>`。現有 demo 是直接 `python3 processor.py`，沒有對應的 colcon package。

有參數介面才用 `ros2 param list /node名稱`、`ros2 param get /node名稱 參數名`；目前公式沒有做成參數。

</details>

來源：[ROS 2 Jazzy topic 教學](https://github.com/ros2/ros2_documentation/blob/jazzy/source/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Topics/Understanding-ROS2-Topics.rst)、[rclpy Node 原始碼](https://github.com/ros2/rclpy/blob/jazzy/rclpy/rclpy/node.py)、[QoS 相容性](https://github.com/ros2/ros2_documentation/blob/jazzy/source/Concepts/Intermediate/About-Quality-of-Service-Settings.rst)。

[回技術目錄](README.md) · [接著練位元](位元運算.md)
