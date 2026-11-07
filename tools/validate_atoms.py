#!/usr/bin/env python3
"""Validate every puzzle atom independently and print its reference solution."""

from __future__ import annotations

import argparse
import sys
from dataclasses import dataclass, field
from pathlib import Path

from atomlib import Atom, AtomError, DIRECTIONS, OPPOSITE, load_atoms


@dataclass
class Simulation:
    atom: Atom
    walls: set[tuple[int, int]] = field(default_factory=set)
    one_way: dict[tuple[int, int], str] = field(default_factory=dict)
    locked: set[tuple[int, int]] = field(default_factory=set)
    plates: set[tuple[int, int]] = field(default_factory=set)
    blocks: set[tuple[int, int]] = field(default_factory=set)
    teleports: dict[tuple[int, int], tuple[int, int]] = field(default_factory=dict)
    fractures: set[tuple[int, int]] = field(default_factory=set)
    triggered: set[tuple[int, int]] = field(default_factory=set)
    pits: set[tuple[int, int]] = field(default_factory=set)
    items: dict[tuple[int, int], str] = field(default_factory=dict)
    foods: set[tuple[int, int]] = field(default_factory=set)
    exit_cell: tuple[int, int] = (0, 0)
    head: tuple[int, int] = (0, 0)
    body: list[tuple[int, int]] = field(default_factory=list)
    direction: str = "d"
    keys: int = 0
    shields: int = 0
    phases: int = 0
    doors_open: bool = False
    steps: int = 0

    def build(self) -> None:
        teleport_cells: list[tuple[int, int]] = []
        for r, row in enumerate(self.atom.rows):
            for c, marker in enumerate(row):
                p = (c, r)
                if marker == "#":
                    self.walls.add(p)
                elif marker == "<":
                    self.one_way[p] = "a"
                elif marker == ">":
                    self.one_way[p] = "d"
                elif marker == "^":
                    self.one_way[p] = "w"
                elif marker == "v":
                    self.one_way[p] = "s"
                elif marker == "L":
                    self.locked.add(p)
                elif marker == "_":
                    self.plates.add(p)
                elif marker == "B":
                    self.blocks.add(p)
                elif marker == "T":
                    teleport_cells.append(p)
                elif marker == "~":
                    self.fractures.add(p)
                elif marker in "KAP2":
                    self.items[p] = marker
                elif marker in "*$":
                    self.foods.add(p)
                elif marker == "X":
                    self.exit_cell = p
        if len(teleport_cells) % 2 != 0:
            raise ValueError("teleporters must appear in pairs")
        for i in range(0, len(teleport_cells), 2):
            a = teleport_cells[i]
            b = teleport_cells[i + 1]
            self.teleports[a] = b
            self.teleports[b] = a
        self.head = (self.atom.col, self.atom.row)
        self.body = [
            (self.atom.col - 1, self.atom.row),
            (self.atom.col - 2, self.atom.row),
        ]
        self.direction = "d"

    def in_bounds(self, p: tuple[int, int]) -> bool:
        return 0 <= p[0] < self.atom.width and 0 <= p[1] < self.atom.height

    def occupied(self, p: tuple[int, int]) -> int:
        if self.head == p:
            return 0
        try:
            return self.body.index(p) + 1
        except ValueError:
            return -1

    def blocked_for_block(self, p: tuple[int, int]) -> bool:
        if not self.in_bounds(p) or p in self.walls or p in self.pits:
            return True
        if p in self.locked and not self.doors_open:
            return True
        if p in self.one_way or p in self.teleports or p in self.fractures:
            return True
        if p in self.blocks or self.occupied(p) >= 0:
            return True
        if p in self.items or p in self.foods:
            return True
        return False

    def press_plate(self, p: tuple[int, int]) -> None:
        if p in self.plates:
            self.doors_open = True

    def push_block(self, target: tuple[int, int], direction: str) -> bool:
        if target not in self.blocks:
            return True
        dx, dy = DIRECTIONS[direction]
        x, y = target
        moved = False
        while True:
            nxt = (x + dx, y + dy)
            if not self.in_bounds(nxt):
                break
            if nxt in self.plates:
                if self.occupied(nxt) >= 0 or nxt in self.blocks:
                    break
                x, y = nxt
                moved = True
                self.press_plate(nxt)
                break
            if self.blocked_for_block(nxt):
                break
            x, y = nxt
            moved = True
        if not moved:
            return False
        self.blocks.remove(target)
        self.blocks.add((x, y))
        self.press_plate((x, y))
        return True

    def fatal(self, reason: str) -> str | None:
        if self.shields > 0:
            self.shields -= 1
            return None
        return reason

    def collapse(self) -> None:
        for p in list(self.triggered):
            if self.occupied(p) < 0:
                self.triggered.discard(p)
                self.fractures.discard(p)
                self.pits.add(p)

    def move_once(self, direction: str) -> str | None:
        if OPPOSITE[direction] == self.direction:
            return self.fatal("reversed into its own neck")
        dx, dy = DIRECTIONS[direction]
        target = (self.head[0] + dx, self.head[1] + dy)
        teleported = False
        final = target

        if not self.in_bounds(target):
            return self.fatal("hit the atom boundary")
        if target in self.walls:
            if self.phases > 0:
                self.phases -= 1
                self.walls.discard(target)
            else:
                return self.fatal("hit a wall")
        if target in self.pits:
            return self.fatal("fell through a fractured floor")
        if target in self.one_way and self.one_way[target] != direction:
            return self.fatal("entered a one-way gate against its arrow")
        if target in self.locked and not self.doors_open:
            if self.keys <= 0:
                return self.fatal("reached a locked door without a key")
            self.keys -= 1
            self.doors_open = True
        if target in self.blocks and not self.push_block(target, direction):
            return self.fatal("could not push the stone")
        if target in self.teleports:
            dest = self.teleports[target]
            if self.occupied(dest) >= 0 or dest in self.blocks:
                return self.fatal("teleport discharge point is occupied")
            final = dest
            teleported = True

        grow = final in self.foods
        segment = self.occupied(final)
        if segment > 0:
            if segment == len(self.body) and not grow:
                pass
            else:
                return self.fatal("bit into its own body")

        self.body = [self.head] + (self.body if grow else self.body[:-1])
        self.head = final
        self.direction = direction
        self.steps += 1

        if final in self.items:
            item = self.items.pop(final)
            if item == "K":
                self.keys += 1
            elif item == "A":
                self.shields += 1
            elif item == "P":
                self.phases += 1
        if final in self.foods:
            self.foods.discard(final)
        if final in self.plates:
            self.press_plate(final)
        if final in self.fractures:
            self.triggered.add(final)
        self.collapse()
        if teleported:
            self.triggered.discard(final)
        if final == self.exit_cell:
            return None
        return None

    def run(self) -> tuple[bool, str]:
        self.build()
        for command in self.atom.commands:
            for _ in range(command.steps):
                error = self.move_once(command.direction)
                if error is not None:
                    return False, f"{error} after {self.steps} steps"
                if self.head == self.exit_cell:
                    return True, ""
        if self.head == self.exit_cell:
            return True, ""
        return False, f"reference solution ended at {self.head}, not X{self.exit_cell}"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--atoms", default="assets/atoms", help="atom directory or file")
    parser.add_argument("--quiet", action="store_true", help="only print failures")
    args = parser.parse_args()

    try:
        atoms = load_atoms(args.atoms)
    except (AtomError, OSError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    failures = 0
    for atom in atoms:
        try:
            sim = Simulation(atom)
            ok, error = sim.run()
        except (ValueError, KeyError) as exc:
            ok, error = False, str(exc)
        if ok:
            if not args.quiet:
                print(f"OK    {atom.name:<20} {atom.width}x{atom.height}  "
                      f"{len(atom.commands)} commands  solution: {atom.solution}")
        else:
            failures += 1
            print(f"ERROR {atom.name:<20} {error}", file=sys.stderr)
            print(f"      reference: {atom.solution}", file=sys.stderr)

    if failures:
        print(f"\n{failures}/{len(atoms)} atoms failed", file=sys.stderr)
        return 1
    print(f"\n{len(atoms)} atoms validated; every reference solution reaches its exit.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
