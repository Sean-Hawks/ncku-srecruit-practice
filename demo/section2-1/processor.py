"""先看 on_msg：收到一個整數，計算後另發一筆訊息。
目前保留你的練習公式：3*x+1（21 → 64）；原版公式是 2*x（21 → 42）。
改完要 :w 存檔，再停止並重新執行 python3 processor.py。
"""

import rclpy
from rclpy.node import Node
from std_msgs.msg import Int32

# 可改：改輸入 topic 時，發送者也必須使用相同名稱。
# /number 是通訊名稱，不是變數或函式。/number 與 /numbers 是不同通道。
INPUT_TOPIC = "/number"
# /double 只是輸出名稱，不會自動把數字乘二；計算由下方 on_msg 決定。
OUTPUT_TOPIC = "/double"


class Processor(Node):
    def __init__(self):
        # 建立名為 /processor 的 node；ros2 node list 查的是這個名稱。
        super().__init__("processor")
        # 登記接收 /number 的 Int32 訊息。收到後，由 executor 呼叫 self.on_msg。
        # 10 是此寫法的訊息歷史深度，不是乘數、頻率或 topic 編號。
        # ros2 node info /processor 會在 Subscribers 顯示這條接線。
        self.sub = self.create_subscription(Int32, INPUT_TOPIC, self.on_msg, 10)
        # 在這裡就把「往 /double 送」設定進 self.pub。
        # 所以下方 publish(out) 不必再寫一次 /double。
        # node info 的 Publishers 可看到這條接線；topic list 查的是通道名稱。
        self.pub = self.create_publisher(Int32, OUTPUT_TOPIC, 10)

    # received 是這次收到的訊息物件；received.data 才是裡面的整數。
    # 沒收到訊息就不會進入此函式，所以啟動後安靜等待是正常的。
    def on_msg(self, received):
        # 這行只印原始輸入；看到 received: 21，還不能知道最後算出多少。
        print(f"received: {received.data}", flush=True)

        # 建立另一個訊息物件來放輸出，先不發送。
        out = Int32()
        # 可改：真正的計算在這一行。例如 *2 得 42；*3+1 得 64；-5 得 16。
        # = 是把右邊結果存進左邊；若寫成 out.data - received.data*2，
        # 只會算減法，不會存回結果。Int32 的輸入與輸出都要在型別範圍內。
        out.data = received.data * 3 + 1
        # 現在才送到 __init__ 設定的 /double。
        # ros2 topic echo /double 是另一個接收者，只印資料，不負責乘二。
        # echo /number 則可直接收到原始輸入，不需要經過 processor。
        self.pub.publish(out)


def main():
    # 初始化 ROS 通訊，接著建立 node 與上面的訂閱、發布設定。
    rclpy.init()
    node = Processor()
    try:
        # 等待並處理事件，例如收到訊息後執行 on_msg；這不是重新讀取程式檔。
        # 測試：另開 echo /double，再用 CLI 往 /number 發送 21。
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        # 離開時清理 node 與 ROS 資源。
        node.destroy_node()
        rclpy.try_shutdown()


if __name__ == "__main__":
    main()
