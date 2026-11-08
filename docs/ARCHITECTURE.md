> **Historical prototype document.** This describes the early raylib implementation.
> The current product direction is the terminal-based SP design in the root README.

# Architecture

## 1. Runtime layers

```text
┌─────────────────────────────────────────────────────────────┐
│ raylib UI                         headless CLI / dump / test │
│ src/ui/render_raylib.c            src/ui/render_cli.c        │
│ src/main.c                        src/main.c                 │
└──────────────────────┬──────────────────────────────────────┘
                       │ App / Game / Event APIs
┌──────────────────────▼──────────────────────────────────────┐
│ application flow                                             │
│ src/app.c — runs, execution state, chamber pause/resume      │
└──────────────────────┬──────────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────────┐
│ core simulation                                             │
│ board · snake · command parser · game_step · events · score │
│ entity tables: terrain · door · item · food                 │
└──────────────────────┬──────────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────────┐
│ generation and proof                                         │
│ atom loader · dependency graph · generator · validator      │
│ reference simulation · chamber reachability BFS              │
└─────────────────────────────────────────────────────────────┘

Python tools are outside the runtime boundary and only read/write
text configs and generator dumps.
```

The core never includes raylib. Tests link `core + gen + app` without the
graphics layer. `app.c` also avoids raylib so it can be exercised headlessly.

## 2. Modules

| Module | Responsibility |
| --- | --- |
| `core/board` | fixed grid, bounds, cell storage, terrain glyphs |
| `core/snake` | body array, growth, self-occupancy tests |
| `core/command` | strict `w 3; d 2` parser and formatting |
| `core/event` | bounded ring buffer of typed events |
| `core/game` | authoritative state and one-step resolution |
| `core/rng` | SplitMix64 deterministic random source |
| `entity/terrain` | data table for passability/phase/symbol |
| `entity/item` | table of item effects, symbols, scores |
| `entity/food` | food kinds and base scores |
| `entity/door` | locked/special/mini-exit gate metadata |
| `gen/atom` | text/SPA atom parser and stamping |
| `gen/dependency` | graph construction, acyclicity, topo order |
| `gen/generator` | full map construction and retry loop |
| `gen/solver` | reference simulation; reachability BFS |
| `gen/validator` | static checks + witness proof for full level |
| `ui/render_cli` | ASCII dump, terminal view, Python handoff format |
| `ui/render_raylib` | window, board, HUD, command field, overlays |
| `app` | menu-free run flow, execution queue, chamber transitions |

## 3. Authoritative step order

`game_step()` resolves one movement action in this order:

1. reject dead/invalid/pending-transition states;
2. reject reverse direction;
3. bounds and wall check (phase charge may dissolve one wall);
4. one-way gate direction check;
5. locked door check / key consumption;
6. special gate activation check;
7. mini exit activation check;
8. push block (slide to obstacle or pressure plate);
9. pit check;
10. teleport entry and destination validation;
11. grow decision from destination food;
12. self-collision check (tail may vacate);
13. shift body / grow;
14. resolve destination: food, item, seal, orb, plate, fracture;
15. collapse fractured floors no longer occupied;
16. trigger special gate entry or mini-level completion;
17. replace eaten food if an empty cell exists.

Events are emitted at each observable action, so the renderer never inspects
simulation internals to invent messages. The death reason is stored in
`Game.death_reason` for the game-over overlay.

## 4. App execution state

The app owns an explicit execution cursor:

```text
CommandSequence + command_index + steps_left + step_timer
```

- A submitted sequence is parsed completely before any step executes. A parse
  error moves nothing.
- `app_tick()` consumes one step per timer expiry, then advances the cursor.
- On death, the cursor is cleared and the session best is updated.
- On special-gate entry, the main cursor is paused and saved. The chamber gets
  its own `Game` and its own command sequence.
- On chamber completion, the main gate is marked used, the bonus is added, and
  the paused main sequence resumes. Remaining steps are never undone.
- If chamber generation fails, the gate is consumed with no reward and the main
  run continues; this path is not expected with the shipped atom library.

## 5. Generation pipeline

`generator_generate()` runs up to 48 deterministic attempts:

