/* 自編練習：把 RV32I 的 32-bit 機器碼解碼成 ADDI 或 LW。
 * 這是「在一般電腦跑的 C 解碼器」，不是把指令交給電腦 CPU 執行。
 * 只示範這兩種 I-type 指令；不是完整 RISC-V 模擬器。
 */
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static bool decode(uint32_t word) {
    uint32_t opcode = word & 0x7Fu;         // bits 6..0：主要操作分類。
    uint32_t funct3 = (word >> 12) & 7u;    // bits 14..12：再細分操作。
    bool addi = opcode == 0x13u && funct3 == 0;
    bool lw = opcode == 0x03u && funct3 == 2;
    if (!addi && !lw) {
        fprintf(stderr, "0x%08" PRIX32 ": this exercise only decodes RV32I ADDI/LW\n", word);
        return false;
    }

    uint32_t rd = (word >> 7) & 0x1Fu;      // bits 11..7：寫入哪個暫存器。
    uint32_t rs1 = (word >> 15) & 0x1Fu;    // bits 19..15：讀哪個暫存器。
    uint32_t raw_imm = (word >> 20) & 0xFFFu; // bits 31..20：12-bit 立即數。
    // 立即數是有號的：低 11 bits 權重為正，最高位權重是 -2048。
    // 例如 0xFFC：2044 - 2048 = -4，不是正數 4092。
    int32_t imm = (int32_t)(raw_imm & 0x7FFu) - (int32_t)(raw_imm & 0x800u);

    printf("word=0x%08" PRIX32 " opcode=0x%02" PRIX32
           " rd=%" PRIu32 " rs1=%" PRIu32 " funct3=%" PRIu32 " imm=%" PRId32 "\n",
           word, opcode, rd, rs1, funct3, imm);
    if (addi)
        printf("asm: addi x%" PRIu32 ", x%" PRIu32 ", %" PRId32 "\n", rd, rs1, imm);
    else
        printf("asm: lw x%" PRIu32 ", %" PRId32 "(x%" PRIu32 ")\n", rd, imm, rs1);
    return true;
}

int main(int argc, char **argv) {
    if (argc == 1) {
        const uint32_t examples[] = {0x00830293u, 0x00832283u, 0xFFC30293u};
        for (size_t i = 0; i < sizeof examples / sizeof examples[0]; ++i)
            if (!decode(examples[i])) return 1;
        return 0;
    }
    for (int i = 1; i < argc; ++i) {
        char *end;
        errno = 0;
        unsigned long long parsed = strtoull(argv[i], &end, 0);
        if (errno != 0 || end == argv[i] || *end != '\0'
            || argv[i][0] < '0' || argv[i][0] > '9' || parsed > UINT32_MAX) {
            fprintf(stderr, "invalid unsigned 32-bit instruction: %s\n", argv[i]);
            return 1;
        }
        if (!decode((uint32_t)parsed)) return 1;
    }
    return 0;
}
