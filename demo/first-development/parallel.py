"""自編：以多個獨立 C 程序練多板分派，不是硬體／eBNN 效能測試。

每個連線只交給一條主機 thread 使用；可同時等待不同 worker 的回覆。
先跑一個 worker 當正確性基準，再比較多個 worker 的每筆答案。
"""

import argparse
from concurrent.futures import ThreadPoolExecutor
from contextlib import ExitStack
from queue import Empty, Queue
import sys
import time

from host import WorkerClient


def run_batch(values, worker_count):
    jobs = Queue()
    for job_id, value in enumerate(values):
        jobs.put((job_id, value))

    # 一個 WorkerClient 開一個獨立 C 程序，類比一個板子連線。
    # ExitStack 確保發生錯誤時也會關閉已建立的所有 worker。
    with ExitStack() as resources:
        clients = [resources.enter_context(WorkerClient())
                   for _ in range(worker_count)]

        def consume(board_index, client):
            answers = []
            while True:
                try:
                    job_id, value = jobs.get_nowait()
                except Empty:
                    return answers
                # 下一份工作只交給已完成上一份的 worker；不平均硬切兩半。
                answer = client.exchange(value, request_id=job_id)
                answers.append((job_id, answer, board_index))

        started = time.perf_counter()
        with ThreadPoolExecutor(max_workers=worker_count) as pool:
            futures = [pool.submit(consume, i, client)
                       for i, client in enumerate(clients)]
            records = [record for future in futures for record in future.result()]
        elapsed_ms = (time.perf_counter() - started) * 1000

    # 回覆可能亂序。用工作 ID 放回原位置，不把到達順序當作輸入順序。
    answers = [None] * len(values)
    counts = [0] * worker_count
    for job_id, answer, board_index in records:
        if not 0 <= job_id < len(values) or answers[job_id] is not None:
            raise RuntimeError(f"未知或重複工作 ID：{job_id}")
        answers[job_id] = answer
        counts[board_index] += 1
    if any(answer is None for answer in answers):
        raise RuntimeError("還有工作沒有結果")
    return answers, counts, elapsed_ms


def main():
    parser = argparse.ArgumentParser(description="多個 C worker 的分派與回收練習")
    parser.add_argument("--workers", type=int, default=2)
    parser.add_argument("--jobs", type=int, default=100)
    args = parser.parse_args()
    if not 1 <= args.workers <= 8 or not 1 <= args.jobs <= 5000:
        parser.error("workers 需為 1..8，jobs 需為 1..5000")
    values = [i % 1000 for i in range(args.jobs)]
    try:
        baseline, _, serial_ms = run_batch(values, 1)
        answers, counts, parallel_ms = run_batch(values, args.workers)
        if answers != baseline:
            raise RuntimeError("多 worker 的結果與單 worker 基準不同")
        print(f"PASS: {len(values)} 筆結果按工作 ID 還原，與單 worker 一致")
        for i in range(min(4, len(values))):
            print(f"id={i} input={values[i]} result={answers[i]}")
        print(f"各 worker 完成筆數：{counts}")
        print(f"單 worker 批次耗時：{serial_ms:.3f} ms")
        print(f"{args.workers} workers 批次耗時：{parallel_ms:.3f} ms")
        print("這是 Linux 程序與管道的量測；短工作不保證多 worker 更快。")
    except (OSError, ValueError, RuntimeError, TimeoutError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