1. **Select atoms** — weighted, without replacement when possible, compatible
   entry rows/heights, total width <= 12.
2. **Build graph** — linear dependency graph, acyclic check.
3. **Initialize board** — every cell wall; carve safe start pocket.
4. **Stamp core atoms** — walls, locked doors, plates, blocks, teleporters,
   one-way gates, fractures, items and food.
5. **Place seals** — one at each atom exit.
6. **Place special gate** — requires both seals.
7. **Carve optional loops** — 1–2 reward loops off the post-chain corridor
   where geometry allows; 0 only on very narrow custom grids.
8. **Populate rewards** — food and one-shot items in optional loops.
9. **Build witness** — start connector + atom solutions + connectors + final
   corridor steps; long runs are split into legal 1–9 commands.
10. **Validate** — copy the game and execute the witness through `game_step`;
    require no death, both seals and entry into the special gate. The validator
    also checks initial snake cells and gate metadata.
11. **Commit or retry** — on failure, seed the next attempt from the attempt
    index and repeat. Output seed is restored to the requested seed.

The generator never decides a map is solvable from geometry alone. The
reference solution must survive the real engine. This catches body collisions,
block slides, teleport ordering, one-way gates and fracture timing.

## 6. Special chamber

`generator_generate_special()`:

- creates a 13×9 chamber (configurable via tests);
- carves an open room and sparse random pillars;
- guarantees a clear starting pocket;
- places three orbs not in dead-end cells;
- places one sealed exit;
- validates connectivity of every orb and the exit with a BFS that ignores the
  closed mini-exit gate;
- retries deterministically if any target is unreachable.

When all orbs are collected, `game_refresh_gates()` opens the exit. Entering it
returns `STEP_MINI_COMPLETE`, which the app turns into a main-map bonus.

## 7. Determinism

- One RNG type (`SplitMix64`) feeds generation and runtime food respawn.
- No `rand()`, time or global mutable random state is used by core decisions.
- The same initial seed, atom library, board size and build produce the same
  map. Generation attempts are derived from the seed, not wall-clock time.
- App run N uses `base_seed + N * constant`, so death always rolls a new but
  reproducible route.
- `--seed` is displayed in the HUD and in map dumps.

## 8. Events

`EventQueue` is a fixed 256-event ring buffer. It carries event type, cell,
score delta and a human-readable message. Consumers:

- `App`: converts messages into the event log that the raylib HUD shows.
- `render_cli`: ignores the queue but shares the same state queries.
- tests: can poll events to assert actions.

Core code never calls rendering or logging functions.

## 9. Extending the system

### Add a terrain

1. Add enum value in `entity/terrain.h`.
2. Add table row in `entity/terrain.c`.
3. Implement the special transition in `core/game.c` if it is not a simple
   passability rule.
4. Add a glyph and color path in both renderers.
5. Add a test.

### Add an item

1. Add enum and table row in `entity/item.*`.
2. Add effect branch in `game_apply_item()`.
3. Add HUD inventory slot and renderer glyph.
4. Add a test.

### Add an atom

No C changes are required:

1. Create `assets/atoms/atom_name.txt`.
2. Mark west-edge `E`, east-edge `X` on the same row.
3. Add a working `solution` witness.
4. Run `python3 tools/validate_atoms.py`.
5. Run `python3 tools/export_atoms.py` if a compact SPA1 build is desired.

### Replace the UI

A new UI only needs to drive:

```text
app_init / app_new_run / app_input_* / app_submit_command
app_tick / app_handle_restart
read App.main_game, App.special_game, App.active, App.notice, App.log
```

No simulation rule lives in `render_raylib.c`.

## 10. Known boundaries

- Atom orientation is horizontal west→east. Rotation is not implemented.
- Main chain is two core atoms; scaling to more atoms would require dynamic
  width budgeting and chain-aware branch placement.
- The reference proof focuses on the initial snake body. Player-induced growth
  or a deliberately ruined push puzzle can still make the current map
  unsolvable. This is intentional roguelike consequence, not a fresh-map bug.
- Mini chambers are connectivity-validated, not exhaustively body-solved.
- Best score is session-only; the project writes no player data.
- Python tools are not runtime dependencies and are never invoked from C.
