"""Small C++ quality commands; only application-owned files are checked."""
import argparse
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=["format", "format-check", "tidy"])
    args = parser.parse_args()
    sources = sorted(p for directory in (ROOT / "src", ROOT / "tests")
                     for p in directory.rglob("*") if p.suffix in (".cpp", ".h"))
    if args.command.startswith("format"):
        flags = ["-i"] if args.command == "format" else ["--dry-run", "--Werror"]
        subprocess.run([os.environ.get("CLANG_FORMAT", "clang-format"), *flags,
                        *map(str, sources)], check=True)
        return
    build = ROOT / "build/debug"
    # GCC-only driver flags are unsupported by clang-tidy. Preserve the real database.
    database = (build / "compile_commands.json").read_text()
    for flag in ("-mno-direct-extern-access", "-fno-keep-inline-dllexport"):
        database = database.replace(flag, "")
    tidy_build = ROOT / "build/tidy"
    tidy_build.mkdir(parents=True, exist_ok=True)
    (tidy_build / "compile_commands.json").write_text(json.dumps(json.loads(database), indent=2))
    for source in sources:
        if source.suffix == ".cpp":
            subprocess.run([os.environ.get("CLANG_TIDY", "clang-tidy"), str(source),
                            "-p", str(tidy_build), "--quiet"], check=True)


if __name__ == "__main__":
    main()
