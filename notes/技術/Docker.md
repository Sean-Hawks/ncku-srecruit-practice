# Docker：環境與檔案在哪裡

**先記：image 是環境範本；container 是用它開出的執行環境；bind mount 是把主機資料夾接進去。**

## 目前練習環境

```text
主機 /home/hawks/Development/ncku-srecruit-practice/demo
                         ↕ 同一份檔案，讀寫掛載
容器 srecruit-demo 裡的 /demo
```

主機與容器可以有不同程式、套件和路徑。容器通常共用主機 Linux kernel；它不是另外開一台完整電腦。

## 最常用：在主機貼

```bash
docker ps -a                         # 查 container 與狀態
docker start srecruit-demo            # 已停止才需要 start
docker exec -it srecruit-demo bash    # 在既有 container 開一個 shell
```

進去後：

```bash
cd /demo/section2-1
python3 processor.py
```

另一個主機終端機也先 `docker exec -it srecruit-demo bash`，才跑 ROS 指令。

| 操作 | 做什麼 |
|---|---|
| `docker build` | 依 Dockerfile 建 image |
| `docker run` | 建立並啟動新的 container |
| `docker start` | 啟動已存在、停止中的 container |
| `docker exec` | 在正在執行的 container 裡啟動另一個程式 |
| `exit` | 結束目前 shell；是否讓整個 container 停止，要看主程序 |

## 外面改了，裡面沒變

1. nvim `:w` 存檔。
2. 查兩邊是不是同一份檔案。
3. Python：Ctrl-C 後重跑。C：重新編譯再跑。

在**主機**：

```bash
docker inspect srecruit-demo --format '{{range .Mounts}}{{.Source}} -> {{.Destination}}{{println}}{{end}}'
rg 'out.data =' ~/Development/ncku-srecruit-practice/demo/section2-1/processor.py
docker exec srecruit-demo grep 'out.data =' /demo/section2-1/processor.py
```

你之前真的發生過：nvim 改新 repo，container 卻掛舊的 ncku-practice/demo。目前已改成上面的新路徑。**在哪個主機資料夾執行 exec，不會改掛載。**

`COPY . /demo` 是建 image 時的快照；bind mount 才是直接接到主機資料夾。mount 到 /demo 後，該位置原本的 image 內容會被掛載內容遮住。

## 要掛到自己選的資料夾

<details>
<summary>新建練習容器的長指令：只改 src 與名字</summary>

前提：主機上已有 srecruit-demo image，src 資料夾存在。**這會建立另一個容器**：

```bash
docker run --rm -it \
  --name my-practice \
  --mount type=bind,src=/home/hawks/Development/ncku-srecruit-practice/demo,dst=/demo \
  srecruit-demo bash
```

src 是主機資料夾，dst 是容器路徑。第二個終端機改用：

```bash
docker exec -it my-practice bash
```

原始 bash 結束時，這個 `--rm` 容器會刪除；掛載的主機檔案保留。既有 container 要換 mount，需要另建容器時指定。

若 image 不存在，在**主機的 demo 目錄**建置：

```bash
cd ~/Development/ncku-srecruit-practice/demo
docker build -t srecruit-demo .
```

</details>

## USB、網路與停止

- 容器看不到主機 USB，不代表板子壞了；還可能缺裝置映射與權限。優先用題目提供的燒錄環境。
- 目前 ROS 練習都在同一個 container。跨容器／跨主機還涉及網路與 ROS discovery，別先猜演算法錯。
- `docker stop srecruit-demo` 會中斷裡面正在執行的程式。
- 掛載是讀寫的：在容器刪 /demo 裡的檔案，也會刪主機檔案。

**驗證自己懂了：**主機改公式後，容器 grep 立即看見新公式；重啟 processor 後，21 才得到新結果。

來源：[Docker bind mounts](https://docs.docker.com/engine/storage/bind-mounts/)、[container 概念](https://docs.docker.com/get-started/docker-concepts/the-basics/what-is-a-container/)。

[回技術目錄](README.md)
