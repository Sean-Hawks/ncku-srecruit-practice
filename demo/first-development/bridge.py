"""ROS 2 橋接：topic → binary → C worker → binary → topic。

這個檔案在既有 ROS 2 容器執行，不需要開發板或 PlatformIO。
第一輪先讀 on_msg；host.py 處理封包，worker.c 處理計算。
"""

import rclpy
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node
from std_msgs.msg import Int32

from host import WorkerClient


class DevelopmentBridge(Node):
    def __init__(self):
        super().__init__("development_bridge")
        self.worker = WorkerClient()
        self.pub = self.create_publisher(Int32, "/practice/out", 10)
        self.sub = self.create_subscription(Int32, "/practice/in", self.on_msg, 10)
        self.get_logger().info("ready: /practice/in → C worker → /practice/out")

    def on_msg(self, received):
        try:
            # 計算發生在另一個 C 程序，不是把公式寫在 callback 裡。
            answer = self.worker.exchange(received.data)
            out = Int32()
            out.data = answer
            self.pub.publish(out)
            self.get_logger().info(f"input={received.data} result={answer}")
        except (OSError, ValueError, RuntimeError, TimeoutError) as error:
            self.get_logger().error(str(error))

    def destroy_node(self):
        self.worker.close()
        return super().destroy_node()


def main():
    rclpy.init()
    node = None
    try:
        node = DevelopmentBridge()
        rclpy.spin(node)
    except (KeyboardInterrupt, ExternalShutdownException):
        pass
    finally:
        if node is not None:
            node.destroy_node()
        rclpy.try_shutdown()


if __name__ == "__main__":
    main()
