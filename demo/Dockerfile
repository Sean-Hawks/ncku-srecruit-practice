# 此檔是建立 image 的規則，不是日常執行每支程式的指令。
# image：裝好 ROS/Python/編譯器的模板；container：依模板啟動的執行環境。
# 在專案根目錄建置：docker build -t srecruit-demo demo/
#
# 日常用法、掛載與三個終端機的命令可直接查 notes/操作清單.md，不必背。
# COPY 是建置 image 當時的快照；改主機檔案不會自動改這份 image。
# bind mount 則把主機資料夾接到容器 /demo，兩邊直接讀寫同一份檔案。
# 目前練習掛載的是 ~/Development/ncku-srecruit-practice/demo。
# 在哪個主機資料夾執行 docker exec，不會改變已建立的掛載。
# 改掛載的 Python 檔：存檔、重啟該 Python 程式即可，不必每次重建 image。
# 修改此 Dockerfile/依賴環境時，再考慮重新 docker build。
#
# 可改：需要其他 base image 或鏡像來源時，從 build arg 指定。
ARG DEMO_BASE_IMAGE=ros:jazzy-ros-core
FROM ${DEMO_BASE_IMAGE}

# 建置 image 時安裝 C 編譯器、標準函式庫開發檔和 make。
RUN apt-get update \
    && apt-get install -y --no-install-recommends gcc libc6-dev make \
    && rm -rf /var/lib/apt/lists/*

# . 指建置上下文 demo/；把當時的檔案複製到 image 的 /demo。
# 啟動 container 若再掛載 /demo，就會看到掛載來源的內容。
COPY . /demo

# 額外 docker exec 開的 shell 也需要 ROS 環境。
# 這裡設定 root 的互動式 bash；使用其他帳號需在該帳號 shell 做相同設定。
RUN echo 'source /opt/ros/jazzy/setup.bash' >> /root/.bashrc

# 預設工作目錄與啟動程式；啟動後進到 /demo 的 bash。
WORKDIR /demo
CMD ["bash"]
