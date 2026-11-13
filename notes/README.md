# 考場入口

**先讀 [猜題與解題思路](猜題與解題思路.md)。** 每題都有簡介、線索、已解讀例子和作答順序；不用另外執行練習。

底層名詞不熟，先查 [架構與 C 記憶體](底層架構與資料流.md)。忘了公式看 [一頁速查](考場速查與離線模型提問.md)，忘了命令看 [操作清單](操作清單.md)。

## 按題目找資料

| 看到什麼 | 查哪頁 |
|---|---|
| opcode、register、PC、指令解碼、位址 | [ISA 編碼與定址](技術/ISA與指令編碼.md) |
| .pio、out、in、push、FIFO、移位 | [PIO](技術/PIO.md) |
| mask、取某幾個 bits、封裝整數 | [位元運算](技術/位元運算.md) |
| node、topic、callback、訊息沒到 | [ROS 2](技術/ROS2.md) |
| thread、雙核心、鎖、queue、多板、封包 | [平行運算與工作交接](技術/平行運算.md) |
| DMA、count、increment、搬運、CRC／校驗 | [資料搬運與校驗](技術/資料搬運.md) |
| RP2350、SDK、UF2、韌體怎麼更新 | [RP2350 與 Pico SDK](技術/RP2350與PicoSDK.md) |
| 正確但慢、越界、崩潰 | [效能與除錯](技術/效能量測與除錯.md) |

Docker／Make／Git／nvim 都合併到 [操作清單](操作清單.md)。ESP32／eBNN 合併成一頁 [歷屆備查](技術/歷屆延伸.md)，題目有出現才看。

## 官方線索與猜題的界線

| 來源 | 實際線索 |
|---|---|
| [正式公告](https://www.csie.ncku.edu.tw/zh-hant/news/13617) 的附件「考試要點」 | 讀懂系統軟硬體文件、優化程式與整體效能 |
| [官方演練 PDF](../docs/116學年度國立成功大學資訊工程學系特殊選才演練試題.pdf) 印刷頁 2–5 | ROS 2 CLI、資料流與診斷 |
| 同 PDF 印刷頁 5–6 | 位元、PIO；特別提示 ISA 編碼與定址、PIO 周邊章節 |
| 同 PDF 印刷頁 6–8 | 臨界區、搬運引擎、筆數／寬度／遞增、搬運與校驗周邊 |
| [演練 section2-2 說明](../demo/section2-2/README.md) | 連到 RISC-V ISA 與 RP2350 PIO 文件 |

演練不是正式考題，沒有透露正式演算法。**以下猜題按線索安排閱讀優先序，不代表確定題目、板型或出題機率。**

[考場規則與時間](成大上機考規則與避雷.md) · [GitHub 本頁](https://github.com/Sean-Hawks/ncku-srecruit-practice/blob/main/notes/README.md)
