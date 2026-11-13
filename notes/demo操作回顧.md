# demo 操作回顧：以例題看懂程式

[← 回考場入口](README.md) · [從問題找原碼](程式註解導讀.md)

**按原本五個例題回顧：這題做什麼 → 程式怎麼走 → 改哪裡、結果怎麼變 → 你問過的問題。** 答案已附，直接看對照；每題末尾可回到原操作教學。

| 例題 | 要看懂的變化 |
|---|---|
| [01 位元運算](#例題01位元運算) | mask改0x0F／0xF0，為何05變A0，右移後才是0A |
| [02 ROS訊息](#例題02ros-訊息) | 21從42變64；收不到和算錯怎麼區分 |
| [03 共享變數](#例題03共享變數) | 兩次加一，讀寫順序不同為何得到1或2 |
| [04 資料搬運](#例題04資料搬運) | 4096改8；copy、fill、mistake分別做什麼 |
| [05 PIO資料](#例題05pio-資料) | 同改X→Y不變，只改一個卻讀到舊值 |

## 例題01：位元運算

**這題做什麼：**從同一個整數選出指定位置的 bits，分清「保留原位置」與「移到最低位後的值」。原碼是 [demo_asm.c](../demo/section2-2/demo_asm.c)。

原本 x=0xA5，也就是二進位1010 0101：

```text
x & 0x0F：1010 0101 & 0000 1111 = 0000 0101
```

**改哪裡，結果怎麼變：**

| 表達式 | 已解讀的結果 | 原因 |
|---|---|---|
| x & 0x0F | 0x05 | 留下低四位 |
| x & 0xF0 | 0xA0 | 留下高四位，位置沒動 |
| (x & 0xF0) >> 4 | 0x0A，即10 | 把高四位移到最低位 |
| x換成0x3C，再做以上三式 | 0x0C、0x30、0x03 | 相同mask，處理不同輸入 |

**容易混淆：**`x & mask`只算出結果，不會改x；`x &= mask`才存回。`%02X`只決定十六進位顯示方式。

**這題記一句：AND選位置，右移把選到的欄位放到最低位。**

[原本逐行修改與操作教學](例題/01-位元運算.md) · [公式查表](技術/位元運算.md)

## 例題02：ROS 訊息

**這題做什麼：**接到一個整數，算完再發布。主角是 [processor.py](../demo/section2-1/processor.py) 的on_msg，不是topic名稱。

```text
publisher或CLI → /number → processor的on_msg → /double → echo
```

**改哪裡，結果怎麼變：**

| 計算／設定 | 輸入21時的結果 |
|---|---|
| 原版 out.data = received.data * 2 | 42 |
| 目前repo out.data = received.data * 3 + 1 | 64 |
| 訂閱從/number改成/numbers，仍往/number送 | 接不到這筆輸入，不是算出另一個數字 |

**你問過：為什麼函式裡沒寫double？** `self.pub`在初始化時已記得往/double送。`/double`只是通道名，真正公式是out.data那一行；`.data`是訊息中的整數欄位，`=`才把結果存進去。

| 原教學中的動作 | 這個步驟在做什麼 |
|---|---|
| python3 processor.py | 啟動接收、計算與發布；沒輸入時等待 |
| ros2 topic echo /double | 接收並顯示結果，不替processor計算 |
| ros2 topic pub --once /number … | CLI送一筆已知輸入，取代隨機publisher |
| ros2 node info /processor | Subscribers看從哪收，Publishers看往哪送；只是接線資訊 |

看/number不需要processor，echo自己就能接收。node list列功能單位，topic list列通道；通道存在不表示已有資料通過。

**當時改了卻沒變：**nvim改新repo，Docker卻掛舊demo。修正時把 `~/Development/ncku-srecruit-practice/demo` 接到容器 `/demo`。掛載正確後也要先存檔，再重啟Python，新啟動的程序才使用新公式。

**這題記一句：先確認收到哪筆資料，再看callback算什麼，最後看發布到哪。**

[原本三個終端機與改前改後教學](例題/02-ROS訊息.md) · [原演練詳細操作](../demo/section2-1/README.md) · [當時掛載排錯紀錄](練習環境與路線.md)

## 例題03：共享變數

**這題做什麼：**用兩次加一的讀寫順序，理解為什麼共享更新需要保護。原demo是 [critical-section/main.c](../demo/section2-3/critical-section/main.c)。

原教學另用a、b表示「各自讀到的舊值」，固定示意兩種順序：

| 讀寫順序 | 已解讀的結果 |
|---|---|
| A讀0→B讀0→A寫1→B寫1 | 最後1，B還在用先前讀到的0 |
| A讀0→A寫1→B讀1→B寫2 | 最後2，B讀到更新後的值 |

這是時序示意；實際C程式未同步的衝突存取屬data race，不能保證每次只少幾次。

**接回demo的三種worker：**兩個worker各加20000次，預期counter為40000。

| 版本 | counter怎麼處理 | 要看懂什麼 |
|---|---|---|
| no lock | 讀改寫未同步 | 結果不可靠，偶然印ok也不代表安全 |
| lock, short hold | spinlock保護完整counter++ | 鎖外可放獨立計算，但所有共享狀態仍要同步 |
| lock, long hold | 同一把鎖連耗時計算一起保護 | 別的worker連獨立計算也得等，可能變慢 |

**你要分清：**只鎖寫入，仍可能用到過期的讀取值；`volatile`也不會把counter++變成原子操作。原短鎖版本還有共享sink的race，不能因counter等於40000就把整支程式當安全範本。

**這題記一句：保護完整讀→改→寫，獨立耗時計算才放鎖外。**

[原本a／b逐步對照與操作教學](例題/03-共享變數.md) · [同步與API查表](技術/平行運算.md)

## 例題04：資料搬運

**這題做什麼：**main描述「從哪搬、搬幾筆、每筆多大」，交給引擎執行。你貼過的輸出是原版4096筆；目前repo保留N=8，這是兩個版本。

**先看make那串命令：**

| 檔案 | 在這題的角色 |
|---|---|
| [main.c](../demo/section2-3/transfer-engine/main.c) | 填來源、目的、count、size與increment工作單 |
| [engine.h](../demo/section2-3/transfer-engine/engine.h) | 宣告工作單型別與函式介面 |
| [engine.c](../demo/section2-3/transfer-engine/engine.c) | thread裡真正逐筆搬資料 |
| [Makefile](../demo/section2-3/Makefile) | 把兩份.c編譯連結；run目標再執行產物 |

main.c include engine.h，不代表engine.c就會自動加入；實作是由Makefile的編譯／連結命令接進來。

**改哪裡，結果怎麼變：**

| 設定／指令 | 已解讀的結果 |
|---|---|
| 原版N=4096、XFER_32 | 4096筆×4 bytes＝16384 bytes |
| 目前N=8、XFER_32 | 8筆×4 bytes＝32 bytes |
| copy：來源／目的都遞增 | src[0]→dst[0]，src[1]→dst[1]… |
| fill：來源固定、目的遞增 | 同一個pattern→dst[0]、dst[1]… |
| make mistake | 同一個執行檔加--mistake，追加錯誤count示範 |

故意錯的那行：

```c
wrong.count = N * sizeof(uint32_t); // N=8時，算出32卻當成筆數
// 正確應是 count = N
```

每個uint32_t是32 bits，在此平台佔4 bytes。錯誤版本搬32筆×4＝128 bytes，比預定多24筆。demo實際預留32筆，沒有因此超出真正配置；若只配8筆就會越界。memcpy／memcmp的長度才用bytes，不能和count混用。

**你問過：為什麼不用dst=src？** 陣列不能在宣告後這樣整體賦值；如果是指標賦值，只換指向，沒有複製內容。這題用thread模擬搬運，仍消耗CPU；真DMA才是硬體幫忙搬。

**那串輸出怎麼看：**claimed channel 0是取得第0號通道；「主程式轉幾圈」是等待時的計數，不是搬幾筆或速度。configure只是設定，start只是開始，**wait完成後才使用結果**。原引擎volatile busy另有同步限制，不能當正式thread同步範本。

**這題記一句：先算筆數×每筆bytes，確認兩端位址，再等完成取結果。**

[原本4096→8、copy／fill／mistake教學](例題/04-資料搬運.md) · [Make與.h/.c詳解](技術/編譯與Make.md)

## 例題05：PIO 資料

**這題做什麼：**追一筆資料在PIO各儲存位置之間移動。對照 [demo_pio.pio](../demo/section2-2/demo_pio.pio)；這題是閱讀／手算回顧，沒有實板操作紀錄。

假設CPU已啟動狀態機、設定32-bit autopull並關閉autopush，FIFO可正常收送；單獨的.pio檔沒有完整啟動程式。

```pio
out x, 32    ; OSR → X，取32 bits
in  x, 32    ; X → ISR，放32 bits
push         ; ISR → RX FIFO，供CPU稍後讀取
```

**資料實際走哪裡：**CPU→TX FIFO→OSR→X→ISR→RX FIFO→CPU。autopull負責TX FIFO補OSR；out／in／push對應中間三步。

**改哪裡，結果怎麼變：**

| 程式變化 | 已解讀的結果 |
|---|---|
| out x,32；in x,32；push | 送0x12345678，原樣回0x12345678 |
| out與in的兩個X一起改Y | 一樣原樣回傳，只換中途暫存位置 |
| 只把out改Y、in仍讀X；原X=7，送99 | 回7，因為99寫入Y，下一行卻讀舊X |

**你問過：push是queue或記憶體嗎？** FIFO才是有限容量的硬體queue；push是一條指令，把ISR放進RX FIFO。CPU還要讀才拿到；RX滿時預設阻塞的push會等。32是位數，不是數值32。

**這題記一句：每一行都找來源與目的，寫Y卻讀X就不是同一份資料。**

[原本逐步追資料與X／Y對照教學](例題/05-PIO資料.md) · [PIO指令與CPU端API](技術/PIO.md)

## 配套與其他原教學

- [原演練環境與三份demo](../demo/README.md)、[Docker／掛載教學](技術/Docker.md)：回查當時環境與完整指令。
- [C worker／binary／ROS自編教學](../demo/first-development/README.md)、[ISA解碼器](../demo/isa-practice/README.md)、[Pico／PIO專案](../demo/rp2350-pio-walkthrough/README.md)：原文件保留，沒有據此宣稱每個變化都已親自操作。

把各題概念套到新題目，另看 [猜題與解題思路](猜題與解題思路.md)。
