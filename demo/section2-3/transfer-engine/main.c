/* 先看這份，再看 engine.h，最後看 engine.c 的 run_job。
 *
 * main.c 填「搬運工作單」；engine.c 裡的 thread 真的逐筆讀寫。
 * 一般只複製陣列可直接 memcpy；這份是在練 configure → start → wait → 驗證。
 * 它用 thread 模擬 DMA，仍消耗 CPU，沒有宣稱一定比 memcpy 快。
 *
 * 在 /demo/section2-3：make run-transfer-engine；加錯誤示範：make mistake。
 * C 改完需重新編譯；make 會根據檔案更新時間決定是否重建執行檔。
 */
// 讀入工作單型別與函式宣告。函式本體在 engine.c，靠 Makefile 一起編譯連結。
#include "engine.h"

#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// 可改：正常要搬的「筆數」。保留你目前的 N=8；原版為 4096。
// N=8 且每筆 4 bytes → 正常搬 32 bytes；不必把 count 也乘以 4。
#define N 8
// 為 make mistake 預留四倍空間：錯把 byte 數當筆數，會搬四倍的量。
// 保留這個額外空間，才能觀察「超過原定範圍」而不超出實際陣列。
#define ROOM (4 * N)

// src、dst 是兩塊不同的陣列；每個 uint32_t 在此環境佔 4 bytes。
// dst=src 在 C 不能用於整個陣列；若改成指標賦值，也不會複製資料。
static uint32_t src[ROOM], dst[ROOM];

// 每個示範前重新準備來源與目的，避免上一個示範的資料影響驗證。
static void reset(void) {
    for (int i = 0; i < ROOM; i++) {
        // 產生可區別的測試數值，乘數只是資料圖樣，不是 DMA 的特殊規則。
        src[i] = (uint32_t)(i * 2654435761u);
        dst[i] = 0;
    }
}

