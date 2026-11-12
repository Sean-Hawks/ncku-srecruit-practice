# 02 改一行，21 從 42 變成 64

打開 [processor.py](../../demo/section2-1/processor.py)。先只看：

```python
out.data = received.data * 2
```

`received.data` 是收到的數字；`out.data` 是準備送出的數字。這裡就是普通的乘法，ROS 負責把輸入送來、把輸出送走。

## 原版：送 21，收到 42

三個終端機都進入同一個 container 的 `/demo/section2-1`。這題停掉 publisher.py，避免隨機數混進來。

A 跑接收與處理程式：

```bash
python3 processor.py
```

B 先觀察輸出，保持執行：

```bash
ros2 topic echo /double
```

C 發送一次已知數字：

```bash
ros2 topic pub --once /number std_msgs/msg/Int32 "{data: 21}"
```

B 應出現 `data: 42`：

```text
21 → /number → 乘 2 → /double → 42
```

## 改一次：乘 3，再加 1

將 processor.py 的那行改成：

```python
out.data = received.data * 3 + 1
```

在 A 停止舊程式後重新執行 `python3 processor.py`，再到 C 重送 21。B 現在應收到 `data: 64`。

**檔案改了要重啟程序，舊程序不會自動讀取修改。** Topic 仍叫 /double，名稱不會自動強迫內容乘二。

## 再改一次：故意讓資料接不到

將：

```python
INPUT_TOPIC = '/number'
```

改成：

```python
INPUT_TOPIC = '/numbers'
```

重啟 A，再送到 /number。這次不應出現新的對應輸出。你送的是 /number，程式等的是 /numbers。

用 C 查：

```bash
ros2 node info /processor
```

訂閱清單會出現 /numbers。這就是斷點。改回 /number、重啟、再送 21，應恢復 64。

## 換你試

改成 `out.data = received.data - 5`。重啟後，分別發送 8 與 0，應收到什麼？

<details>
<summary>看答案</summary>

8 → 3；0 → -5。每次都先保持 B 的 echo 執行，再從 C 發送。

</details>

**過關：**你能分清「計算改錯」與「輸入根本沒送到」。只修改檔案，不需要了解整套 ROS 內部實作。

下一題：[把兩次加一拆開看](03-共享變數.md)。
