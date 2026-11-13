/* 這是「型別與函式宣告」，讓 main.c 和 engine.c 使用相同規格。
 * main.c：決定要搬什麼。engine.c：實作怎麼搬。Makefile：把兩份 .c 編譯連結。
 * engine 只是作者的模組名稱；全部寫在同一份 main.c 也可以。
 * #include 讓編譯器讀到這些宣告，不會自動把 engine.c 編譯進來。
 */
/* include guard：同一編譯單元重複 include 時，不重複定義型別。 */
#ifndef ENGINE_H
#define ENGINE_H

#include <stdbool.h>
#include <stddef.h>

#define ENGINE_CHANNELS 4  // 可取得四個通道，編號是 0、1、2、3；不是每筆四 bytes。

/* 名稱的 8/16/32 是 bit 數；右邊的 1/2/4 才是每筆 byte 數。
 * XFER_32 = 4，因為 32 bits / 8 = 4 bytes。
 * 改 size 時，來源、目的物件的寬度與對齊也要配合，不能只改名字。
 */
typedef enum { XFER_8 = 1, XFER_16 = 2, XFER_32 = 4 } xfer_size_t;

/* 這張「工作單」只存位址、筆數與設定，建立它不會複製陣列。
 * C 陣列不能用 dst=src 整體賦值；指標 p=src 只會指向同一份資料。
 * 真正複製內容要做讀寫，像 memcpy(dst,src,總bytes)，或 engine.c 的迴圈。
 */
typedef struct {
  const void *src;            // 從這個位址讀；const 表示不經此指標修改來源。
  void       *dst;            // 往這個位址寫。void* 讓介面可接不同型別的物件。
  size_t      count;          // 可改：搬幾筆。填 N，不是 N*sizeof(uint32_t)。
  xfer_size_t size;           // 每筆幾 bytes；總 bytes = count * size。
  bool        src_increment;  // true 每筆讀下一格；false 每次讀同一個 pattern。
  bool        dst_increment;  // true 每筆寫下一格；false 重複寫同一位置。
} engine_job_t;

/* 先取得一個未被使用的通道；回傳編號，全部被占用則是 -1。 */
int  engine_claim(void);

/* 交工作單：檢查設定並複製描述，尚未開始搬。通道不可正在執行。 */
void engine_configure(int ch, const engine_job_t *job);

/* 啟動：請另一條 thread 執行搬運，這個函式會先返回主程式。 */
void engine_start(int ch);

/* 查是否仍忙碌。原實作的 volatile busy 未提供正確的跨 thread 同步，
 * 此介面用來示範流程；正式實作需用 atomic/鎖等同步機制。
 */
bool engine_busy(int ch);

/* 等待工作 thread 結束；使用目的陣列結果前呼叫。 */
void engine_wait(int ch);

/* wait 完再歸還通道，別人之後才能 claim。 */
void engine_release(int ch);

#endif // ENGINE_H
