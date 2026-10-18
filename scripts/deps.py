"""Explicit preparation of the locked third-party provider, outside app configuration."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]


def prepare_kde(dependency, supplied, work, prefix):
    archive = supplied or work / dependency["archive"]
    if not archive.exists():
        if supplied:
            raise SystemExit(f"Offline archive missing: {archive}")
        with urllib.request.urlopen(dependency["url"], timeout=60) as response:
            archive.write_bytes(response.read())
    if hashlib.sha256(archive.read_bytes()).hexdigest() != dependency["sha256"]:
        raise SystemExit(f"Checksum mismatch: {archive}")
    source = work / dependency["source"]
    if not source.exists():
        with tarfile.open(archive) as bundle:
            bundle.extractall(work, filter="data")
    build = work / ("build-" + dependency["source"])
    subprocess.run([
        "cmake", "-S", str(source), "-B", str(build), "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=Release", f"-DCMAKE_INSTALL_PREFIX={prefix}",
        f"-DCMAKE_PREFIX_PATH={prefix}", "-DCMAKE_INSTALL_LIBDIR=lib",
        "-DBUILD_TESTING=OFF", "-DBUILD_QCH=OFF", "-DBUILD_WITH_QT6=ON",
        "-DKF_SKIP_PO_PROCESSING=ON", "-DBUILD_codeeditor=OFF",
        "-DBUILD_codepdfprinter=OFF", "-DBUILD_minimaltest=OFF",
    ], check=True)
    subprocess.run(["cmake", "--build", str(build), "--parallel", "2"], check=True)
    subprocess.run(["cmake", "--install", str(build)], check=True)



def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", type=Path, help="Use an offline archive; checksum is still verified")
    parser.add_argument("--prepare-kde", action="store_true", help="Prepare pinned KDE 6.10 for a Qt 6.8 SDK")
    parser.add_argument("--ecm-archive", type=Path)
    parser.add_argument("--syntax-archive", type=Path)
    args = parser.parse_args()
    dependency = json.loads((ROOT / "dependencies.lock.json").read_text())["qmarkdown"]
    work = ROOT / "build/deps"
    work.mkdir(parents=True, exist_ok=True)
    prefix = work / "prefix"
    if args.prepare_kde:
        lock = json.loads((ROOT / "dependencies.lock.json").read_text())
        prepare_kde(lock["ecm"], args.ecm_archive, work, prefix)
        prepare_kde(lock["syntax-highlighting"], args.syntax_archive, work, prefix)
    elif args.ecm_archive or args.syntax_archive:
        parser.error("KDE archives require --prepare-kde")
    archive = args.archive or work / (dependency["revision"] + ".tar.gz")
    if not archive.exists():
        if args.archive:
            parser.error("The supplied offline archive does not exist")
        temporary = archive.with_suffix(".download")
        with urllib.request.urlopen(dependency["url"], timeout=60) as response:
            with temporary.open("wb") as output:
                while chunk := response.read(1024 * 1024):
                    output.write(chunk)
        temporary.replace(archive)
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    if digest != dependency["sha256"]:
        raise SystemExit(f"Checksum mismatch: {archive}; expected {dependency['sha256']}, got {digest}")
    source = work / ("qmarkdown-" + dependency["revision"])
    if not source.exists():
        with tarfile.open(archive) as bundle:
            bundle.extractall(work, filter="data")
    build = work / ("build-" + dependency["revision"])
    prefix = work / "prefix"
    # Build with the Qt selected by the caller's CMAKE_PREFIX_PATH / Qt6_ROOT.
    subprocess.run([
        "cmake", "-S", str(source), "-B", str(build), "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_SHARED_LIBS=ON",
        "-DBUILD_TESTING=OFF", "-DQMARKDOWN_BUILD_EXAMPLES=OFF",
        "-DQMARKDOWN_BUILD_BENCHMARKS=OFF", "-DQMARKDOWN_WITH_MATH=ON",
        "-DQMARKDOWN_WITH_KSYNTAXHIGHLIGHTING=ON", f"-DCMAKE_PREFIX_PATH={prefix}", "-DCMAKE_INSTALL_LIBDIR=lib",
        "-DQMARKDOWN_QML_INSTALL_DIR=lib/qt6/qml", f"-DCMAKE_INSTALL_PREFIX={prefix}",
    ], check=True)
    subprocess.run(["cmake", "--build", str(build), "--parallel", "2"], check=True)
    subprocess.run(["cmake", "--install", str(build)], check=True)
    print(f"Prepared QMarkdown {dependency['version']} in {prefix}")


if __name__ == "__main__":
    main()
