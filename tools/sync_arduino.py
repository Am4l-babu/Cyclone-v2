#!/usr/bin/env python3
"""Copy the game sources from src/ into the Arduino IDE sketch folder.

PlatformIO builds src/. The Arduino IDE needs a sketch folder whose main
file is named after the folder, so this script copies every game file and
renames main.cpp to CycloneTargetLock.ino.

Run from anywhere:  python tools/sync_arduino.py
"""
import shutil
from pathlib import Path

root = Path(__file__).resolve().parent.parent
src = root / "src"
dst = root / "arduino" / "CycloneTargetLock"

dst.mkdir(parents=True, exist_ok=True)

# remove stale copies first so deleted files do not linger
for old in dst.iterdir():
    if old.is_file():
        old.unlink()

for f in sorted(src.iterdir()):
    if not f.is_file() or f.suffix not in (".cpp", ".h"):
        continue
    target = dst / ("CycloneTargetLock.ino" if f.name == "main.cpp" else f.name)
    shutil.copyfile(f, target)
    print(f"{f.relative_to(root)}  ->  {target.relative_to(root)}")
