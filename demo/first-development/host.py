"""主機端：啟動 worker，以八-byte binary 封包傳工作與收結果。

先執行 make，再用 python3 host.py 21。
只有 Python 標準函式庫；管道／select 的收送寫法以 Linux 為目標。
"""

import argparse
import os
from pathlib import Path
import select
import struct
import subprocess
import sys
import time


class WorkerClient:
    def __init__(self):
        executable = Path(__file__).resolve().parent / "bin" / "worker"
        if not executable.is_file():
            raise FileNotFoundError("找不到 bin/worker，請先在本專案執行 make")
        # 另一個程序有自己的記憶體。用管道連接，並不是直接呼叫 C 函式。
        self.process = subprocess.Popen(
            [str(executable)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            bufsize=0,
        )
        self.request_id = 0

    def exchange(self, value, timeout=3.0, request_id=None):
        if not 0 <= value <= 1000000:
            raise ValueError("輸入範圍是 0..1000000")
        if request_id is None:
            self.request_id = (self.request_id + 1) & 0xFFFFFFFF
        elif isinstance(request_id, int) and 0 <= request_id <= 0xFFFFFFFF:
            self.request_id = request_id  # 多 worker 時由排程器提供全域工作 ID。
        else:
            raise ValueError("工作 ID 必須是 unsigned 32-bit 整數")
        # <：小端序；兩個 I：兩個 unsigned 32-bit 數字，共八 bytes。
        request = struct.pack("<II", self.request_id, value)
        if self.process.stdin.write(request) != len(request):
            raise RuntimeError("工作封包未完整送出")
        self.process.stdin.flush()

        # 不能假設一次 read 就收到八 bytes，累積到完整回覆，最多等三秒。
        deadline = time.monotonic() + timeout
        reply = bytearray()
        while len(reply) < 8:
            remaining = deadline - time.monotonic()
            if remaining <= 0 or not select.select(
                [self.process.stdout], [], [], max(remaining, 0)
            )[0]:
                raise TimeoutError("worker 沒及時回覆；查編譯版本與收送流程")
            chunk = os.read(self.process.stdout.fileno(), 8 - len(reply))
            if not chunk:
                raise RuntimeError("worker 已關閉回覆管道；查看上方錯誤")
            reply.extend(chunk)

        reply_id, answer = struct.unpack("<II", reply)
        if reply_id != self.request_id:
            raise RuntimeError(f"工作 ID 不符：送 {self.request_id}，收 {reply_id}")
        return answer

    def close(self):
        if self.process.stdin and not self.process.stdin.closed:
            self.process.stdin.close()  # 通知 worker 不再有工作。
        try:
            self.process.wait(timeout=1)
        except subprocess.TimeoutExpired:
            self.process.terminate()
            try:
                self.process.wait(timeout=1)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
        if self.process.stdout:
            self.process.stdout.close()

    def __enter__(self):
        return self

    def __exit__(self, *_args):
        self.close()


def main():
    parser = argparse.ArgumentParser(description="送 binary 工作給 C worker")
    parser.add_argument("values", nargs="*", type=int)
    parser.add_argument("--check", action="store_true", help="驗證原規格 2*x+1")
    args = parser.parse_args()
    values = [0, 1, 21, 1000000] if args.check else (args.values or [21])
    try:
        with WorkerClient() as worker:
            for value in values:
                answer = worker.exchange(value)
                print(f"id={worker.request_id} input={value} result={answer}")
                if args.check and answer != value * 2 + 1:
                    raise RuntimeError(f"不符合原規格：預期 {value * 2 + 1}")
        if args.check:
            print("PASS: 原規格 2*x+1，四筆結果與工作 ID 正確")
    except (OSError, ValueError, RuntimeError, TimeoutError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
