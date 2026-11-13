# ISA、指令編碼與定址

[← 回考場筆記入口](../README.md) · [位元公式](位元運算.md)

**ISA 是「一串 bits 代表什麼動作」的規格。** CPU 讀機器碼，依規格決定用哪些暫存器、做什麼事、下一條去哪。

官方演練的位元／PIO 節（section2-2） 特別提示「基本的編碼與定址方式」。本頁以參考資料中的 **RISC-V RV32I** 解釋；正式題目若給別的架構，要查該架構的欄位圖。

## 先認清執行者

| 執行者 | 看哪種指令 | 查哪份手冊 |
|---|---|---|
| RISC-V CPU | addi、lw、sw、beq | 題目指定 RISC-V ISA／擴充 |
| Arm CPU | Arm／Thumb 指令 | 題目指定的 Arm 架構手冊 |
| PIO 狀態機 | out、in、push、jmp | 晶片 datasheet 的 PIO 章節 |

RP2350 提供 Arm 與 Hazard3 RISC-V CPU 選項，實際用哪個看專案設定。PIO 自己的 ISA 與兩者都不同。

## 文字、機器碼、執行效果

```text
組語：addi x5, x6, 8
編碼：0x00830293
意思：x5 ← x6 的值 + 8
```

`x5` 是目的暫存器；`x6` 是來源暫存器；`8` 是指令直接帶的常數，叫立即數 immediate。編碼是把這些資訊塞進指定 bits；解碼是把它拆出來。

## 固定欄位速查：RV32I 的 32-bit 指令

bit 0 在最右側；下表是欄位，不表示每條指令都會用到每個欄位。

| 欄位 | bits | 取法，w 為 uint32_t |
|---|---|---|
| opcode：主要分類 | 6..0 | `w & 0x7Fu` |
| rd：目的暫存器 | 11..7 | `(w >> 7) & 31u` |
| funct3：細分操作 | 14..12 | `(w >> 12) & 7u` |
| rs1：來源 1 | 19..15 | `(w >> 15) & 31u` |
| rs2：來源 2 | 24..20 | `(w >> 20) & 31u` |
| funct7：進一步細分 | 31..25 | `(w >> 25) & 127u` |

| 格式 | 高位 → 低位 | 常見用途 |
|---|---|---|
| R | funct7 / rs2 / rs1 / funct3 / rd / opcode | add、sub |
| I | imm[11:0] / rs1 / funct3 / rd / opcode | addi、lw、jalr |
| S | imm[11:5] / rs2 / rs1 / funct3 / imm[4:0] / opcode | sw |
| B | 立即數分散，見下表 | beq |
| U | imm[31:12] / rd / opcode | lui、auipc |
| J | 立即數分散，見下表 | jal |

opcode 只是分類，完整辨識還要核對 funct3、funct7 等固定欄位。例如 add/sub 的 opcode 相同。

## 常見指令辨識與效果

下表只列 RV32I 中這些指令，其他編碼按手冊；rs/rd 的內容都是 32-bit 值。

| 指令 | opcode | funct3／其他 | 效果 |
|---|---|---|---|
| add | 0x33 | 0，funct7=0 | rd ← rs1 + rs2 |
| sub | 0x33 | 0，funct7=0x20 | rd ← rs1 − rs2 |
| addi | 0x13 | 0 | rd ← rs1 + 有號 imm12 |
| lw | 0x03 | 2 | 從 rs1+imm12 的位址讀 4 bytes 到 rd |
| sw | 0x23 | 2 | 把 rs2 的低 32 bits 寫到 rs1+imm12 |
| beq | 0x63 | 0 | rs1==rs2 時，PC ← 目前 PC + 有號 B 偏移 |
| jal | 0x6F | J 格式 | rd ← PC+4；PC ← 目前 PC + J 偏移 |
| jalr | 0x67 | 0 | rd ← PC+4；PC ← (原 rs1+imm12) 清最低 bit |
| lui | 0x37 | U 格式 | rd ← 指令的高 20 bits，低 12 bits 清零 |
| auipc | 0x17 | U 格式 | rd ← 目前 PC + U 立即數 |

