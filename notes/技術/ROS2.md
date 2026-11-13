# ROS 2：程式之間怎麼傳資料

[← 回考場筆記入口](../README.md) · [命令集中查](../操作清單.md)

## 第一性原理：不同程式要交換資料，必須先約定通道與格式

把收資料、計算、顯示全部寫在同一個程式裡，彼此可以直接呼叫函式；拆成不同程序或不同電腦後，就不能靠一個普通變數把資料自動交過去。雙方必須約定「在哪裡交換、資料長什麼樣、如何接收」。**ROS 2 的 topic 通訊把這件事整理成具名稱、具訊息型別的發布／訂閱介面。**

發布者把訊息送到指定 topic，訂閱者透過相同 topic 名稱、相符型別與相容通訊設定接收，再由執行器安排 callback。雙方不必把對方的運算程式寫進自己裡面，因此能替換處理節點，也能另外加入 `echo` 觀察資料。通道只負責交換訊息，計算公式仍由 callback 決定。

### 要解決什麼問題

讓多個功能模組可以各自執行、交換有明確格式的資料，並能從外部觀察接線與訊息。這也說明為什麼「node 還在」不代表「答案有回來」：接線、資料到達、計算、回傳是不同步驟。概念依據見 [ROS 2 官方 Topics 說明](https://github.com/ros2/ros2_documentation/blob/jazzy/source/Concepts/Basic/About-Topics.rst)。

### 上機考可能怎麼出現

- **演練已出現：**[section2-1](../../demo/section2-1/README.md) 用 CLI 查 node、topic、型別、頻率，並送已知輸入檢查 processor；官方示範公式是 `2*x`。
- **可能任務（推測）：**訂閱題目輸入 → 拆欄位／呼叫演算法或板子 → 發布指定格式的結果。也可能要求找出 topic 拼字、型別或 QoS 問題。
- **具體例子：**本 repo 已把公式改成 `3*x+1`，所以輸入 21 應回 64。若 `/number` 有 21、`/double` 沒資料，就沿 callback 與 publish 查；`echo` 本身不會替你計算。
- **動手順序：**先查名稱與型別 → 用已知值測輸入／輸出 → 找 callback 的處理位置 → 再考慮耗時運算與收訊是否需要分工。

---

**兩個程式的變數不會自己互通。ROS 2 讓它們依約定的 topic 名稱、訊息型別交換資料。**

## 三個名詞與你問過的差別

| 名詞 | 意思 | 本 repo 例子 |
|---|---|---|
| node | ROS 裡執行某項功能的單位 | /publisher、/processor |
| topic | 訊息通道 | /number、/double |
| message type | 通道上的資料格式 | std_msgs/msg/Int32，含 data 欄位 |

```text
publisher.py → /number → processor.py 的 on_msg() → /double → echo
```

- `node list` 列執行者；`topic list` 列通道。
- `python3 processor.py` 啟動你的處理程式：訂閱輸入、計算、發布結果。
- `ros2 topic echo /double` 啟動觀察者：接收並印出 /double，沒有幫你跑 processor 的計算。
- `echo /number` 可以直接收到 publisher 的輸入，不需要 processor。多個訂閱者可各自收到訊息。
- processor 沒輸入時等待是正常的；`rclpy.spin` 持續處理事件，收到訊息才叫 callback。

## /double 的名稱不決定計算

對照 [processor.py](../../demo/section2-1/processor.py)：

```python
self.pub = self.create_publisher(Int32, OUTPUT_TOPIC, 10)
self.sub = self.create_subscription(Int32, INPUT_TOPIC, self.on_msg, 10)
# OUTPUT_TOPIC='/double'；INPUT_TOPIC='/number'

def on_msg(self, received):
    out = Int32()                      # 建要送出的訊息
    out.data = received.data * 3 + 1   # 這行才決定算法
    self.pub.publish(out)             # pub 已經記得送往 /double
```

| 哪裡改變 | 會發生什麼 |
|---|---|
| 計算改成 `received.data * 2` | 輸入 21 → 42，topic 名稱照設定 |
| 本 repo 現值 `received.data * 3 + 1` | 輸入 21 → 64 |
| 訂閱 /number 改成 /numbers | 發到 /number 的資料接不到 |
| 輸出 topic 改名 | echo 也要跟著看新名稱 |

`10` 是這種 API 寫法的訊息歷史深度，不是 10 Hz、十秒或乘十。`.data` 是訊息的整數欄位；不是整個訊息物件。

## 怎麼讀 node info

```text
Subscribers:
  /number: std_msgs/msg/Int32
Publishers:
  /double: std_msgs/msg/Int32
```

意思是「這個 node 從 /number 接收，向 /double 發布，兩邊格式都是 Int32」。這只證明接線登記，**不證明已有資料通過**。

`/rosout`、`/parameter_events`、各種參數 Service Servers 是常見附帶介面；查本例資料流先找 /number、/double。Service 是請求／回應介面，與持續發布的 topic 分開看。

## 查資料在哪一段斷掉

以下名稱是本 repo 範例，正式題目替換成其名稱。

| 目的 | 命令 | 如何解讀 |
|---|---|---|
| 程式有登記 node 嗎 | `ros2 node list` | 沒看到就查啟動、環境與錯誤 |
| 接收／輸出去哪裡 | `ros2 node info /processor` | 名稱、型別是否符合題目 |
| 所有通道與型別 | `ros2 topic list -t` | /number 與 /numbers 是不同通道 |
| 端點數與 QoS | `ros2 topic info /number --verbose` | 有 publisher/subscriber，QoS 是否相容 |
| 訊息有哪些欄位 | `ros2 interface show std_msgs/msg/Int32` | 例如 `int32 data` |
| 輸入是否真的到達 | `ros2 topic echo /number` | 有資料再往 callback、輸出查 |
| 輸出是否真的到達 | `ros2 topic echo /double` | echo 本身也會增加訂閱者 |
| 觀察收到的頻率 | `ros2 topic hz /number` | 不等於整條路的處理延遲 |
| 發一筆已知輸入 | `ros2 topic pub --once /number std_msgs/msg/Int32 '{data: 21}'` | 接收者應先啟動；隨機來源同時跑會混入其他值 |

## 明明都在跑，卻互相看不到

| 檢查 | 原因 |
|---|---|
| 每個 shell 都有載入同一套 ROS 環境 | 找不到 ros2／package 通常先查 source |
| `ROS_DOMAIN_ID` 是否相同 | 不同 domain 通常不互相發現 |
| topic 完整名稱與 type | namespace、remap 都可能改名 |
| QoS 的 reliability／durability | 有端點不保證相容 |
| 容器／主機的網路 | discovery 需要可互通的環境 |

發布者是 best effort 時，觀察可使用：`ros2 topic echo /實際topic --qos-reliability best_effort`。先看 `topic info --verbose`，不要盲改 QoS。

## .py 與 ROS package 的啟動方式

| 專案形式 | 對應方式 |
|---|---|
| 本 repo 的獨立 Python 檔 | `python3 processor.py`，改檔後重啟 |
| 題目提供 package／workspace | 依 README 用 colcon build，再 source install/setup.bash、ros2 run／launch |

參數、套件名、launch 檔不是由 topic 名稱推導。容器入口與 source 命令見 [操作清單](../操作清單.md)。

來源：[ROS 2 Jazzy topic 教學](https://github.com/ros2/ros2_documentation/blob/jazzy/source/Tutorials/Beginner-CLI-Tools/Understanding-ROS2-Topics/Understanding-ROS2-Topics.rst)、[rclpy Node API](https://github.com/ros2/rclpy/blob/jazzy/rclpy/rclpy/node.py)、[QoS 相容性](https://github.com/ros2/ros2_documentation/blob/jazzy/source/Concepts/Intermediate/About-Quality-of-Service-Settings.rst)。
