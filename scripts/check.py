"""Local and hosted CI share this offline acceptance sequence."""
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def run(*command, env=None):
    print("+", " ".join(map(str, command)), flush=True)
    subprocess.run(list(map(str, command)), cwd=ROOT, check=True, env=env)


if __name__ == "__main__":
    run(sys.executable, "scripts/quality.py", "format-check")
    run("cmake", "--preset", "debug")
    run("cmake", "--build", "--preset", "debug")
    run("ctest", "--preset", "debug")
    run("cmake", "--build", "build/debug", "--target", "all_qmllint")
    run(sys.executable, "scripts/quality.py", "tidy")
    run("cmake", "--preset", "release")
    run("cmake", "--build", "--preset", "release")
    run("cmake", "--install", "build/release")
    environment = os.environ.copy()
    environment.update(QT_QPA_PLATFORM="offscreen", QT_QUICK_BACKEND="software",
                       QT_QUICK_CONTROLS_STYLE="Fusion", QT_QPA_PLATFORMTHEME="")
    # Staging checks the application payload; this is not a portable binary bundle.
    provider_lib = (ROOT / "build/release/qmarkdown-library-dir.txt").read_text().strip()
    environment["LD_LIBRARY_PATH"] = os.pathsep.join(filter(None, (provider_lib, environment.get("LD_LIBRARY_PATH", ""))))
    run(ROOT / "build/install/bin/md-viewer", "--smoke-test", env=environment)
