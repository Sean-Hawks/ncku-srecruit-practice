# 指令編碼實作（自編，不需板子）

> 原教學與操作步驟保留。先看 [demo 操作回顧](../../notes/demo操作回顧.md)，再依需要回查本頁。

先讀 [ISA、編碼與定址](../../notes/技術/ISA與指令編碼.md)。

在既有 **Docker 容器**：

```bash
cd /demo/isa-practice
mkdir -p bin
cc -std=c11 -Wall -Wextra decode.c -o bin/decode
./bin/decode
```

應依序解出：

```text
asm: addi x5, x6, 8
asm: lw x5, 8(x6)
asm: addi x5, x6, -4
```

前面也會印出 opcode、rd、rs1、funct3、imm，逐欄對照手冊。這個 C 程式只解碼 ADDI 與 LW，不執行機器碼、不讀寫模擬 RAM，也不處理其他指令格式。

## 自己改，不用背答案

```bash
./bin/decode 0x00C30293
./bin/decode 0x00830393
./bin/decode 0xFFC32283
```

先預測哪個欄位變了，再執行。預期依序為：

- `addi x5, x6, 12`：立即數從 8 改 12。
- `addi x7, x6, 8`：目的暫存器從 x5 改 x7。
- `lw x5, -4(x6)`：記憶體位址是 x6 的值減 4。

再修改 [decode.c](decode.c)：在已判斷是 LW 的分支，假設 rs1 暫存器存 0x1000，印出有效位址。正偏移 8 應是 0x1008；負偏移 -4 應是 0x0FFC。這個數字是位址，不是該位址存的內容。