// argc 是命令列參數個數；argv 是參數文字。
// ./bin/transfer-engine --mistake → argc=2，argv[1] 是 "--mistake"。
int main(int argc, char **argv) {
    // 取得一個通道，此處通常為 0。完整應用需處理回傳 -1 的無通道情況。
    // "claimed channel 0 (there are 4)" 只是在說通道編號，不是搬運大小。
    int ch = engine_claim();
    printf("claimed channel %d (there are %d)\n\n", ch, ENGINE_CHANNELS);

    // 1. copy：src[0]→dst[0]、src[1]→dst[1]，兩端一起往下一格走。
    reset();
    // engine_job_t 定義在 engine.h。
    // 左邊 .src 是工作單的欄位；右邊 src 是此陣列轉成的起始位址。
    // 這裡只是記錄「從哪裡搬」，資料尚未被複製。
    engine_job_t copy = {
        .src = src,
        .dst = dst,
        .count = N,            // 可改：搬 N 筆；不是 N bytes。
        .size = XFER_32,       // 每筆 32 bits = 4 bytes，定義在 engine.h。
        .src_increment = true, // 每次讀來源下一格：src[0]、src[1]……
        .dst_increment = true, // 每次寫目的下一格：dst[0]、dst[1]……
    };
    // &copy 是工作單的位址，讓 configure 能讀取它的欄位。
    // configure 複製設定；start 才建立另一條 thread 開始搬。
    engine_configure(ch, &copy);
    engine_start(ch); // 函式返回，只代表已啟動，不代表 dst 已填好。

    // 搬運 thread 在工作時，main 這條 thread 只做「計數」示範。
    // 換成自己的獨立工作也可以；使用 dst 的搬運結果前必須等完成。
    unsigned long spins = 0;
    while (engine_busy(ch)) {
        spins++; // 這不是搬幾筆！一億多次是主程式計數次數，每次跑都會變。
        // 偶爾讓出 CPU 的執行機會。0xffff 是低 16 位遮罩，
        // 條件每 65536 次成立一次；這段不是複製資料的迴圈。
        if ((spins & 0xffff) == 0)
            sched_yield();
    }
    // busy 變 false 後仍 join 工作 thread，收尾後才檢查陣列。
    engine_wait(ch);

    // memcmp 比較前 N*4 bytes；回傳 0 表示這段內容相同。
    // 此處需要 bytes，所以 N*sizeof(uint32_t) 正確；與 count 的單位不同。
    printf("1. copy %d words          %s\n", N,
           memcmp(src, dst, N * sizeof(uint32_t)) == 0 ? "dst matches src" : "MISMATCH");
    printf("   while it ran, this program went round its own loop %lu times\n\n",
           spins);

    // 2. fill：每次都從同一個 pattern 讀，依序寫入 dst 各格。
    reset();
    // 可改：想填 0 可將 pattern 改成 0；下方「every word...」是固定文字，
    // 若改 pattern，記得一起改印出的文字，否則提示會與內容不一致。
    uint32_t pattern = 0xA5A5A5A5u;
    // &pattern 指向單一數值；來源不往前，才會反覆填相同值。
    // 不可只把 src_increment 改成 true，因為 pattern 不是一整排來源陣列。
    engine_job_t fill = {
        .src = &pattern,
        .dst = dst,
        .count = N,
        .size = XFER_32,
        .src_increment = false, // 每次都讀同一個 pattern。
        .dst_increment = true,  // 目的每次前進 4 bytes。
    };
    engine_configure(ch, &fill);
    engine_start(ch);
    engine_wait(ch);

    // 這個 for 是「驗證」每格值正確；真正填值已由 engine.c 做完。
    // found mismatch 時將 filled 設為 0，break 提前結束檢查。
    int filled = 1;
    for (int i = 0; i < N; i++)
        if (dst[i] != pattern) {
            filled = 0;
            break;
        }
    printf("2. fill %d words          %s\n", N,
           filled ? "every word is 0xA5A5A5A5" : "MISMATCH");
    printf("   same engine, same count -- only the increment flags changed\n\n");

    // 已 wait 並驗證完成，才歸還通道，讓之後的工作能再次取得。
    engine_release(ch);

    // 3. 故意填錯：make mistake 會執行 ./bin/transfer-engine --mistake。
    // 前兩個示範照常執行，下面只在帶這個參數時追加。
    if (argc > 1 && strcmp(argv[1], "--mistake") == 0) {
        reset();
        ch = engine_claim();
        // 這裡複製的是 struct 工作單。struct 賦值在 C 合法，
        // 會複製裡面的位址與設定；不會額外複製 src、dst 陣列內容。
        engine_job_t wrong = copy;
        // sizeof(uint32_t)=4：32 bits / 每 byte 8 bits = 4 bytes。
        // 故意錯誤：N=8 時算出 32，卻填進「筆數」，引擎就搬 32 筆＝128 bytes。
        // 可改：改成 wrong.count = N; 就修正了錯誤，應只搬 8 筆＝32 bytes。
        // 這裡保留錯誤，方便你比較修正前後；size 已經設定每筆 4 bytes。
        wrong.count = N * sizeof(uint32_t);
        engine_configure(ch, &wrong);
        engine_start(ch);
        engine_wait(ch);
        engine_release(ch);

        // 找出最後一個非零位置，估計寫到第幾格；不是算非零元素的總數。
        // 這依賴目前 reset 產生的測試資料；若改資料圖樣，可能需要改驗證方法。
        size_t touched = 0;
        for (size_t i = 0; i < ROOM; i++)
            if (dst[i] != 0)
                touched = i + 1;
        // 原錯誤版 N=8：搬 32 筆，比原定 8 筆多 24 筆。
        // 若修正 count=N：這兩個數字應變成搬 8 筆、多 0 筆。
        printf("3. count given as %zu (the byte count) instead of %d\n",
               (size_t)(N * sizeof(uint32_t)), N);
        printf("   -> the engine moved %zu words and wrote %zu past the %d you meant\n",
               touched, touched - N, N);
        printf("   nothing complained: the engine did exactly what you described.\n");
    } else {
        printf("3. run `make mistake` to see what happens when count is given\n");
        printf("   in bytes instead of transfers.\n");
    }
    return 0;
}
