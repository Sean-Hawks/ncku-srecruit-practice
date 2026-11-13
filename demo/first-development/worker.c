/* 自編：沒有板子時，用獨立 C 程序代替板子端的運算。
 * 這不是 ESP32／RP2350 模擬器；此程式跑在 Linux CPU。
 * 概念：收工作 → 計算 → 回傳。未來板子也要完成這三件事。
 */
#include <stdint.h>
#include <stdio.h>

// 第一輪只改這個函式。原規格：0..1000000 的輸入，算 2*x+1。
static uint32_t compute(uint32_t input) {
    return input * 2u + 1u;
}

// 線上格式規定 little-endian：第一個 byte 是最低的八位。
// 不直接把 bytes 轉成 uint32_t*，避免對齊與主機 endian 的依賴。
static uint32_t load_le32(const unsigned char *p) {
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

static void store_le32(unsigned char *p, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
        p[i] = (unsigned char)((value >> (8u * i)) & 0xFFu);
}

int main(void) {
    unsigned char request[8], reply[8];
    size_t received;

    // stdin 收 binary：前四 bytes 是工作 ID，後四 bytes 是輸入數字。
    // fread 沒湊到八 bytes 時會等待；發送端關閉時才可能得到不足八 bytes。
    while ((received = fread(request, 1, sizeof request, stdin)) != 0) {
        if (received != sizeof request) {
            fputs("worker: incomplete 8-byte request\n", stderr);
            return 1;
        }
        uint32_t id = load_le32(request);
        uint32_t input = load_le32(request + 4);
        if (input > 1000000u) {
            fputs("worker: input must be 0..1000000\n", stderr);
            return 1;
        }
        uint32_t answer = compute(input);
        store_le32(reply, id);          // 把同一個工作編號帶回，避免認錯答案。
        store_le32(reply + 4, answer);

        // stdout 是 binary 通道，不能混入 printf("debug...")。
        // 錯誤文字放 stderr；Python 只從 stdout 收八 bytes。
        if (fwrite(reply, 1, sizeof reply, stdout) != sizeof reply
            || fflush(stdout) == EOF) { // flush：立刻送出，不留在 C 輸出緩衝區。
            fputs("worker: cannot send reply\n", stderr);
            return 1;
        }
    }
    if (ferror(stdin)) {
        fputs("worker: input read failed\n", stderr);
        return 1;
    }
    return 0;  // 主機關閉輸入，工作完成。
}
