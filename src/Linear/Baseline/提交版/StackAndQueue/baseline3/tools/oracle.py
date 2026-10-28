#!/usr/bin/env python3
"""独立 oracle（综合）：按课程 README 的 T 场景格式，用 Python 独立实现
"队列实现栈 / 栈实现队列 / 栈队列互转"的语义，与 outputs/ 中由被测程序
自身生成的标准输出逐用例比对。

与 C++ 实现零共享代码，作为 outputs/ 之外的第二正确性来源。

两种 CONVERT 方向最终都输出"反转后的原始序列"：
  - STACK_TO_QUEUE：栈顶→队首，故队列 front→back = 原序列逆序；
  - QUEUE_TO_STACK：队首→栈底，故栈 top→bottom = 原序列逆序。

用法：python3 tools/oracle.py        （须在 baseline3/ 目录下运行）
"""
import glob
import os
import sys


def run_case(text):
    it = iter(text.split())
    nxt = lambda: next(it)
    T = int(nxt())
    out = []
    for t in range(1, T + 1):
        typ = nxt()
        out.append(f"CASE {t}")
        if typ == "STACK_BY_QUEUE":
            n = int(nxt())
            stack = [int(nxt()) for _ in range(n)]  # bottom→top
            m = int(nxt())
            for _ in range(m):
                op = nxt()
                if op == "PUSH":
                    stack.append(int(nxt()))
                elif op == "POP":
                    out.append("ERROR" if not stack else str(stack.pop()))
                elif op == "TOP":
                    out.append("ERROR" if not stack else str(stack[-1]))
                elif op == "PRINT":
                    out.append("EMPTY" if not stack else " ".join(map(str, reversed(stack))))
        elif typ == "QUEUE_BY_STACK":
            n = int(nxt())
            q = [int(nxt()) for _ in range(n)]  # front→back
            m = int(nxt())
            for _ in range(m):
                op = nxt()
                if op == "PUSH":
                    q.append(int(nxt()))
                elif op == "POP":
                    out.append("ERROR" if not q else str(q.pop(0)))
                elif op == "FRONT":
                    out.append("ERROR" if not q else str(q[0]))
                elif op == "PRINT":
                    out.append("EMPTY" if not q else " ".join(map(str, q)))
        elif typ == "CONVERT":
            n = int(nxt())
            seq = [int(nxt()) for _ in range(n)]
            nxt()  # STACK_TO_QUEUE / QUEUE_TO_STACK，两个方向结果同形
            out.append("EMPTY" if not seq else " ".join(map(str, reversed(seq))))
    return "\n".join(out) + "\n"


def main():
    bad = total = 0
    for path in sorted(glob.glob("inputs/*.txt")):
        base = os.path.basename(path)
        total += 1
        try:
            expected = open(os.path.join("outputs", base)).read()
        except FileNotFoundError:
            print(f"{base}: 缺少 outputs 文件")
            bad += 1
            continue
        try:
            actual = run_case(open(path).read())
        except Exception as e:
            print(f"{base}: 样例不符合 README 的输入格式（{type(e).__name__}: {e}）")
            bad += 1
            continue
        if actual == expected:
            print(f"{base}: OK（独立 oracle 与 outputs 一致）")
        else:
            bad += 1
            print(f"{base}: MISMATCH\n--- oracle ---\n{actual}--- outputs ---\n{expected}")
    print(f"独立 oracle 校验：{total - bad}/{total} 通过")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()