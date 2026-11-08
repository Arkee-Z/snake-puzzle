#!/usr/bin/env python3
"""Package one terminal build into a relocatable release archive."""

from __future__ import annotations

import argparse
import hashlib
import shutil
import stat
import tarfile
import tempfile
import zipfile
from pathlib import Path


def write_quickstart(path: Path, version: str, platform: str, executable: str) -> None:
    path.write_text(
        f"""Snake Puzzle {version} - {platform}
====================================

Run:
  {executable}
  {executable} maps/tutorial.map
  {executable} --seed 1234
  {executable} --help

Turn mode:
  w/a/s/d [1-99]    move (Enter submits)
  w 2; d 3          command sequence
  p                 one BFS autopilot step
  auto              continuous BFS autopilot
  v debug HUD   r restart   ? help   q quit

Real-time mode:
  WASD steers immediately; the snake moves by itself.
  Reach E to finish. Walls and your own body are fatal.

Map: # wall  . floor  * food  D door  @ head  o body  E exit
The atom-free, fixed-array terminal version writes no save data.
""",
        encoding="utf-8",
    )


def add_tree(tar: tarfile.TarFile, root: Path, arc_root: str) -> None:
    for item in sorted(root.rglob("*")):
        arcname = f"{arc_root}/{item.relative_to(root).as_posix()}"
        tar.add(item, arcname=arcname, recursive=False)


def add_zip_tree(archive: zipfile.ZipFile, root: Path, arc_root: str) -> None:
    for item in sorted(root.rglob("*")):
        arcname = f"{arc_root}/{item.relative_to(root).as_posix()}"
        archive.write(item, arcname=arcname)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", default="build/sp", help="built terminal executable")
    parser.add_argument("--platform", required=True, help="e.g. macos-arm64, linux-x86_64")
    parser.add_argument("--version", required=True, help="release version")
    parser.add_argument("--output-dir", default="dist", help="archive destination")
    parser.add_argument("--maps", default="maps", help="map directory")
    args = parser.parse_args()

    binary = Path(args.binary)
    maps = Path(args.maps)
    output = Path(args.output_dir)
    if not binary.is_file():
        raise SystemExit(f"error: binary not found: {binary}")
    if not maps.is_dir():
        raise SystemExit(f"error: maps not found: {maps}")

    version = args.version.lstrip("v")
    root_name = f"snake-puzzle-{version}-{args.platform}"
    is_windows = args.platform.startswith("windows")
    exe_name = "sp.exe" if is_windows else "sp"
    archive_suffix = ".zip" if is_windows else ".tar.gz"
    archive_path = output / f"{root_name}{archive_suffix}"
    output.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="sp-package-") as temp:
        stage = Path(temp) / root_name
        stage.mkdir()
        target = stage / exe_name
        shutil.copy2(binary, target)
        if not is_windows:
            target.chmod(target.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

        shutil.copytree(maps, stage / "maps")
        for name in ("README.md", "devlog.md", "LICENSE"):
            shutil.copy2(name, stage / name)
        write_quickstart(stage / "QUICKSTART.txt", version, args.platform, exe_name)

        if is_windows:
            (stage / "run.bat").write_text(
                '@echo off\r\ncd /d "%~dp0"\r\nsp.exe %*\r\n', encoding="utf-8"
            )
        else:
            script = stage / "run.sh"
            script.write_text(
                '#!/bin/sh\nDIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)\n'
                'cd "$DIR" || exit 1\nexec ./sp "$@"\n',
                encoding="utf-8",
            )
            script.chmod(0o755)
        if args.platform.startswith("macos"):
            command = stage / "run.command"
            command.write_text(
                '#!/bin/sh\nDIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)\n'
                'cd "$DIR" || exit 1\nexec ./sp "$@"\n',
                encoding="utf-8",
            )
            command.chmod(0o755)

        if is_windows:
            with zipfile.ZipFile(archive_path, "w", zipfile.ZIP_DEFLATED) as archive:
                add_zip_tree(archive, stage, root_name)
        else:
            with tarfile.open(archive_path, "w:gz") as archive:
                add_tree(archive, stage, root_name)

    print(f"packaged {archive_path}")
    print(f"size     {archive_path.stat().st_size} bytes")
    print(f"sha256   {sha256(archive_path)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
