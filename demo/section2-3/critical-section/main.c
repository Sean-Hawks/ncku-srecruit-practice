/* 目的：比較「沒鎖、只鎖共享更新、把耗時工作也鎖住」三種寫法。
 * 每個 worker 對同一個 counter 加 ROUNDS 次；預期總數是 WORKERS*ROUNDS。
 * 在 /demo/section2-3 執行 make run-critical-section。
 * 先讀三個 worker，再看 run 如何建立 thread、等結束並量時間。
 * 注意：原碼無鎖 counter 與短鎖版本的 sink 有 data race，保留作閱讀示範；
 * 不把某次跑對或輸出的耗時，當成整支程式正確同步的證明。
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <sched.h>
#endif

// 可改：工作者數量；下方 run 會依此建立 thread，這不是指定 CPU 核心數。
#define WORKERS  2
// 可改：每條 thread 加幾次。改成 5，理想總數就是 2*5=10。
// 測試正確性與比較鎖範圍時，保持相同 WORKERS、ROUNDS 與工作量。
#define ROUNDS   20000

// 共享全域變數，兩條 thread 都能讀寫。
// volatile 不會讓 counter++ 變原子操作，也不會提供 thread 同步。
static volatile long counter;
// spinlock 是兩條 thread 共同使用的同一把鎖；等待者會反覆檢查。
static pthread_spinlock_t lock;

// 模擬耗時運算；此函式的 x 是每次呼叫自己的區域值。
// 2000 次和兩個大常數只是製造工作量，不需要背；uint32_t 運算依寬度回繞。
static uint32_t work(uint32_t x) {
  for (int i = 0; i < 2000; i++) x = x * 1664525u + 1013904223u;
  return x;
}

// 接住運算結果，減少工作被最佳化刪除的機會。但 sink 本身是共享變數！
// 無鎖與短鎖 worker 在鎖外更新它，仍有 data race；正式程式應讓每個
// thread 個別累計後再彙整，或對共享更新做正確同步。
static uint32_t sink;

// 查這個程序能使用的 CPU 數；容器或 affinity 設定可能限制可用數量。
// 單核心也可能交錯執行；無鎖偶然正確，不表示這種寫法安全。
static long usable_cpus(void) {
#ifdef __linux__
  cpu_set_t set;
  if (sched_getaffinity(0, sizeof set, &set) == 0) return CPU_COUNT(&set);
#endif
  return sysconf(_SC_NPROCESSORS_ONLN);
}

// 1. 未保護 counter：用具體時序看，A、B 都讀到 0，再各自寫回 1。
// 這只是其中一種交錯；實際 C data race 為未定義行為，結果不保證。
static void *worker_unlocked(void *arg) {
  (void)arg;
  for (int i = 0; i < ROUNDS; i++) {
    sink += work(i);
    long v = counter;               // step 1: read
    counter = v + 1;                // steps 2 and 3: add, write back
                                    // the other worker can land in between
  }
  return NULL;
}

// 2. 保護 counter 的完整更新；獨立計算放鎖外。
// 此範例的 sink 仍有上述同步限制；重點先看 counter 的鎖範圍。
static void *worker_locked(void *arg) {
  (void)arg;
  for (int i = 0; i < ROUNDS; i++) {
    sink += work(i);                // the work stays OUTSIDE the lock
    // 同時使用這把鎖的其他 worker，必須等 unlock 才能做自己的更新。
    pthread_spin_lock(&lock);
    counter++;                      // critical section: one line
    pthread_spin_unlock(&lock);
  }
  return NULL;
}

// 3. 同一把鎖涵蓋 work 與 counter；另一條 thread 連獨立計算都要等。
// 可練習：對照第 2 種，把不依賴共享狀態的計算移出臨界區。
static void *worker_locked_too_long(void *arg) {
  (void)arg;
  for (int i = 0; i < ROUNDS; i++) {
    pthread_spin_lock(&lock);
    sink += work(i);                // the work is now INSIDE the lock
    counter++;                      // ... so only one worker runs at a time
    pthread_spin_unlock(&lock);
  }
  return NULL;
}

// fn 是要選哪種 worker，就像把 Python callback 傳給另一個函式。
// run 負責清零、開始計時、建立 thread、等待全部結束、計算毫秒數。
static double run(void *(*fn)(void *)) {
  struct timespec t0, t1;
  pthread_t th[WORKERS];

  counter = 0;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  // create 讓工作者開始執行；不會先等第一個做完才建立第二個。
  for (int i = 0; i < WORKERS; i++) pthread_create(&th[i], NULL, fn, NULL);
  // join 等每個工作者結束，所以之後讀 counter 才是本次最終結果。
  for (int i = 0; i < WORKERS; i++) pthread_join(th[i], NULL);
  clock_gettime(CLOCK_MONOTONIC, &t1);

  // 秒差換成毫秒，加上奈秒差換成毫秒，作為本次耗時。
  return (t1.tv_sec - t0.tv_sec) * 1e3 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
}

int main(void) {
  // 先初始化鎖；使用結束後，最後的 pthread_spin_destroy 做清理。
  pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);

  long cpus = usable_cpus();
  printf("expected counter = %d   (%ld CPU%s available)\n",
         WORKERS * ROUNDS, cpus, cpus == 1 ? "" : "s");
  if (cpus < 2)
    printf("NOTE: with only one CPU the two workers rarely overlap, so line 1\n"
           "      below may come out correct. That does not make it safe.\n");
  printf("\n");

  // 依序跑三次獨立測試，每次 run 都重新把 counter 清成 0。
  // 輸出的 ok 只檢查數值是否等於預期，不證明沒有 data race。
  double ms = run(worker_unlocked);
  printf("1. no lock            counter = %-7ld  %s   %6.1f ms\n",
         counter, counter == WORKERS * ROUNDS ? "ok  " : "LOST UPDATES", ms);

  ms = run(worker_locked);
  printf("2. lock, short hold   counter = %-7ld  %s   %6.1f ms\n",
         counter, counter == WORKERS * ROUNDS ? "ok  " : "LOST UPDATES", ms);

  ms = run(worker_locked_too_long);
  printf("3. lock, long hold    counter = %-7ld  %s   %6.1f ms\n",
         counter, counter == WORKERS * ROUNDS ? "ok  " : "LOST UPDATES", ms);

  printf("\nrun `make run-critical-section` a few times -- line 1 changes, lines 2 and 3 do not.\n");
  pthread_spin_destroy(&lock);
  return 0;
}
