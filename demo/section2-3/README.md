# 考前練習材料

[← demo 操作回顧](../../notes/demo操作回顧.md) · [原本的搬運教學](../../notes/例題/04-資料搬運.md)

> 目前 repo 的搬運設定為 N=8；你先前貼過與官方原版的4096筆，是不同版本。以下已改用8筆示例；耗時與等待圈數只是示意，不是固定結果。原碼同步限制見本文臨界區說明。

這裡有兩個小程式，用來在考試前先接觸兩個概念。**在你自己的電腦上就能編譯執行，不需要任何開發板。**

這些程式跟考題本身沒有直接關係，裡面不會出現任何考題內容。它們只是讓你先摸過兩件事，省下考場上從零開始理解的時間。

## 編譯與執行

需要 `gcc`（或 `clang`）與 `make`。Linux 直接跑；Windows 建議用 WSL；macOS 也可以。

```bash
make            # 編譯
make run        # 依序執行兩個 demo
```

也可以分開跑：

| 指令 | 作用 |
|---|---|
| `make run-critical-section` | 只跑 critical section |
| `make run-transfer-engine` | 只跑 transfer engine |
| `make mistake` | transfer engine 的「最常見錯誤」示範 |
| `make clean` | 清掉編譯產物 |
| `make help` | 列出所有指令 |

原始碼在 `critical-section/` 與 `transfer-engine/` 兩個資料夾裡，編譯出來的執行檔放在 `bin/`。

## Critical section：兩個執行緒、一個共享變數

```bash
make run-critical-section
```

兩個工作者各對同一個變數加 1 共 20000 次，結果「理所當然」應該是 40000。

實際輸出大概像這樣：

```
expected counter = 40000   (12 CPUs available)

1. no lock            counter = 39734    LOST UPDATES     70.3 ms
2. lock, short hold   counter = 40000    ok               67.2 ms
3. lock, long hold    counter = 40000    ok              155.6 ms
```

三件事值得注意：

1. **第1行有未同步的共享讀寫。** 「A、B都讀舊值，再寫回相同結果」可用來理解遺失更新；實際 C data race 是未定義行為，不能保證每次都少幾次，偶然印 ok 也不代表安全。
2. **第2行用 spinlock 保護 counter 的完整讀改寫。** 這段是臨界區；但原碼在鎖外還更新共享 sink，仍有 data race，不能把整個版本當正確同步範本。
3. **第3行把耗時計算也放進同一把鎖。** 其他 worker 必須等待，可能變慢；上方耗時只是示例，不能保證固定慢幾倍。該保護的共享存取要完整保護，獨立計算才放鎖外。

> 單核心也可能交錯執行；無鎖偶然得到40000不證明安全。原搬運引擎的 volatile busy 也缺少完整 thread 同步。範例用來讀配置與流程，實際同步依正確的鎖／atomic／SDK API。

## Transfer engine：把工作「描述」出去，而不是自己寫迴圈

```bash
make run-transfer-engine
make mistake
```

這裡用另一條 CPU thread 模擬假想的「搬運引擎」；抽象上是把工作交給另一個執行者，並非真的硬體 DMA。你不寫複製迴圈，而是**描述**這件工作：從哪讀、寫到哪、幾筆、每筆多大、哪一邊的位址要往前走。描述完就啟動，然後去做別的事，之後再回來收結果。

使用的節奏固定是四拍：

```
描述 (configure)  →  啟動 (start)  →  等待 (wait / busy)  →  取用結果
```

輸出大概像這樣：

```
claimed channel 0 (there are 4)

1. copy 8 words          dst matches src
   while it ran, this program went round its own loop 98705538 times

2. fill 8 words          every word is 0xA5A5A5A5
   same engine, same count -- only the increment flags changed
```

四個值得記住的點：

1. **`count` 的單位是「幾筆傳輸」，不是「幾個位元組」。** 這是最常見的錯誤。跑 `make mistake` 會示範把位元組數填進去會發生什麼事——引擎不會抱怨，它只是忠實地照你說的做了四倍的量。
2. **位址要不要遞增，是描述的一部分。** 程式裡的「複製」和「填值」用的是同一個引擎、同樣的筆數，差別只在來源位址走不走。
3. **每筆的大小只能是 1、2 或 4 個位元組**，而且位址要對齊。填錯引擎會直接停下來告訴你哪條規則被違反了。
4. **通道是有限資源**，用之前要取得、用完要歸還。這個引擎只有四個。

### 這個程式沒辦法示範的事

那個引擎在這裡是用一條執行緒做的，也就是說**它還是佔用了你電腦的一顆核心**。

真正的硬體不是這樣——它消耗的是記憶體匯流排的頻寬，不是 CPU 的指令。搬運期間 CPU 是真的空出來的，可以去做完全不相干的事情。

所以輸出裡那個「主程式跑了幾圈」的數字，只能說明「啟動之後你的程式還能繼續往下跑」，**不能**說明「這件事沒有成本」。這一層差別沒有硬體是做不出來的，你先知道有這回事就好。
