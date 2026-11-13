# C 記憶體與指標：值、位址、內容複製

**變數存值；指標存位址；複製指標和複製該位置的資料是兩件事。**

## 你問過：為什麼 sizeof(uint32_t) 是 4

在這個平台：1 byte = 8 bits；uint32_t 是 32 bits，所以佔 4 bytes。

```text
uint32_t a[3]
假設 a[0] 在位址 1000：
a[0]：1000..1003
a[1]：1004..1007
a[2]：1008..1011
```

uint32_t 指標 `p+1` 前進一個元素，也就是這裡的 4 bytes；不是前進 1 byte。一般 C 的 byte 大小由 CHAR_BIT 表示，不能把所有平台都當成 8。

## 可直接跑：指向同一份 vs 複製成另一份

存成 `memory_check.c`：

```c
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    uint32_t src[3] = {10, 20, 30};
    uint32_t dst[3] = {0};
    uint32_t *p = src;                // p 指向 src[0]，沒有搬內容
    p[0] = 99;                       // 就是改 src[0]
    assert(src[0] == 99 && dst[0] == 0);

    memcpy(dst, src, sizeof src);     // 第三參數是總 bytes，不是元素數
    src[0] = 7;
    assert(dst[0] == 99 && src[0] == 7); // 複製後是不同儲存空間
    assert(p + 1 == &src[1]);
    printf("CHAR_BIT=%d element=%zu array=%zu pointer=%zu\n",
           CHAR_BIT, sizeof src[0], sizeof src, sizeof p);
    printf("src[0]=%u dst[0]=%u\n", (unsigned)src[0], (unsigned)dst[0]);
}
```

```bash
cc -std=c11 -Wall -Wextra memory_check.c -o memory_check
./memory_check
```

目前 archlinux 應是 CHAR_BIT=8、element=4、array=12；pointer 大小依執行平台，不能假設等於 4。第二行應是 `src[0]=7 dst[0]=99`。

## 常見寫法對照

| 寫法 | 意思 |
|---|---|
| `&x` | 取得 x 的位址 |
| `*p` | 存取 p 指向的物件 |
| `p = src` | p 指向 src 第一個元素，不複製陣列 |
| `dst = src` | dst/src 都是陣列時，不合法 |
| `memcpy(dst, src, bytes)` | 複製內容，區域不可重疊 |
| `memmove(dst, src, bytes)` | 複製內容，允許重疊 |
| `memcmp(a, b, bytes) == 0` | 指定範圍的 bytes 全部相同 |
| `job2 = job1` | struct 可以賦值；其中指標只複製位址，不複製指向的 buffer |

`sizeof array / sizeof array[0]` 只能在這裡仍是實際陣列時算元素數。函式參數 `void f(uint32_t a[])` 裡的 a 已調整成指標，`sizeof a` 是指標大小；另傳 count。

## 上機最常出錯的三件事

1. **越界：**長度 N 的陣列只可存取 0..N-1。配置容量至少要有 count × 每筆 bytes。
2. **生命週期：**thread／DMA 還沒完成，區域陣列就離開函式，或來源被改寫。
3. **誤認內容：**`memset(dst, 1, sizeof dst)` 設的是每個 byte=1，不是每個 uint32_t=1；此平台會得到 0x01010101。多 byte 填值用迴圈或正確的引擎設定。

傳輸協定不要直接假設 C struct 的記憶體排列等於線上的封包；可能有 padding 與 byte 順序差異，見 [序列封包](序列通訊與封包.md)。

來源：[C11 草案，sizeof／陣列參數／string functions](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)。

[回技術目錄](README.md) · [資料搬運](資料搬運.md)