PC 是目前指令的位址；一般 32-bit 指令依序前進 4 bytes。若允許壓縮指令，會有 16-bit 指令，不能全部加 4。x0 固定為 0，寫它不會改變其值。

## 立即數：先拼 bits，再處理正負

| 格式 | 指令 bits 如何組回立即數 |
|---|---|
| I | `[31:20]` → imm[11:0] |
| S | `[31:25]` → imm[11:5]；`[11:7]` → imm[4:0] |
| B | `[31]` → imm[12]；`[7]` → imm[11]；`[30:25]` → imm[10:5]；`[11:8]` → imm[4:1]；imm[0]=0 |
| U | 保留 `[31:12]`，低 12 bits 補 0 |
| J | `[31]` → imm[20]；`[19:12]` → imm[19:12]；`[20]` → imm[11]；`[30:21]` → imm[10:1]；imm[0]=0 |

I/S 立即數有號 12 bits，範圍 -2048..2047。raw `0xFFC` 的最高位為 1，代表 **4092−4096=−4**。

```c
uint32_t raw = (w >> 20) & 0xFFFu; // I 格式
int32_t imm = (int32_t)(raw & 0x7FFu) - (int32_t)(raw & 0x800u);
```

B/J 也要依各自寬度做符號延伸。上表已把最低的 0 拼進偏移，得到的是 **byte 偏移，不要再乘 2**。做算術／位址模擬時，也要遵守目標的 32-bit 規則，避免用 C 有號溢位來模擬。

## 定址：位址與內容分開看

假設 x6=0x1000，而且 0x1008 是合法可讀位置、其中存 42：

| 指令 | 結果 |
|---|---|
| `addi x5, x6, 8` | x5=0x1008，只加法 |
| `lw x5, 8(x6)` | 先算位址 0x1008，再讀內容，因此 x5=42 |
| `lw x5, -4(x6)` | 讀取位址 0x0FFC 的 4 bytes |
| `sw x5, 8(x6)` | 把 x5 的值寫到位址 0x1008 |

這叫「基底暫存器 + 偏移」。偏移 8 是 **8 bytes**；不同於 C 的 `uint32_t *p; p+8`，後者前進 8 個元素。

load/store 寬度也要查：lb/lbu=1 byte，lh/lhu=2 bytes，lw=4 bytes；lb/lh 會符號延伸，lbu/lhu 補零。位址需在有效記憶體範圍，對齊規定依目標環境。

## 同一條指令改 bits，結果怎麼變

| 機器碼 | 已解讀答案 |
|---|---|
| 0x00830293 | `addi x5, x6, 8` |
| 0x00C30293 | `addi x5, x6, 12`：改立即數 |
| 0x00830393 | `addi x7, x6, 8`：改目的暫存器 |
| 0xFFC30293 | `addi x5, x6, -4`：負立即數 |
| 0x00832283 | `lw x5, 8(x6)`：改成讀記憶體 |

若題目輸入的是 bytes 而非完整數值，要先按規定組回指令。例如 little-endian 的 `93 02 83 00` 組成 `0x00830293`；不能把顯示順序直接當數值高低位。

## 手冊查找關鍵字

找 **Instruction formats／Encoding** 取得欄位圖；**Load and Store／Addressing** 查位址與寬度；**Immediate／Sign extension** 查正負；**Control transfer** 查 PC 更新。先確認題目要求的 ISA 與指令子集合。

來源：[RV32I 規格（格式、立即數、讀寫與跳躍）](https://docs.riscv.org/reference/isa/v20260120/unpriv/rv32.html)、[官方逐指令編碼](https://github.com/riscv/riscv-opcodes/blob/master/extensions/rv_i)、[RP2350 CPU 選項](https://www.raspberrypi.com/products/rp2350/)。本頁列出的五筆機器碼已與組譯結果核對；不代表完整 ISA 實作。
