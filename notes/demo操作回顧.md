# demo 操作回顧

[← 回考場入口](README.md) · [從問題找原碼](程式註解導讀.md) · [家用環境紀錄](練習環境與路線.md)

**先用你熟悉的 demo 回想概念，再查陌生題型。** 這頁整理你問過的問題、貼過的輸出與目前程式設定。PIO 是閱讀／手算範例，沒有實板操作紀錄。

## 最重要的六句

| demo／問題 | 回顧重點 |
|---|---|
| Docker 改了沒變 | 當時是新舊 repo 掛錯；同一份檔案、重編譯、重啟程序要分開確認 |
| ROS 的 /double | topic 是名稱，公式在 callback；目前21→64，原版21→42 |
| make 和 engine | Make 把兩份 .c 編譯連結；main 填工作，engine 的 thread 真正搬資料 |
| make mistake | count 是筆數，N=8 的正確值是8；錯填32就搬128 bytes |
| 共享變數 | 保護完整讀改寫；獨立耗時計算不要一起鎖住 |
| PIO 的 push | ISR→RX FIFO；FIFO 才是 queue，CPU 還要讀取 |

## 1 Docker：你當時真的卡在哪

你改的是新 repo，容器原先卻掛另一份舊 demo。**在主機哪個資料夾執行 docker exec，不會改掉原掛載。**

修正時的對應：

```text
主機 ~/Development/ncku-srecruit-practice/demo
                      ↕ bind mount
srecruit-demo 容器的 /demo
```

- 檔案不同：查 mount 的 Source／Destination、編輯器是否存檔。
- 檔案相同但結果舊：Python 要重啟，C 要重編再跑。
- image 的 COPY 是建置時快照；bind mount 才是接到同一份主機檔案。

[當時的路徑與排錯紀錄](練習環境與路線.md) · [完整 Docker／掛載教學](技術/Docker.md)

## 2 ROS：三個終端機各做什麼

```text
publisher 或 CLI → /number → processor 的 on_msg → /double → echo
```

| 動作 | 做什麼 |
|---|---|
| python3 processor.py | 啟動訂閱、計算與發布，沒輸入時等待 |
| ros2 topic echo /double | 只接收並印出結果，不替你執行公式 |
| ros2 topic pub --once … | 由 CLI 送一筆已知輸入 |
| ros2 node info /processor | 看 Subscribers／Publishers 接線，不證明資料已通過 |

`self.pub` 在初始化時已記得送往 /double，所以 callback 不必再寫一次 topic 名。`.data` 是訊息裡的整數；`=` 把算出的值存進去，單獨的減法不會存回。

| 公式／名稱 | 結果 |
|---|---|
| 官方原版 received.data*2 | 21→42 |
| 目前 repo received.data*3+1 | 21→64；這也是先前協助確認過的對照值 |
| /number 改 /numbers | 名稱不同就接不到原通道，不是公式算錯 |

看 /number 不需要 processor，echo 自己就是接收者。`node list` 查執行單位，`topic list` 查通道。

[原本的逐步教學與改前改後](例題/02-ROS訊息.md) · [原演練詳細操作](../demo/section2-1/README.md)

## 3 Make／搬運：那串輸出在說什麼

你貼過的原版輸出是4096筆；**目前 repo 已保留 N=8**，兩個數字代表不同版本。

```text
Makefile → 編譯、連結 main.c 與 engine.c → bin/transfer-engine → 執行
main.c：描述工作；engine.h：宣告型別與介面；engine.c：實際逐筆搬
```

| 目前設定 | 正確解讀 |
|---|---|
| claimed channel 0 (there are 4) | 取得第0號通道，共4個；不是搬0筆 |
| count=8、XFER_32 | 8筆×每筆4 bytes＝32 bytes |
| copy | 來源與目的都逐筆前進 |
| fill | 來源固定讀同一個 pattern，目的逐筆前進 |
| 主程式轉了很多圈 | main 在等待期間做的計數，不是搬運量或速度 |

`wrong.count = N * sizeof(uint32_t)` 在 N=8 時得到32，但這裡要的是筆數：變成32筆×4＝128 bytes，比原本多24筆。正確是 `count=N`；memcpy／memcmp 的長度才用 bytes。

指標 `dst=src` 只換指向；不會複製內容。這份 engine 用 CPU thread 模擬搬運，真 DMA 才由硬體搬。**configure→start→wait→取結果**，start 返回不表示完成。

[原本的 N=4096／N=8／mistake 對照](例題/04-資料搬運.md) · [Make 與 .h/.c 詳解](技術/編譯與Make.md)

## 4 位元：改 mask，改的是保留位置

| x=0xA5 | 結果 |
|---|---|
| x & 0x0F | 0x05，留下低四位 |
| x & 0xF0 | 0xA0，留下高四位但沒移位置 |
| (x & 0xF0) >> 4 | 0x0A，把高四位移到最低位，得到數值10 |

這些表達式本身不改 x，賦值才寫回。十六進位顯示與實際值是兩件事。

[原本的逐行位元教學](例題/01-位元運算.md)

## 5 臨界區：為什麼加兩次可能沒留下2

```text
A讀0 → B讀0 → A寫1 → B寫1
```

鎖保護「讀→加一→寫回」整段，其他使用同一把鎖的 worker 才能等到新值。演練用 spinlock，等待者會反覆檢查；把獨立耗時計算也鎖住，其他 worker 就一起等。

原碼無鎖更新屬 C data race，數字沒有保證；短鎖版本的共享 sink、引擎的 volatile busy 另有同步限制。**一次印 ok 不代表整支程式同步安全。**

[原本的讀寫順序教學](例題/03-共享變數.md) · [同步與 API 查表](技術/平行運算.md)

## 6 PIO：這部分是閱讀回顧

以下沿用原範例假設：CPU 已設定32-bit autopull 等初始化。單獨的 .pio 檔沒有提供完整啟動程式。

```text
CPU→TX FIFO→OSR→out x,32→X→in x,32→ISR→push→RX FIFO→CPU讀取
```

FIFO 是有容量的硬體 queue；push 是把 ISR 放進 RX FIFO 的指令，不是 queue 本身，也不是直接送到 CPU 變數。

兩個 X 一起改成 Y，資料仍原樣回傳；只把 out 改 Y、in 仍讀 X，就回傳 X 的舊值。32是位數，不是數值32。

[原本逐步追資料的教學](例題/05-PIO資料.md) · [PIO 指令與 API](技術/PIO.md)

## 要找其他 demo 文件

- [原演練環境與三份 demo](../demo/README.md)：原操作文件完整保留。
- [自編 C worker／binary／ROS 教學](../demo/first-development/README.md)：保留當時教學與程式，沒有據此宣稱你已跑完每個變化。
- [ISA 解碼器](../demo/isa-practice/README.md)、[Pico／PIO 專案](../demo/rp2350-pio-walkthrough/README.md)：先前新增的補充，與已經操作的原 demo 分開辨認。

要把熟悉的概念套到新題目，再讀 [猜題與解題思路](猜題與解題思路.md)。
