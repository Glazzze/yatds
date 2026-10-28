"""验证统计收集器拒绝失败/缺失/非法数据，并保存完整原始样本。"""
from pathlib import Path
import csv
import subprocess
import sys
import tempfile
sys.dont_write_bytecode = True
from perf_stats import collect

checks = 0
with tempfile.TemporaryDirectory(prefix="perf-collector-") as dirname:
    root = Path(dirname)
    program = root / "fake.py"
    def install(body):
        program.write_text(f"#!{sys.executable}\n" + body)
        program.chmod(0o755)
    install('print("PERF fake n=10 ops=20 time_ms=0.123456")\n')
    samples = collect(root, "./fake.py", 2, root / "raw.csv")
    assert samples[("fake", "10")] == [0.123456, 0.123456]
    assert len(list(csv.DictReader((root / "raw.csv").open()))) == 2
    checks += 1
    for body in ('raise SystemExit(7)\n', 'print("no data")\n',
                 'print("PERF fake n=10 ops=20 time_ms=nan")\n',
                 'print("PERF fake n=10 ops=20 time_ms=-1")\n',
                 'print("PERF fake n=10 ops=20 time_ms=0.1\\nPERF fake n=10 ops=20 time_ms=0.1")\n',
                 'from pathlib import Path\np=Path("counter"); n=int(p.read_text())+1 if p.exists() else 1; p.write_text(str(n)); print(f"PERF fake n={n} ops=20 time_ms=0.1")\n'):
        install(body)
        try:
            collect(root, "./fake.py", 2)
        except (ValueError, subprocess.CalledProcessError):
            checks += 1
        else:
            raise AssertionError("错误数据未被拒绝")
print(f"Performance collector regression: {checks} PASS")
