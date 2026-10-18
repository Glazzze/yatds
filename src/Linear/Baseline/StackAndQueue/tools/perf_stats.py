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


def collect(bdir, program, runs):
    samples = {}  # (label, n) -> [times]
    for _ in range(runs):
        out = subprocess.run([program], cwd=bdir, capture_output=True, text=True).stdout
        for line in out.splitlines():
            m = PERF_RE.match(line)
            if m:
                label, n, _, t = m.groups()
                samples.setdefault((label, n), []).append(float(t))
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
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    ref = None
    if "--ref" in sys.argv:
        ref = sys.argv[sys.argv.index("--ref") + 1]
    if len(args) < 2:
        print(__doc__)
        sys.exit(2)
    bdir, program = args[0], args[1]
    runs = int(args[2]) if len(args) > 2 else 30

    samples = collect(bdir, program, runs)
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
            print(f"  {lab:<20} {statistics.mean(s):8.2f} ± {statistics.stdev(s):5.2f}")
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
                sig = "" if p == p and p >= 0.05 else "（显著）"
                print(f"    {a} vs {b}: t={t:8.3f} df={df:6.2f} "
                      f"p={p:.3g} {star(p)}{sig}")


if __name__ == "__main__":
    main()