# Puzzle Atom Format

Each atom is one small, independently verified puzzle unit. Atoms are loaded at
runtime; adding one does not require recompiling the C core.

## File format

```text
# optional comments before layout
name key_door
difficulty 1
weight 4
layout
#####
#.K.#
E.L.X
#.#.#
#####
end
solution
d 1; w 1; d 1; s 1; d 2
end
```

- `name` is unique and uses no spaces.
- `difficulty` is metadata (1+).
- `weight` is the weighted-random selection weight (1+).
- `layout` rows must all have the same width, maximum 10×10.
- Exactly one `E` on the west edge (`x = 0`) and one `X` on the east edge
  (`x = width - 1`), on the same row.
- `solution` is a command sequence using `w/a/s/d` and step counts `1-9`.
  It is the reference witness used by the offline validator and by the
  generator when it builds a full-level proof.

Atoms are currently stamped horizontally: entry from the west, exit to the
east. The row of `E`/`X` determines the vertical alignment.

## Markers

| Marker | Meaning |
| --- | --- |
| `#` | wall |
| `.` | floor |
| `E` | entry (west edge, becomes floor) |
| `X` | exit (east edge, generator adds a seal) |
| `B` | pushable block |
| `_` | pressure plate |
| `L` | locked door |
| `<` `>` `^` `v` | one-way gate facing left/right/up/down |
| `T` | teleporter; pair by file order |
| `~` | fractured floor |
| `K` | key |
| `A` | shield |
| `P` | phase charge |
| `2` | double-next-fruit |
| `*` | normal food |
| `$` | golden food |

All locked doors in one atom are linked to all pressure plates in that atom.
Teleporters must appear in pairs.

## Rules for authoring

- The witness must start at `E` and finish at `X` without dying.
- The atom should have one clear solution; optional red herrings are fine.
- Do not put a seal on the path yourself. The generator adds it at `X`.
- Keep the exit row and entry row identical.
- Prefer mechanisms that still work when the snake body is a few segments
  longer than the initial length.
- Run the validator after editing:

```bash
python3 tools/validate_atoms.py
python3 tools/validate_atoms.py --atoms path/to/one_atom.txt
```

## Compact SPA1 export

`tools/export_atoms.py` produces a single file the C loader also accepts:

```bash
python3 tools/export_atoms.py --output build/atoms.spa
./build/snake_puzzle --atoms build/atoms.spa --batch 20 --seed 1000
```

SPA1 is a line-oriented format:

```text
SPA1
BEGIN name|difficulty|weight|commands
layout rows...
END
```

The C parser handles both this file and the per-atom `.txt` directory.
