#!/usr/bin/env python3
"""Shared parser for Snake Puzzle atom configuration files."""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
import re

ALLOWED_MARKERS = set("#.EXB_LT<>^vKAP2*$~")
SOLUTION_TOKEN = re.compile(r"([wasdWASD])\s+([1-9])\Z")
DIRECTIONS = {
    "w": (0, -1),
    "d": (1, 0),
    "s": (0, 1),
    "a": (-1, 0),
}
OPPOSITE = {"w": "s", "s": "w", "a": "d", "d": "a"}


class AtomError(ValueError):
    pass


@dataclass
class Command:
    direction: str
    steps: int


@dataclass
class Atom:
    name: str
    difficulty: int
    weight: int
    rows: list[str]
    row: int
    col: int
    exit_row: int
    exit_col: int
    commands: list[Command]
    path: Path
    entry_col: int = 0
    exit_col_end: int = 0
    width: int = field(init=False)
    height: int = field(init=False)
    solution: str = ""

    def __post_init__(self) -> None:
        self.height = len(self.rows)
        self.width = len(self.rows[0]) if self.rows else 0
        self.entry_col = self.col
        self.exit_col_end = self.width - 1


def parse_solution(text: str) -> tuple[list[Command], str]:
    commands: list[Command] = []
    normalized: list[str] = []
    for raw in text.split(";"):
        token = raw.strip()
        if not token:
            continue
        match = SOLUTION_TOKEN.fullmatch(token)
        if not match:
            raise AtomError(f"invalid solution token '{token}' (expected e.g. 'w 2')")
        direction = match.group(1).lower()
        steps = int(match.group(2))
        commands.append(Command(direction, steps))
        normalized.append(f"{direction} {steps}")
    if not commands:
        raise AtomError("solution is empty")
    return commands, "; ".join(normalized)


def parse_atom(path: Path) -> Atom:
    text = path.read_text(encoding="utf-8")
    lines = [line.strip() for line in text.splitlines()]
    name = ""
    difficulty = 1
    weight = 2
    rows: list[str] = []
    solution_text = ""
    state = "meta"
    line_no = 0

    for line in lines:
        line_no += 1
        if state == "meta":
            if not line or line.startswith("#"):
                continue
            if line == "layout":
                state = "layout"
                continue
            if " " not in line:
                raise AtomError(f"{path}:{line_no}: expected 'key value'")
            key, value = line.split(" ", 1)
            value = value.strip()
            if key == "name":
                name = value
            elif key == "difficulty":
                difficulty = int(value)
            elif key == "weight":
                weight = int(value)
            else:
                raise AtomError(f"{path}:{line_no}: unknown metadata '{key}'")
        elif state == "layout":
            if line == "end":
                state = "solution"
                continue
            if len(line) > 10 or len(rows) >= 10:
                raise AtomError(f"{path}:{line_no}: layout exceeds 10x10")
            rows.append(line)
        elif state == "solution":
            if line == "end":
                state = "done"
                continue
            if line == "solution" or not line:
                continue
            solution_text = f"{solution_text} {line}".strip()
        elif line and not line.startswith("#"):
            raise AtomError(f"{path}:{line_no}: unexpected content after solution")

    if not name:
        raise AtomError(f"{path}: missing name")
    if len(rows) < 3 or len(rows[0]) < 3:
        raise AtomError(f"{path}: missing or invalid layout")
    width = len(rows[0])
    if any(len(row) != width for row in rows):
        raise AtomError(f"{path}: inconsistent layout row width")
    for r, row in enumerate(rows):
        for c, marker in enumerate(row):
            if marker not in ALLOWED_MARKERS:
                raise AtomError(f"{path}: unknown marker '{marker}' at {c},{r}")
    entries = [(r, c) for r, row in enumerate(rows) for c, ch in enumerate(row) if ch == "E"]
    exits = [(r, c) for r, row in enumerate(rows) for c, ch in enumerate(row) if ch == "X"]
    if len(entries) != 1 or len(exits) != 1:
        raise AtomError(f"{path}: atom needs exactly one E and one X")
    entry_row, entry_col = entries[0]
    exit_row, exit_col = exits[0]
    if entry_col != 0 or exit_col != width - 1 or entry_row != exit_row:
        raise AtomError(f"{path}: E must be on the west edge and X on the east edge on one row")
    commands, normalized = parse_solution(solution_text)
    atom = Atom(
        name=name,
        difficulty=max(1, difficulty),
        weight=max(1, weight),
        rows=rows,
        row=entry_row,
        col=entry_col,
        exit_row=exit_row,
        exit_col=exit_col,
        commands=commands,
        path=path,
    )
    atom.solution = normalized
    return atom


def load_atoms(path: str | Path) -> list[Atom]:
    root = Path(path)
    if root.is_file():
        return [parse_atom(root)]
    if not root.is_dir():
        raise AtomError(f"atom path does not exist: {root}")
    atoms = []
    for child in sorted(root.iterdir()):
        if child.suffix in {".txt", ".atom"} and child.is_file():
            atoms.append(parse_atom(child))
    if not atoms:
        raise AtomError(f"no atom files found in {root}")
    return atoms
