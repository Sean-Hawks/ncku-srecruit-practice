"""資料來源：每秒往 /number 發送一個 1～100 的隨機整數。
它不需要 processor 才能發送；echo /number 也能直接接收。
改檔後存檔、停止舊程序、重新執行，才會使用新內容。
"""

import random

import rclpy
from rclpy.node import Node
from std_msgs.msg import Int32


class Publisher(Node):
    def __init__(self):
        # /publisher 是 node 名稱：用 ros2 node list 查。
        super().__init__("publisher")
        # /number 是發送通道：用 ros2 topic list 查。
        # 接收者要使用相同 topic、相符的訊息型別與相容 QoS。
        # 可改：若這裡改成 /numbers，processor 的 INPUT_TOPIC 也要配合。
        # 10 是訊息歷史深度，與「每秒幾次」無關。
        self.pub = self.create_publisher(Int32, "/number", 10)
        # 可改：1.0 是間隔秒數。每秒呼叫 tick 一次；改 0.5 約每秒兩次。
        # self.tick 是要執行的函式，這裡先登記，不立即呼叫。
        self.timer = self.create_timer(1.0, self.tick)

    def tick(self):
        # Int32 是 ROS 訊息型別；msg.data 是它攜帶的整數。
        msg = Int32()
        # 可改：為了固定測試，可換成 msg.data = 21。
        # 保留這份原碼則每次選 1～100；用 CLI 測試時先停止它，避免數字混在一起。
        msg.data = random.randint(1, 100)
        # 把這筆訊息送到 /number。echo 與 processor 都能各自訂閱，
        # 不會因為 processor 先收到，echo 就收不到。
        self.pub.publish(msg)


def main():
    rclpy.init()
    node = Publisher()
    print("publisher 已啟動，按 Ctrl-C 停止", flush=True)
    try:
        # 讓 ROS executor 處理 timer 事件，tick 才會持續執行。
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.try_shutdown()


if __name__ == "__main__":
    main()
