#!/usr/bin/env python3
"""独立 oracle（队列）：按课程 README 的输入/输出格式，用 Python 独立实现队列语义，
与 outputs/ 中"由被测程序自身生成"的标准输出逐用例比对。

与 C++ 实现零共享代码，作为 outputs/ 之外的第二正确性来源，打破自洽循环。

用法：python3 tools/oracle.py        （须在 baseline2/ 目录下运行）
"""
import glob
import os
import sys

MAXCAP = 10_000_000


def clamp(cap):
    if cap < 0:
        return 0
    return MAXCAP if cap > MAXCAP else cap


def run_case(text):
    lines = text.splitlines()
    idx = 0
    cap, n = map(int, lines[idx].split())
    idx += 1
    cap = clamp(cap)
    init = list(map(int, lines[idx].split())) if n else []
    idx += 1

    q = []  # 索引 0 为队首，末尾为队尾
    out = []

    def push(x):
        if len(q) >= cap:
            out.append("ERROR")
        else:
            q.append(x)

    for x in init:
        push(x)

    m = int(lines[idx])
    idx += 1
    for _ in range(m):
        p = lines[idx].split()
        idx += 1
        op = p[0]
        if op == "PUSH":
            push(int(p[1]))
        elif op == "POP":
            out.append("ERROR" if not q else str(q.pop(0)))
        elif op == "FRONT":
            out.append("ERROR" if not q else str(q[0]))
        elif op == "SIZE":
            out.append(str(len(q)))
        elif op == "EMPTY":
            out.append("true" if not q else "false")
        elif op == "PRINT":
            out.append("EMPTY" if not q else " ".join(map(str, q)))
    return ("\n".join(out) + "\n") if out else ""


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