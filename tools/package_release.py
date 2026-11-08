#!/usr/bin/env python3
"""Create a relocatable release archive for one platform build."""

from __future__ import annotations

import argparse
import hashlib
import os
import shutil
import stat
import tarfile
import tempfile
import zipfile
from pathlib import Path


def write_quickstart(path: Path, version: str, platform: str, executable: str) -> None:
    text = f"""Snake Puzzle {version} — {platform}
========================================

Run:
  {executable}

Command examples:
  {executable} --seed 1234
  {executable} --seed 1234 --step-ms 180
  {executable} --batch 100 --seed 1000
  {executable} --validate-seed 42 --dump map42.txt

Controls:
  Type commands in the bottom field and press Enter.
  Command syntax: w 3; d 2; s 1; a 4
  F1  field manual
  Esc quit

The game resolves its atom registry from:
  1. SNAKE_PUZZLE_ATOMS (environment variable)
  2. assets/atoms next to this file
  3. the current working directory

No save data is written. The best score is session-scoped.
See README.md for the full rules, architecture and source.
"""
    path.write_text(text, encoding="utf-8")


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
    parser.add_argument("--binary", required=True, help="built game executable")
    parser.add_argument("--platform", required=True, help="e.g. macos-arm64, linux-x86_64")
    parser.add_argument("--version", required=True, help="release version, e.g. 1.1.0")
    parser.add_argument("--output-dir", default="dist", help="archive destination")
    parser.add_argument("--assets", default="assets/atoms", help="atom registry directory")
    parser.add_argument("--docs", default="docs", help="documentation directory")
    parser.add_argument("--no-docs", action="store_true", help="omit docs/ from the package")
    args = parser.parse_args()

    binary = Path(args.binary)
    assets = Path(args.assets)
    docs = Path(args.docs)
    output = Path(args.output_dir)
    if not binary.is_file():
        raise SystemExit(f"error: binary not found: {binary}")
    if not assets.is_dir():
        raise SystemExit(f"error: atom assets not found: {assets}")
    if not docs.is_dir() and not args.no_docs:
        raise SystemExit(f"error: docs not found: {docs}")

    version = args.version.lstrip("v")
    root_name = f"snake-puzzle-{version}-{args.platform}"
    output.mkdir(parents=True, exist_ok=True)

    exe_name = binary.name
    if args.platform.startswith("windows") and not exe_name.lower().endswith(".exe"):
        exe_name = "snake_puzzle.exe"
    is_windows = args.platform.startswith("windows")
    archive_suffix = ".zip" if is_windows else ".tar.gz"
    archive_path = output / f"{root_name}{archive_suffix}"

    with tempfile.TemporaryDirectory(prefix="snake-puzzle-package-") as temp:
        stage = Path(temp) / root_name
        stage.mkdir()

        target_binary = stage / exe_name
        shutil.copy2(binary, target_binary)
        if not is_windows:
            target_binary.chmod(target_binary.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP |
                                stat.S_IXOTH)

        shutil.copytree(assets, stage / "assets" / "atoms")
        shutil.copy2("README.md", stage / "README.md")
        shutil.copy2("LICENSE", stage / "LICENSE")
        if not args.no_docs:
            shutil.copytree(docs, stage / "docs")
        write_quickstart(stage / "QUICKSTART.txt", version, args.platform, exe_name)

        if is_windows:
            (stage / "run.bat").write_text(
                '@echo off\r\n"%~dp0snake_puzzle.exe" %*\r\n', encoding="utf-8"
            )
        else:
            run_script = stage / "run.sh"
            run_script.write_text(
                '#!/bin/sh\nDIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)\n'
                'exec "$DIR/snake_puzzle" "$@"\n',
                encoding="utf-8",
            )
            run_script.chmod(0o755)
        if args.platform.startswith("macos"):
            command = stage / "run.command"
            command.write_text(
                '#!/bin/sh\nDIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)\n'
                'exec "$DIR/snake_puzzle" "$@"\n',
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
