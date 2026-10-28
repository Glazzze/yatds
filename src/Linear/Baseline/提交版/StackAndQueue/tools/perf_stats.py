#!/usr/bin/env python3
"""性能统计与显著性检验：重复运行基准程序，解析其 `PERF name n=... ops=... time_ms=...`
行，报告【均值 ± 总体标准差】并做 **Welch t 检验**（不等方差两样本），
用于判断自实现与参照实现的耗时差异是否显著、而非仅比较均值。

另附 peak-RSS 空间探针（space_probe）用于量化"链表每元素额外开销"。

用法：
  python3 tools/perf_stats.py <baseline_dir> <program> [runs] [--ref LABEL]

示例：
  python3 tools/perf_stats.py baseline1 ./test 30 --ref std_stack
  python3 tools/perf_stats.py baseline2 ./queue_methods 30 --ref circular_queue
  python3 tools/perf_stats.py baseline3 ./test 30          # 全部两两比较

无 --ref 时对所有实现做两两 Welch 检验。
"""
import argparse
import csv
from pathlib import Path
import math
import re
import statistics
import subprocess
import sys


def _betacf(a, b, x):
    """连分式（Lentz 法）求不完全贝塔函数。"""
    tiny = 1e-30
    qab, qap, qam = a + b, a + 1.0, a - 1.0
    c = 1.0
    d = 1.0 - qab * x / qap
    if abs(d) < tiny:
        d = tiny
    d = 1.0 / d
    h = d
    for m in range(1, 200):
        m2 = 2 * m
        aa = m * (b - m) * x / ((qam + m2) * (a + m2))
        d = 1.0 + aa * d
        if abs(d) < tiny:
            d = tiny
        c = 1.0 + aa / c
        if abs(c) < tiny:
            c = tiny
        d = 1.0 / d
        h *= d * c
        aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2))
        d = 1.0 + aa * d
        if abs(d) < tiny:
            d = tiny
        c = 1.0 + aa / c
        if abs(c) < tiny:
            c = tiny
        d = 1.0 / d
        de = d * c
        h *= de
        if abs(de - 1.0) < 1e-12:
            break
    return h


def betai(a, b, x):
    """正则化不完全贝塔函数 I_x(a, b)。"""
    if x <= 0.0:
        return 0.0
    if x >= 1.0:
        return 1.0
    lbeta = (math.lgamma(a + b) - math.lgamma(a) - math.lgamma(b)
             + a * math.log(x) + b * math.log(1.0 - x))
    if x < (a + 1.0) / (a + b + 2.0):
        return math.exp(lbeta) * _betacf(a, b, x) / a
    return 1.0 - math.exp(lbeta) * _betacf(b, a, 1.0 - x) / b


def welch_ttest(s1, s2):
    """返回 (t, df, p_two_tailed)。"""
    n1, n2 = len(s1), len(s2)
    m1, m2 = statistics.mean(s1), statistics.mean(s2)
    v1 = statistics.variance(s1)
    v2 = statistics.variance(s2)
    se = math.sqrt(v1 / n1 + v2 / n2)
    if se == 0.0:
        return float("nan"), float("nan"), float("nan")
    t = (m1 - m2) / se
    df = (v1 / n1 + v2 / n2) ** 2 / (
        (v1 / n1) ** 2 / (n1 - 1) + (v2 / n2) ** 2 / (n2 - 1))
    p = betai(df / 2.0, 0.5, df / (df + t * t))  # 双尾
    return t, df, p


PERF_RE = re.compile(r"^PERF\s+(\S+)\s+n=(\S+)\s+ops=(\S+)\s+time_ms=(\S+)")


def collect(bdir, program, runs, csv_path=None):
    if runs < 2:
        raise ValueError("统计至少需要两次独立运行")
    samples = {}
    rows = []
    schema = None
    for run in range(1, runs + 1):
        result = subprocess.run([program], cwd=bdir, capture_output=True, text=True, check=True)
        current = set()
        for line in result.stdout.splitlines():
            if not line.startswith("PERF "):
                continue
            match = PERF_RE.fullmatch(line)
            if match is None:
                raise ValueError(f"无效 PERF 行: {line}")
            label, n, ops, value = match.groups()
            scale, operations, elapsed = int(n), int(ops), float(value)
            if scale < 0 or operations < 0 or not math.isfinite(elapsed) or elapsed < 0:
                raise ValueError(f"无效性能数据: {line}")
            key = (label, n)
            if key in current:
                raise ValueError(f"同一次运行出现重复记录: {key}")
            current.add(key)
            samples.setdefault(key, []).append(elapsed)
            rows.append((run, label, scale, operations, elapsed))
        if not current or (schema is not None and current != schema):
            raise ValueError("性能记录缺失，或多次运行的标签/规模不一致")
        schema = current
    if csv_path:
        path = Path(csv_path)
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("w", newline="", encoding="utf-8") as output:
            writer = csv.writer(output)
            writer.writerow(("run", "label", "n", "ops", "time_ms"))
            writer.writerows(rows)
    return samples


def star(p):
    if p != p:  # NaN
        return "  n/a"
    if p < 0.001:
        return "***"
    if p < 0.01:
        return " **"
    if p < 0.05:
        return "  *"
    return " ns"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bdir")
    parser.add_argument("program")
    parser.add_argument("runs", nargs="?", type=int, default=30)
    parser.add_argument("--ref")
    parser.add_argument("--csv", help="保存每次独立运行的原始 PERF 数据")
    args = parser.parse_args()
    bdir, program, runs, ref = args.bdir, args.program, args.runs, args.ref
    samples = collect(bdir, program, runs, args.csv)
    if not samples:
        print("未解析到任何 PERF 行")
        sys.exit(1)

    ns = sorted({n for (_, n) in samples}, key=lambda s: int(s))
    print(f"# {bdir}/{program}  独立运行 {runs} 次，每项报告 均值 ± 总体标准差（ms）")
    for n in ns:
        labels = [lab for (lab, nn) in samples if nn == n]
        # 保持出现顺序
        seen, ordered = set(), []
        for lab in labels:
            if lab not in seen:
                seen.add(lab)
                ordered.append(lab)
        print(f"\n## n={n}")
        for lab in ordered:
            s = samples[(lab, n)]
            print(f"  {lab:<20} {statistics.mean(s):10.6f} ± {statistics.pstdev(s):9.6f}")
        pairs = []
        if ref is not None:
            if (ref, n) not in samples:
                print(f"  （参照 {ref} 在 n={n} 下无数据）")
            else:
                for lab in ordered:
                    if lab != ref:
                        pairs.append((lab, ref))
        else:
            for i in range(len(ordered)):
                for j in range(i + 1, len(ordered)):
                    pairs.append((ordered[i], ordered[j]))
        if pairs:
            print("  Welch t 检验（H0：两组均值相等）：")
            for a, b in pairs:
                t, df, p = welch_ttest(samples[(a, n)], samples[(b, n)])
                sig = "（显著）" if math.isfinite(p) and p < 0.05 else ""
                print(f"    {a} vs {b}: t={t:8.3f} df={df:6.2f} "
                      f"p={p:.3g} {star(p)}{sig}")


if __name__ == "__main__":
    main()