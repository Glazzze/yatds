#!/usr/bin/env python3
"""独立暴力 oracle：随机生成 DNA 检索用例，与 ./test 的输出做差分。

oracle 不复用 C++ 实现中的任何函数：
  * 前缀函数 pi 按定义 O(m^2) 直接计算；
  * 6 条翻译序列上的匹配按“逐位置暴力比较”完成，不使用 KMP；
  * 坐标映射与区间统计各自独立实现。

用法（在 baseline3 目录下）：
    python3 tools/oracle.py [轮数] [随机种子]
"""

import os
import random
import subprocess
import sys
import tempfile

CODON = {}
_bases = "TCAG"  # 字典序，便于按标准表录入
# 标准遗传密码表（按 T/C/A/G x T/C/A/G x T/C/A/G 顺序）
_aa_rows = [
    "FFLLSSSSYY**CC*W",
    "LLLLPPPPHHQQRRRR",
    "IIIMTTTTNNKKSSRR",
    "VVVVAAAADDEEGGGG",
]
for _i, _b1 in enumerate(_bases):
    for _j, _b2 in enumerate(_bases):
        for _k, _b3 in enumerate(_bases):
            CODON[_b1 + _b2 + _b3] = _aa_rows[_i][_j * 4 + _k]

COMP = {"A": "T", "T": "A", "C": "G", "G": "C"}


def rev_comp(dna):
    return "".join(COMP[c] for c in reversed(dna))


def translate(dna, frame):
    out = []
    i = frame
    while i + 3 <= len(dna):
        out.append(CODON[dna[i:i + 3]])
        i += 3
    return "".join(out)


def prefix_function(p):
    m = len(p)
    pi = []
    for i in range(m):
        best = 0
        for k in range(1, i + 1):
            if p[:k] == p[i - k + 1:i + 1]:
                best = k
        pi.append(best)
    return pi


def brute_find(text, pattern):
    """逐位置暴力比较，返回全部起点（允许重叠）。"""
    pos = []
    for i in range(len(text) - len(pattern) + 1):
        if text[i:i + len(pattern)] == pattern:
            pos.append(i)
    return pos


def solve(dna, protein, queries):
    n = len(dna)
    m = len(protein)
    matches = []

    if m > 0 and n >= 3 * m:
        for strand, seq in (("+", dna), ("-", rev_comp(dna))):
            for frame in range(3):
                protein_seq = translate(seq, frame)
                for a in brute_find(protein_seq, protein):
                    start = frame + 3 * a
                    if strand == "+":
                        left = start + 1
                        right = start + 3 * m
                    else:
                        left = n - (start + 3 * m) + 1
                        right = n - start
                    matches.append((left, right, strand, frame + 1))

    matches.sort(key=lambda t: (t[0], t[2], t[3]))

    lines = [" ".join(map(str, prefix_function(protein))), str(len(matches))]
    for left, right, strand, frame in matches:
        lines.append(f"{left} {right} {strand} {frame}")
    for l, r in queries:
        count = sum(1 for left, right, _s, _f in matches if left >= l and right <= r)
        lines.append(str(count))
    return "\n".join(lines) + "\n"


def random_case(rng):
    n = rng.randint(3, 30)
    dna = "".join(rng.choice("ACGT") for _ in range(n))

    # 一半用例从某条翻译序列里截取子串作 P，保证有匹配；另一半随机氨基酸
    if rng.random() < 0.6:
        seq = translate(dna, rng.randrange(3))
        if len(seq) >= 1:
            a = rng.randrange(len(seq))
            b = rng.randint(a + 1, len(seq))
            protein = seq[a:b]
        else:
            protein = rng.choice("ACDEFGHIKLMNPQRSTVWY")
    else:
        protein = "".join(rng.choice("ACDEFGHIKLMNPQRSTVWY")
                          for _ in range(rng.randint(1, 4)))

    q = rng.randint(1, 8)
    queries = []
    for _ in range(q):
        l = rng.randint(1, n)
        r = rng.randint(l, n)
        queries.append((l, r))
    return dna, protein, queries


def main():
    rounds = int(sys.argv[1]) if len(sys.argv) > 1 else 200
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 20261110
    rng = random.Random(seed)
    here = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    exe = os.path.join(here, "test")
    if not os.path.exists(exe):
        sys.exit("未找到 ./test，请先执行 make")

    failed = 0
    for t in range(rounds):
        dna, protein, queries = random_case(rng)
        expected = solve(dna, protein, queries)
        with tempfile.NamedTemporaryFile("w", suffix=".txt", delete=False) as fh:
            path = fh.name
            fh.write("\n".join([dna, protein, str(len(queries))]
                               + [f"{l} {r}" for l, r in queries]) + "\n")
        try:
            actual = subprocess.run([exe, path], capture_output=True,
                                    text=True, check=True).stdout
        finally:
            os.unlink(path)
        if actual != expected:
            failed += 1
            print(f"[FAIL] 第 {t + 1} 轮")
            print("DNA     :", dna)
            print("蛋白    :", protein)
            print("查询    :", queries)
            print("--- oracle ---")
            print(expected, end="")
            print("--- 实际 ---")
            print(actual, end="")
            if failed >= 3:
                break

    if failed == 0:
        print(f"随机差分验证通过：{rounds} 轮，全部一致（种子 {seed}）")
    else:
        sys.exit(1)


if __name__ == "__main__":
    main()