#!/usr/bin/env python3
"""Render a generated Snake Puzzle map dump in the terminal."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

COLORS = {
    "#": "\033[90m",
    ".": "\033[38;5;238m",
    "@": "\033[1;92m",
    "o": "\033[1;32m",
    "*": "\033[1;91m",
    "$": "\033[1;93m",
    "S": "\033[1;95m",
    "s": "\033[38;5;96m",
    "D": "\033[1;95m",
    "L": "\033[1;35m",
    "+": "\033[32m",
    "_": "\033[36m",
    "=": "\033[1;36m",
    "B": "\033[1;33m",
    "~": "\033[1;33m",
    "x": "\033[31m",
    "T": "\033[1;94m",
    "O": "\033[1;96m",
    "K": "\033[1;93m",
    "A": "\033[1;96m",
    "P": "\033[1;95m",
    "2": "\033[1;33m",
    "e": "\033[93m",
    "E": "\033[1;93m",
    "<": "\033[1;34m",
    ">": "\033[1;34m",
    "^": "\033[1;34m",
    "v": "\033[1;34m",
}
RESET = "\033[0m"


def parse_dump(path: Path) -> tuple[dict[str, str], list[dict[str, str]], list[str]]:
    lines = path.read_text(encoding="utf-8").splitlines()
    meta: dict[str, str] = {}
    atoms: list[dict[str, str]] = []
    rows: list[str] = []
    in_map = False

    for line in lines:
        if not in_map:
            if line == "map":
                in_map = True
                continue
            if not line or line.startswith("#"):
                continue
            if line.startswith("atom "):
                parts = line.split()
                if len(parts) >= 6:
                    atoms.append(
                        {
                            "index": parts[1],
                            "name": parts[2],
                            "x": parts[3],
                            "y": parts[4],
                            "w": parts[5],
                            "h": parts[6] if len(parts) > 6 else "?",
                        }
                    )
                continue
            key, _, value = line.partition(" ")
            meta[key] = value
        else:
            if line == "end":
                break
            rows.append(line)

    if not rows:
        raise ValueError("map section not found")
    return meta, atoms, rows


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("map", type=Path, help="map dump produced by --dump")
    parser.add_argument("--no-color", action="store_true", help="disable ANSI colors")
    parser.add_argument("--plain", action="store_true", help="also hide box decoration")
    args = parser.parse_args()

    try:
        meta, atoms, rows = parse_dump(args.map)
    except (OSError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    title = "SNAKE PUZZLE MAP"
    print(title)
    print("=" * len(title))
    seed = meta.get("seed", "?")
    size = meta.get("size", "?")
    print(f"seed {seed}   size {size}   valid {meta.get('valid', '?')}")
    print(f"atoms {meta.get('registry', '?')}   seals {meta.get('seals', '?')}   "
          f"branches {meta.get('branches', '?')}")
    print(f"reference witness: {meta.get('witness_steps', '?')} steps / "
          f"{meta.get('witness_commands', '?')} commands   "
          f"chain {meta.get('chain_length', '?')}")
    if atoms:
        for atom in atoms:
            print(f"  chain {atom['index']}: {atom['name']} at "
                  f"({atom['x']},{atom['y']}) {atom['w']}x{atom['h']}")
    print()

    color = not args.no_color and sys.stdout.isatty()
    for row in rows:
        if color:
            print("".join(f"{COLORS.get(ch, '')}{ch}{RESET}" for ch in row))
        else:
            print(row)
    print()

    legend = [
        ("@", "snake head"), ("o", "snake body"), ("#", "wall"), ("*", "fruit"),
        ("$", "golden fruit"), ("S", "seal"), ("D", "special gate"),
        ("L", "locked door"), ("_", "pressure plate"), ("B", "push block"),
        ("~", "fracture"), ("T", "teleporter"), ("O", "chamber orb"),
        ("K", "key"), ("A", "shield"), ("P", "phase"), ("2", "double"),
    ]
    if not args.plain:
        print("  ".join(f"{glyph}={label}" for glyph, label in legend[:8]))
        print("  ".join(f"{glyph}={label}" for glyph, label in legend[8:]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
