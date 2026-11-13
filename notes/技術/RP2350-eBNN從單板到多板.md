# 多板與 eBNN：歷屆情境查表

[← 回考場筆記入口](../README.md) · [平行運算](平行運算.md) · [封包](序列通訊與封包.md)

**這頁保留先前討論的多板情境。今年的板型與演算法尚未確認。** RP2350 的建置查 [Pico SDK](RP2350與PicoSDK.md)，ESP32 查 [ESP32](ESP32.md)，模型格式查 [eBNN](eBNN.md)。

## 從一塊變多塊，增加的是誰的責任

```text
主機：讀輸入 → 配 job_id → 選板子 → 傳送 → 收回 → 按 job_id 放回正確位置
                                ↙          ↘
板 A：收完整輸入 → 計算 → 回 id＋結果     板 B：同樣流程、另一份工作
```

板子各自有 CPU、RAM、韌體，不能直接用指標讀另一板的陣列。主機負責通訊與分派；多核心則是在同板共享 RAM 的另一種分工。

## 已解讀的分派例子

假設有獨立圖片 0..4、兩塊板：

| 板子 | 工作 | 回傳 |
|---|---|---|
| A | 0、2、4 | 每筆各附原 job_id 與分類 |
| B | 1、3 | 每筆各附原 job_id 與分類 |

先回來的可能是 1，不是 0。主機用 `results[job_id]` 放回位置，記錄已完成狀態，最後確認 0..4 都收到一次。結果要依題目合併；分類通常維持各筆，不能相加。

## 工作分配方式

| 方式 | 適合 | 代價 |
|---|---|---|
| 固定切塊／輪流分配 | 每筆計算時間相近 | 慢板可能拖住整批 |
| 空閒板再領工作 | 每筆時間不同 | 需管理 pending、busy、done 與錯誤 |
| 分不同演算法到各板 | 題目要求各種獨立工作 | 最慢的一路可能決定總時間 |

去年心得提過排序、SHA-0、CRC32，屬個人回憶。若正式題目也出現，**精確版本、參數與測資以當年 PDF／正式勘誤為準**，不能把 SHA-0 當 SHA-256。

## 同板分收訊與運算

```text
收訊端：收完整一筆 → 標記 buffer 可用 → 放入工作 queue
運算端：取得工作 → 計算 → 交回結果 → 歸還 buffer
```

queue 若傳指標，只傳了位址，內容仍在原 buffer。運算端用完之前，收訊端不能拿它裝下一筆。常見做法是明確所有權、兩個 buffer 或 buffer pool；具體 API 依 SDK。

同一模型若用全域可變暫存區，兩個核心同時推論可能互相覆蓋。可重入函式或每個 worker 獨立暫存區才適合並行；單純加核心不會自動修正。

## 哪種優化處理哪種問題

| 量到的問題 | 對應方向 |
|---|---|
| 計算慢、樣本獨立 | 多板／多核心分派 |
| 一直等串口、文字解析慢 | binary／批次，兩端一起修改 |
| CPU 忙著搬 buffer | 減少複製，適合路徑才用 DMA |
| 接腳時序、位元流處理 | PIO，依周邊規格配置 |
| 算法本身不同版本 | 先修正規格，不用效能技巧掩蓋錯誤 |

ROS 2 管主機程式的訊息通道；PIO 管周邊時序／位元流；模型推論通常仍由 CPU 執行。它們可以出現在同一系統，但沒有必須全部用上的規定。

## 結果有時不見，先查這幾件事

每個 job 是否有唯一 ID、是否收到完整 payload、buffer 是否提早重用、是否有重複／漏回、是否把 debug 文字混進 binary、板子是否重啟／斷線。只有收齊正確結果後，整批時間才有意義。

依據：你提供的歷屆心得是情境來源；API 查 [Pico multicore](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_multicore/include/pico/multicore.h)、[Pico queue](https://github.com/raspberrypi/pico-sdk/blob/master/src/common/pico_util/include/pico/util/queue.h)、[ESP-IDF FreeRTOS](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html)。本頁不主張已知今年題目。
