#!/usr/bin/env python3
"""Copy the game sources from src/ into the Arduino IDE sketch folder.

PlatformIO builds src/. The Arduino IDE needs a sketch folder whose main
file is named after the folder, so this script copies every game file and
renames main.cpp to CycloneTargetLock.ino.

    python tools/sync_arduino.py           copy src/ -> arduino/CycloneTargetLock/
    python tools/sync_arduino.py --check   only report drift (exit code 1 if any), used by CI
"""
import argparse
import sys
from pathlib import Path

root = Path(__file__).resolve().parent.parent
src = root / "src"
dst = root / "arduino" / "CycloneTargetLock"


def expected():
    """Map each sketch file name to the src/ file it should be a copy of."""
    files = {}
    for f in sorted(src.iterdir()):
        if f.is_file() and f.suffix in (".cpp", ".h"):
            files["CycloneTargetLock.ino" if f.name == "main.cpp" else f.name] = f
    return files


def same(a, b):
    # line endings may differ on Windows checkouts; the content may not
    return a.read_bytes().replace(b"\r\n", b"\n") == b.read_bytes().replace(b"\r\n", b"\n")


def check():
    want = expected()
    have = {f.name for f in dst.iterdir() if f.is_file()} if dst.is_dir() else set()
    problems = []
    for name, f in want.items():
        if name not in have:
            problems.append(f"missing   {name}  (from {f.relative_to(root)})")
        elif not same(f, dst / name):
            problems.append(f"outdated  {name}  (differs from {f.relative_to(root)})")
    for name in sorted(have - set(want)):
        problems.append(f"stale     {name}  (no longer in src/)")
    if problems:
        print("arduino/CycloneTargetLock is out of sync with src/:")
        for p in problems:
            print("  " + p)
        print("Run: python tools/sync_arduino.py")
        return 1
    print("arduino/CycloneTargetLock is in sync with src/")
    return 0


def sync():
    dst.mkdir(parents=True, exist_ok=True)
    # remove stale copies first so deleted files do not linger
    for old in dst.iterdir():
        if old.is_file():
            old.unlink()
    for name, f in expected().items():
        target = dst / name
        target.write_bytes(f.read_bytes())
        print(f"{f.relative_to(root)}  ->  {target.relative_to(root)}")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true",
                        help="do not copy, exit with 1 if the sketch folder differs from src/")
    args = parser.parse_args()
    return check() if args.check else sync()


if __name__ == "__main__":
    sys.exit(main())
