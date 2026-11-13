/* 讀法：先看 x 的值，再看每個運算保留或改變哪些位置。
 * 可改：先只把 x 改成 0x3C，手算再重新編譯；下方註解答案以原版 x=0xA5 為準。
 * 指令在 container 的 /demo/section2-2：cc demo_asm.c -o demo_asm && ./demo_asm
 * 這是 C 程式，不是 CPU 組合語言；檔名 asm 不代表這裡有組語。
 */
#include <stdio.h>
#include <stdint.h>

int main(void) {
    /* 0x 表示十六進位：A=1010，5=0101，所以 A5=1010 0101。
     * uint32_t：不帶正負號的 32-bit 整數；此環境 8 bits=1 byte，
     * 因此 sizeof(uint32_t)=32/8=4 bytes。sizeof 回傳的是 byte 數。
     * printf 的 %X 用十六進位顯示；%02X 至少兩位、不足補零；%u 顯示十進位。
     */
    uint32_t x = 0xA5;                              /* 1010 0101 */

    /* 下列都是「計算後印出」，沒有 x = ...，所以不會改變 x 本身。 */
    /* 六個運算子 */
    /* AND：兩邊同一位都是 1 才留下 1。
     * A5 & 0F：1010 0101 & 0000 1111 = 0000 0101。
     * 可改：把整行標籤和運算的 0x0F 一起改成 0xF0，會得到 0xA0；
     * 再算 (x & 0xF0) >> 4，便得到高四位的值 0x0A。
     */
    printf("x & 0x0F = 0x%02X\n", x & 0x0F);        /* 0x05  AND：只留低 4 位 */
    /* OR：任一邊是 1 就設成 1；適合設定位元，不能把舊的 1 清成 0。 */
    printf("x | 0x0F = 0x%02X\n", x | 0x0F);        /* 0xAF  OR ：低 4 位全設 1 */
    /* XOR：兩邊不同才是 1；與 1 XOR 會反轉該位，與 0 XOR 則保留。 */
    printf("x ^ 0xFF = 0x%02X\n", x ^ 0xFF);        /* 0x5A  XOR：每一位都反過來 */
    /* ~x 反轉整個 32-bit 值；&0xFF 才把結果限制在低八位顯示。 */
    printf("~x       = 0x%02X\n", ~x & 0xFF);       /* 0x5A  NOT */
    /* 左移四位：A5 → A50；x 是 32-bit，沒有被截成只有八位。 */
    printf("x << 4   = 0x%X\n",   x << 4);          /* 0xA50 左移 4 位 */
    /* 右移四位：A5 → A；這裡是 unsigned 值，高位補 0。 */
    printf("x >> 4   = 0x%X\n",   x >> 4);          /* 0xA   右移 4 位 */

    /* bit 編號從右邊的 0 開始。1u<<n 建立「只有第 n 位為 1」的遮罩。
     * 可改：n 必須在 0..31；不可用 1u<<32 建立 32-bit 遮罩。
     */
    /* 常見用法 */
    printf("set bit 1    = 0x%02X\n", x | (1u << 1));    /* 0xA7 */
    printf("clear bit 0  = 0x%02X\n", x & ~(1u << 0));   /* 0xA4 */
    printf("test bit 7   = %u\n",     (x >> 7) & 1);     /* 1 */
    /* 抽欄位：先移到最右邊，再 &0xF 去掉其他位元。 */
    printf("bits 7..4    = 0x%X\n",   (x >> 4) & 0xF);   /* 0xA  先右移再 mask */
    /* 打包：12 左移八位留出低八位，再放入 34，形成 0x1234。
     * 要「替換」既有欄位需先清空再放入，例如把 A5 高四位改為 3：
     * (x & ~0xF0u) | ((3u & 0xFu)<<4) → 0x35，低四位仍是 5。
     */
    printf("combine      = 0x%X\n",   (0x12u << 8) | 0x34); /* 0x1234 */

    return 0;
}
