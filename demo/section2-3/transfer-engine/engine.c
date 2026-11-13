/* 引擎的實作。讀 main.c 的工作單後，直接看這份的 run_job。
 * 它仍然是讀來源、寫目的的迴圈；差別是 main 把工作交給另一條 thread。
 * 真正 DMA 由硬體搬運；本模擬 thread 仍會使用 CPU。
 * 示範限制：busy 使用 volatile，但這不提供 thread 同步；正式程式需改用
 * atomic/鎖等正確機制。此處保留原實作，只補閱讀註解。
 */
#include "engine.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 參數或使用順序不對時印錯誤並結束；不是回傳可繼續處理的錯誤碼。
static void die(const char *what) {
  fprintf(stderr, "\n*** transfer engine refused the job: %s\n", what);
  exit(1);
}

// 每個通道的內部狀態：是否占用、是否設定、是否忙碌、工作單、thread 控制代號。
// 這是模擬器自己的管理資料，不是來源/目的陣列。
typedef struct {
  bool         claimed;
  bool         configured;
  // volatile 不保證原子性或執行緒之間的記憶體可見性；不能照抄為同步方法。
  volatile bool busy;
  engine_job_t job;
  pthread_t    th;
} channel_t;

// 全域四個通道。未指定初值的 static 物件會先清零：一開始都未占用。
static channel_t chans[ENGINE_CHANNELS];

// ch 是陣列索引。先檢查 0..3，再確認呼叫者先做過 claim。
static void check_channel(int ch) {
  if (ch < 0 || ch >= ENGINE_CHANNELS) die("no such channel");
  if (!chans[ch].claimed)             die("that channel was never claimed");
}

// 尋找未占用的通道。完整的多個呼叫端並行管理還需要額外保護。
int engine_claim(void) {
  for (int i = 0; i < ENGINE_CHANNELS; i++) {
    if (!chans[i].claimed) { chans[i].claimed = true; return i; }
  }
  return -1;   // all four in use -- did you forget to release one?
}

// job 指向 main 傳進來的工作單。job->count 等同 (*job).count。
void engine_configure(int ch, const engine_job_t *job) {
  check_channel(ch);
  if (chans[ch].busy) die("the channel is still running the previous job");

  if (job->size != XFER_8 && job->size != XFER_16 && job->size != XFER_32)
    die("transfer size must be 1, 2 or 4 bytes");
  if (job->count == 0)
    die("count is zero -- count is the number of transfers, not bytes");
  // 對齊：例如每筆 4 bytes，位址必須是 4 的倍數；% 是取餘數。
  // 這裡檢查大小、筆數與對齊，但不知道 buffer 長度，不會替你檢查陣列邊界。
  if (((uintptr_t)job->src % (uintptr_t)job->size) != 0)
    die("source address is not aligned to the transfer size");
  if (((uintptr_t)job->dst % (uintptr_t)job->size) != 0)
    die("destination address is not aligned to the transfer size");

  // 複製整張 struct 工作單；其中 src/dst 仍指向 main 的同一組資料。
  // 搬運完成前，來源/目的 buffer 都必須仍然有效，避免衝突讀寫。
  chans[ch].job = *job;
  chans[ch].configured = true;
}

// 真正的搬運從這裡開始，由 pthread_create 啟動的工作 thread 執行。
static void *run_job(void *arg) {
  // arg 是該通道狀態的位址；c->job 就是先前設定的工作單。
  channel_t *c = (channel_t *)arg;
  // 使用 byte 指標：在這個環境 s+1 前進 1 byte，方便處理 1/2/4-byte 傳輸。
  // s、d 是目前讀寫位置；修改這些指標不會自動改變記憶體內容。
  const unsigned char *s = (const unsigned char *)c->job.src;
  unsigned char       *d = (unsigned char *)c->job.dst;
  // XFER_32 的數值是 4，所以 step=4，接下來每次真的複製 4 bytes。
  size_t step = (size_t)c->job.size;

  // 你問「和平常 for 搬有什麼不同」：這裡就是 for，main 沒做的讀寫由它做。
  // count 是這個迴圈執行的次數，因此不能填 byte 總數。
  for (size_t i = 0; i < c->job.count; i++) {
    // memcpy(目的,來源,byte數)：真的讀 s 指向的資料，寫進 d 指向的記憶體。
    // 若只寫 d=s，只會改變 d 指向哪裡，不會在原目的記憶體產生副本。
    memcpy(d, s, step);
    // copy：兩端都 +=4。fill：s 留在 pattern，只有 d +=4。
    if (c->job.src_increment) s += step;
    if (c->job.dst_increment) d += step;
    // 故意要求短暫暫停，讓主程式有時間觀察 busy；不是高速 DMA 的速度模擬。
    struct timespec nap = { 0, 2000 };
    nanosleep(&nap, NULL);
  }
  // 告知主程式「搬完了」。此原始旗標有 data race，上方已註明限制。
  c->busy = false;
  return NULL;
}

// 先要求 claim、configure，避免沒有描述就啟動，或同一通道重複開工。
void engine_start(int ch) {
  check_channel(ch);
  if (!chans[ch].configured) die("nothing was configured on that channel yet");
  if (chans[ch].busy)        die("that channel is already running");

  chans[ch].busy = true;
  // 建立另一條 thread，執行 run_job(&chans[ch])；main 隨後可以繼續往下跑。
  // 此 demo 未處理 pthread_create 失敗；正式實作需要檢查回傳碼。
  pthread_create(&chans[ch].th, NULL, run_job, &chans[ch]);
}

// main 的 while(engine_busy(ch)) 查的是此旗標，並不是在這裡搬資料。
bool engine_busy(int ch) { check_channel(ch); return chans[ch].busy; }

// pthread_join 等待並回收工作 thread；即使 busy 已經 false，仍需要收尾。
void engine_wait(int ch) {
  check_channel(ch);
  if (chans[ch].busy || chans[ch].th) pthread_join(chans[ch].th, NULL);
  chans[ch].th = 0;
}

// 呼叫者先 wait，再 release；清除占用與設定，之後才能重新 claim。
void engine_release(int ch) {
  check_channel(ch);
  if (chans[ch].busy) die("release while the channel is still running");
  chans[ch].claimed = chans[ch].configured = false;
}
