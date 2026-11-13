# ISA、指令編碼與定址：對照材料提示準備什麼

**材料明確建議熟悉「基本的編碼與定址方式」，這是獨立於環境建置的準備項目。** 原 [section2-2 README](../../demo/section2-2/README.md) 也直接列出 RISC-V 指令集手冊與 RP2350 PIO 文件。

這表示位元操作要練到能拆指令欄位、追蹤暫存器與位址。實際會考解碼、模擬還是其他用途仍未知；以下是依材料線索安排的準備，不是考題預告。

## 1 CPU 指令與 PIO 指令分開查

| 哪個執行者 | 指令例子 | 要查哪份文件 |
|---|---|---|
| RISC-V CPU | addi、lw、sw、beq | 題目使用的 RISC-V ISA 與擴充 |
| Arm CPU | 對應 Arm／Thumb 指令 | 題目指定架構手冊 |
| PIO 狀態機 | out、in、push、jmp | 該晶片 datasheet 的 PIO 章節 |

RP2350 有 Arm 與 Hazard3 RISC-V 的 CPU 選項；板子型號本身不能決定你這題要讀哪個 ISA。先查題目、專案設定和提供的指令範圍。PIO 自己的指令集也不是 RISC-V。

## 2 三個名詞，用同一個例子看

```text
組語文字：addi x5, x6, 8
編碼數值：0x00830293
執行意思：把 x6 裡的值加 8，結果放到 x5
```

- **ISA**：規定有哪些指令、各欄位長什麼樣、執行會改哪些狀態。
- **編碼**：把「做加法、用 x6、寫 x5、加 8」放進指定的 bits。
- **解碼**：反過來從 bits 取出這些欄位，判斷要做什麼。

`x5、x6` 是暫存器編號；`8` 是直接寫在指令內的常數，叫立即數。不是第八個記憶體位置。

## 3 用真的 RV32I 格式拆，不用自創欄位

ADDI 與 LW 使用下面的 32-bit I-type 格式，bit 0 在最右邊：

```text
31          20 19       15 14    12 11        7 6          0
| imm[11:0] |    rs1    | funct3 |     rd     |   opcode   |
```

| 欄位 | 是什麼 | 在 C 裡怎麼取 |
|---|---|---|
| opcode | 主要操作分類 | `word & 0x7Fu` |
| rd | 寫結果的暫存器編號 | `(word >> 7) & 0x1Fu` |
| funct3 | 細分操作 | `(word >> 12) & 7u` |
| rs1 | 讀來源的暫存器編號 | `(word >> 15) & 0x1Fu` |
| imm 原始 bits | 12-bit 立即數，還要當有號解讀 | `(word >> 20) & 0xFFFu` |

`0x00830293` 拆出 opcode=0x13、funct3=0、rd=5、rs1=6、imm=8，因此是 `addi x5, x6, 8`。

不是所有指令都能只看 opcode 決定完整操作；要依該指令格式一起檢查 funct3／funct7 或其他指定 bits。這張表只用於對應的 I-type 欄位，不能照搬去解 branch 等格式。

## 4 定址：算出「去哪裡拿」，再取得「裡面存什麼」

```asm
lw x5, 8(x6)
```

假設 x6 裡是 0x1000，而且題目指定那段記憶體可讀：

```text
先算有效位址：0x1000 + 8 = 0x1008
再從 0x1008 讀一個 32-bit word
假設那裡存 42，最後 x5 就得到 42
```

**x5 得到的是內容 42；0x1008 是用來找內容的位址。** 這就是暫存器基底加偏移的定址方式；這個 8 的單位是 bytes，不是八個 uint32_t。

相對地，`addi x5, x6, 8` 只做加法，沒有去讀記憶體。兩者即使用相同 rs1 與立即數，作用仍不同。

負偏移也要支援：`lw x5, -4(x6)` 的有效位址是 0x0FFC。立即數的 raw bits=0xFFC 代表 -4，不是正數 4092。

## 5 現在就能做的上機練習

打開 [decode.c](../../demo/isa-practice/decode.c)，在現有 **Docker 容器**執行：

```bash
cd /demo/isa-practice
mkdir -p bin
cc -std=c11 -Wall -Wextra decode.c -o bin/decode
./bin/decode
```

它用一般 C 程式解出 ADDI／LW 的欄位和組語，不需要 RISC-V 板子或交叉編譯器；也不是直接執行這些機器碼。

接著自己換指令值：

```bash
./bin/decode 0x00C30293 0x00830393 0xFFC32283
```

依序應看到立即數改成 12、rd 改成 x7，以及 LW 的偏移變 -4。再依 [實作題](../../demo/isa-practice/README.md) 補上有效位址的計算。

## 6 換成新題目，查手冊的順序

1. **指令寬度與範圍：**題目使用哪些 ISA／擴充？不要把所有指令都當 32 bits；壓縮指令可能是 16 bits。
2. **欄位圖：**opcode、暫存器、立即數在第幾個 bit；立即數是否分散在多段。
3. **符號與單位：**立即數要不要 sign extension；偏移是 bytes、指令數還是其他單位。
4. **執行效果：**改哪些暫存器、是否讀寫記憶體、PC 怎麼更新、哪些情況不合法。
5. **測試：**零、最大正偏移、負偏移、邊界與未支援指令；若輸入是 bytes，再確認如何組成指令 word。

RISC-V 的 x0 固定為 0，寫入不會改變它；若題目要求實作執行器，要把這種規則一起做進去。這份練習只是兩種指令的解碼器，沒有完整 CPU 狀態或 RAM。

讀新題目時，若遇到 `.word`／十六進位指令陣列、opcode、decode、register file、PC 等字眼，就回來用這份表追資料。看到它們不代表一定要重寫整顆 CPU，先確認題目要求的指令範圍。

## 7 PIO 也有編碼，但格式不同

現有 `out x, 32` 在沒有 delay／side-set 的這個例子，組譯成 `0x6020`：

| bits | 解讀 |
|---|---|
| 15..13 = 011 | OUT 指令 |
| 12..8 = 0 | 此例沒有額外 delay／side-set |
| 7..5 = 001 | 目的地是 X |
| 4..0 = 00000 | OUT 的位數編碼中，0 代表 32 bits |

這裡的 0 不是搬零個 bit。不同指令的欄位含義要分別查；PIO 的定址、跳躍與移位規則也不直接套 CPU 的 LW 規則。接著看 [PIO 的資料流與實作](PIO.md)。

驗證：六筆 ADDI／LW 的機器碼與 Clang RV32I 組譯結果一致；C 解碼器已在容器編譯執行，負立即數邊界、錯誤輸入與未支援指令均有檢查。沒有執行 RISC-V 韌體或實板測試。

來源：[RISC-V RV32I 規格](https://docs.riscv.org/reference/isa/v20260120/unpriv/rv32.html)、[官方指令編碼定義](https://github.com/riscv/riscv-opcodes/blob/master/extensions/rv_i)、[RP2350 CPU 選項](https://www.raspberrypi.com/products/rp2350/)、[PIO 編碼原始碼](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_pio/include/hardware/pio_instructions.h)。範例只涵蓋標明的指令，不代表考場完整 ISA。

[回技術目錄](README.md)
