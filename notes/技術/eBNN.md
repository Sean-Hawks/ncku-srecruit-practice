# eBNN：把模型當成一個有規格的運算函式

完整實作順序：[RP2350／eBNN 從環境、單板到多板優化](RP2350-eBNN從單板到多板.md)。這份集中查模型介面與位元直覺。

**eBNN 是 Embedded Binarized Neural Networks，目的是讓記憶體很少的裝置也能做神經網路推論。** 二值化讓許多值能用 1 bit 表示，eBNN 再減少中間結果占用的空間。

推論＝輸入資料交給已訓練的模型，算出預測；訓練＝調整模型權重。不要因為看到模型就先重做訓練。

本 repo 沒附 eBNN 模型，以下是延伸速查，不能直接在 /demo 呼叫出模型。

## 上機先找五件事

在題目 repo 根目錄搜尋，沒有 rg 就用編輯器搜尋：

```bash
rg -n 'ebnn_compute|predict|inference|input_shape' .
rg --files -g '*.h' -g '*.c' -g '*model*' -g '*test*' -g '*mnist*'
```

| 要找到 | 為什麼 |
|---|---|
| 函式宣告、官方 example | 真正怎麼呼叫，不憑名字猜參數 |
| 輸入大小與型別 | 784 個像素、98 個打包 bytes、float 陣列是不同格式 |
| 前處理 | 是否二值化、正規化、位元順序，依模型規格 |
| 輸出大小與型別 | 一個類別編號或多個分數，不能混用 |
| 標準測資與預期答案 | 先確定算對，才接板子與 ROS |

常見接法是：**ROS 收資料 → 符合模型格式的 buffer → 呼叫推論 → 包成結果訊息。** 模型吃圖片陣列時，不能直接拿目前 Int32 當成完整輸入。

<details>
<summary>原始 eBNN 專案的呼叫範例怎麼讀</summary>

原作者 README 有 `simple_mnist.h`、`ebnn_compute(input, output)` 的例子。模型產生的 header 決定輸入輸出，不是所有 eBNN 版本都固定同一個介面。

若題目真的提供相同模型介面，接法大致是：

```c
/* 示意片段：需先 include 題目模型 header，並備妥符合模型規格的 input。 */
uint8_t output[1];              // 只有確認模型輸出是一個 byte 才這樣配
ebnn_compute(input, output);
printf("%u\n", (unsigned)output[0]);
```

先依題目 Makefile 編譯；不要從這三行推定任意模型都能接受任意 input。

</details>

## 二值運算的直覺：一次比較很多個 bits

兩組二值資料比較：相同為 1，不同為 0，可用 XNOR，再數出 1 的數量。這是理解 BNN 運算的簡化例子，不是一個完整模型。

```text
a            1010
b            1100
相同的位置    1001 → 兩個位置相同
```

存成 `bnn_bits.c`，在 **Linux 有 C 編譯器的環境**可直接跑：

```c
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    uint32_t a = 0xA, b = 0xC;
    uint32_t equal = ~(a ^ b) & 0xFu; // 只比較四位，遮掉高位的 1
    unsigned same = 0;
    for (unsigned i = 0; i < 4; ++i)
        same += (equal >> i) & 1u;
    int dot = 2 * (int)same - 4;      // 用 +1/-1 表示時的四項內積
    assert(equal == 9 && same == 2 && dot == 0);
    printf("equal=%X same=%u dot=%d\n", (unsigned)equal, same, dot);
}
```

```bash
cc -std=c11 -Wall -Wextra bnn_bits.c -o bnn_bits
./bnn_bits
```

應印 `equal=9 same=2 dot=0`。將 b 改成 0xA，預期 same=4、dot=4，assert 也要配合修改。這裡四項內積成立的前提是每 bit 代表同一套 +1/-1 編碼；完整模型還有層結構、threshold 等設定。

## 正確後再加速

分開量：資料前處理、傳輸、推論、回傳。若板子運算只花 1ms，傳輸花 100ms，先拆更多運算核心不一定有效。多板分工要附 request ID，結果可能亂序；輸入 buffer 在運算完成前不能被重用覆寫。

來源：[原作者專案與模型使用方式](https://github.com/kunglab/ebnn)、[論文](https://arxiv.org/abs/1709.02260)、[eBNN 位元運算實作](https://github.com/kunglab/ebnn/blob/master/c/ebnn.h)。

[回技術目錄](README.md) · [位元運算](位元運算.md) · [效能量測](效能量測與除錯.md)
