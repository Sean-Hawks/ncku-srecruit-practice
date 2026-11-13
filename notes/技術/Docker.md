# Docker：環境、檔案與正在跑的程式

[← 回考場筆記入口](../README.md) · [命令集中查](../操作清單.md)

**Docker 讓程序在指定環境裡執行。檔案同步、重新編譯、重新啟動，是分開的事。**

## 四個名詞

| 名詞 | 意思 |
|---|---|
| image | 打包好的環境範本 |
| container | 用 image 建立的執行環境，裡面可以跑多個程序 |
| COPY | 建 image 時複製檔案，留下當時的快照 |
| bind mount | 將既有主機路徑接到容器路徑，兩邊看同一份檔案 |

Linux 容器通常共用主機 kernel，但套件、程序視野與路徑可不同。它不是板子的韌體，也不是另外啟動一台完整電腦。

## 指令差在哪：在主機使用

| 指令 | 做什麼 |
|---|---|
| `docker ps -a` | 列容器、名稱與狀態 |
| `docker build` | 依 Dockerfile 建 image |
| `docker run` | 新建並啟動一個 container |
| `docker start 容器名` | 啟動已存在、停止中的 container |
| `docker exec -it 容器名 bash` | 在已執行的容器裡開 shell |
| `docker stop 容器名` | 停止容器，裡面的程序也會中斷 |
| `exit` | 結束目前 shell；容器是否停止取決於主程序 |

**在主機 cd 到哪裡，不會改變既有容器的掛載。** exec 只是進入原來的環境，不會重新複製你的目前資料夾。

## 改了沒生效：照這三層查

| 層 | 要確認 | 判斷 |
|---|---|---|
| 檔案 | 編輯器已 `:w`；容器讀到的是哪份檔案 | 兩邊內容若不同，先查 mount／路徑 |
| 建置 | C/C++ 是否產生新執行檔 | 存檔不等於編譯 |
| 程序 | 是否仍有舊程序在跑 | Python／一般執行檔通常要重啟才載入新版本 |

主機查 mount，將容器名換成實際值：

```bash
docker inspect 容器名 --format '{{range .Mounts}}{{.Source}} -> {{.Destination}}{{println}}{{end}}'
```

Source 是主機來源，Destination 是容器路徑。掛到某個目錄後，image 中原本那個位置的內容會被暫時遮住。讀寫掛載下，在容器刪檔也會刪主機檔案。

## 家用設定：不是考場固定路徑

```text
主機 /home/hawks/Development/ncku-srecruit-practice/demo
                           ↕ bind mount
容器 srecruit-demo 裡的 /demo
```

進入既有家用容器：`docker exec -it srecruit-demo bash`。你之前的問題是容器掛舊 repo、nvim 改新 repo，詳見 [環境紀錄](../練習環境與路線.md)。

<details>
<summary>只有需要新建容器時：掛到指定資料夾的命令</summary>

以下前提是主機已有 srecruit-demo image、來源資料夾存在。建立名為 my-practice 的新容器：

```bash
docker run --rm -it \
  --name my-practice \
  --mount type=bind,src=/home/hawks/Development/ncku-srecruit-practice/demo,dst=/demo \
  srecruit-demo bash
```

src 改成想接入的主機資料夾；dst 是容器內路徑。另一個終端進入同一個新容器用 `docker exec -it my-practice bash`。

主 shell 結束後，`--rm` 會移除這個容器；掛載的主機檔案仍保留。更換既有容器的 mount 通常需要另建容器，先保留原容器內需要的資料。

</details>

USB 權限／裝置映射與網路是另外的配置。容器看不到板子、ROS 跨容器找不到 node 時，先查題目提供的部署環境。

來源：[Docker bind mounts](https://docs.docker.com/engine/storage/bind-mounts/)、[container 概念](https://docs.docker.com/get-started/docker-concepts/the-basics/what-is-a-container/)。
