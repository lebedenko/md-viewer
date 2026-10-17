"""Local and hosted CI share this offline acceptance sequence."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

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
    run("desktop-file-validate", "assets/mdv.desktop")
    environment = os.environ.copy()
    environment.update(QT_QPA_PLATFORM="offscreen", QT_QUICK_BACKEND="software",
                       QT_QUICK_CONTROLS_STYLE="Fusion", QT_QPA_PLATFORMTHEME="")
    # Preserve SDK loader settings, but never inject the QMarkdown library path.
    def verify_payload(prefix):
        for relative, source in (
            ("share/applications/mdv.desktop", "assets/mdv.desktop"),
            ("share/icons/hicolor/scalable/apps/mdv.svg", "assets/mdv.svg"),
        ):
            installed = prefix / relative
            if installed.read_bytes() != (ROOT / source).read_bytes():
                raise RuntimeError(f"Incorrect installed payload: {installed}")
        executable = prefix / "bin/mdv"
        if not os.access(executable, os.X_OK):
            raise RuntimeError(f"Not executable: {executable}")
        manifest = (ROOT / "build/release/install_manifest.txt").read_text().splitlines()
        if any(Path(entry).name == "md-viewer" for entry in manifest):
            raise RuntimeError("Unexpected compatibility executable in install manifest")
        run("desktop-file-validate", prefix / "share/applications/mdv.desktop")
        for option in ("--help", "--version", "--smoke-test"):
            run(executable, option, env=environment)

    # Isolated DESTDIR roots exercise real system and user prefix layouts.
    with tempfile.TemporaryDirectory(prefix="mdv-install-") as temporary:
        root = Path(temporary)
        for name, prefix in (("system", "/usr"), ("local", "/home/test/.local")):
            destination = root / name
            install_env = os.environ.copy()
            install_env["DESTDIR"] = str(destination)
            for _ in range(2):
                run("cmake", "--install", "build/release", "--prefix", prefix, env=install_env)
            verify_payload(destination / prefix.lstrip("/"))
        # A failing available cache utility must propagate through installation.
        failed = subprocess.run(
            ["cmake", "-DCMAKE_INSTALL_PREFIX=" + str(root / "system/usr"),
             "-Ddesktop_database_tool=/bin/false", "-P", "build/release/cmake_install.cmake"],
            cwd=ROOT, capture_output=True, text=True)
        if failed.returncode == 0 or "RefreshDesktopCaches.cmake" not in failed.stderr:
            raise RuntimeError("Installation did not propagate cache refresh failure")

    run("cmake", "--install", "build/release", "--prefix", ROOT / "build/install")
    verify_payload(ROOT / "build/install")
