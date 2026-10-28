"""在独立临时工程中注入失败，验证顶层 check/oracle/asan 不会误报成功。"""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

SOURCE = Path(__file__).resolve().parents[1] / "Makefile"
checks = 0
for target in ("check", "oracle", "asan"):
    for failing in (True, False):
        with tempfile.TemporaryDirectory(prefix="make-status-") as dirname:
            root = Path(dirname)
            shutil.copy2(SOURCE, root / "Makefile")
            for folder in ("src", "baseline1/src", "baseline2/src", "baseline3/src", ".build"):
                (root / folder).mkdir(parents=True, exist_ok=True)
            for name in ("baseline1/src/main_test.cpp", "src/array_stack/stackar.cpp",
                         "src/linked_stack/stackli.cpp", "baseline2/src/main_test.cpp",
                         "baseline2/src/linked_queue.cpp", "src/queue/queue.cpp",
                         "baseline3/src/main_test.cpp", "baseline3/src/stack_by_queue.cpp",
                         "baseline3/src/queue_by_stack.cpp", "baseline3/src/vector_containers.cpp"):
                p = root / name
                p.parent.mkdir(parents=True, exist_ok=True)
                p.touch()
            for i in (1, 2, 3):
                status = 7 if failing and i == 1 else 0
                action = "touch ../later-ran; " if i > 1 else ""
                (root / f"baseline{i}/Makefile").write_text(
                    f"check oracle:\n\t@{action}exit {status}\n")
                binary = root / f".build/b{i}_asan"
                binary.write_text(f"#!/bin/sh\n{action}exit {status}\n")
                binary.chmod(0o755)
            # 隔离父 make 的 -B/-n 等标志，避免夹具脚本被强制当作 C++ 重新构建。
            result = subprocess.run(["make", target], cwd=root, capture_output=True, text=True,
                                    env={**os.environ, "MAKEFLAGS": "", "MFLAGS": ""})
            assert (result.returncode != 0) == failing, result.stdout + result.stderr
            assert (root / "later-ran").exists() == (not failing), result.stdout + result.stderr
            checks += 1
print(f"Makefile failure propagation: {checks} PASS")
