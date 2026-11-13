# 編譯與 Make：改檔之後，怎麼產生新程式

**make 讀規則、挑要重建的目標；真正翻譯 C 的是 cc／gcc／clang。** 編譯成功不等於已執行。

## 先看專案種類

| 看到什麼 | 優先操作 |
|---|---|
| 單一 .c | 依 README 用 cc |
| Makefile | `make help` 若有提供；`make -n` 看預計命令 |
| CMakeLists.txt | README 的 cmake configure／build |
| ESP-IDF／PlatformIO | [ESP32](ESP32.md) 的對應流程 |
| ROS package | [ROS 2](ROS2.md) 的 colcon 流程 |

## 你問過：engine.h、engine.c 有沒有用

```text
main.c   ─ include engine.h ─ 宣告：有哪些型別、函式怎麼呼叫
engine.c ─ include engine.h ─ 實作：函式裡實際怎麼做
   └────────── cc 編譯、連結兩份 .c ──────────→ 執行檔
```

include header 不會自動編譯 engine.c。漏掉實作常出現 `undefined reference`。

現有 [Makefile](../../demo/section2-3/Makefile) 實際執行的形式是：

```bash
cc -O2 -Wall -Wextra -pthread -o bin/transfer-engine \
  transfer-engine/main.c transfer-engine/engine.c
```

這條要在 section2-3 執行，且 bin 已存在；平常直接讓 make 管理就好。

## 日常操作：容器的 /demo/section2-3

```bash
make help
make -n run-transfer-engine   # 只列命令，不跑
make                         # 建置，不執行 demo
make run-transfer-engine     # 必要時先重編，再執行
make mistake                 # 同一個執行檔加 --mistake
```

`make clean` 刪本專案的 bin 建置產物；下次 make 重建。不是每次改程式都需要 clean。

## 單檔 C：先除錯，再量效能

下面假設你真的有 main.c，在**Linux 專案目錄**：

```bash
cc -std=c11 -O0 -g -Wall -Wextra main.c -o app
./app
```

確定正確後再比較 `-O2`，不要把不同最佳化設定當同一組效能條件。thread 程式加 `-pthread`。

| 參數 | 用途 |
|---|---|
| `-o app` | 輸出檔名 |
| `-I 路徑` | 增加 header 搜尋目錄 |
| `-c` | 只產生 object，暫不連結 |
| `-g` | 供 debugger 使用的資訊 |
| `-Wall -Wextra` | 顯示更多警告，優先處理型別與邊界問題 |
| `-O0`／`-O2` | 不做一般最佳化／啟用常用最佳化 |

## 錯誤對照

| 訊息 | 先查 |
|---|---|
| header: No such file | include 路徑、SDK 環境，不能拿主機 cc 編板子程式 |
| undefined reference | 少連結 .c／library，或函式名稱不一致 |
| make: missing separator | recipe 開頭通常必須是 Tab |
| No rule to make target | 目標拼錯或不在正確目錄 |
| Nothing to be done | 依時間戳記看起來不需重建；確認改的是依賴檔 |

來源：[GNU Make 手冊原始碼](https://github.com/mirror/make/blob/master/doc/make.texi)、[GCC 選項](https://gcc.gnu.org/onlinedocs/gcc/Option-Summary.html)。

[回技術目錄](README.md)
