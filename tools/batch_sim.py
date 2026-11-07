#!/usr/bin/env python3
"""Run the C generator in batch and summarize solvability and map metrics."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from collections import Counter
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--count", type=int, default=100, help="number of maps")
    parser.add_argument("--seed", type=int, default=1000, help="first seed")
    parser.add_argument("--width", type=int, default=20)
    parser.add_argument("--height", type=int, default=15)
    parser.add_argument("--binary", default=str(root / "build" / "snake_puzzle"))
    parser.add_argument("--atoms", default=str(root / "assets" / "atoms"))
    parser.add_argument("--json", action="store_true", help="print summary as JSON")
    args = parser.parse_args()

    if args.count <= 0:
        print("ERROR: --count must be positive", file=sys.stderr)
        return 2
    binary = Path(args.binary)
    if not binary.exists():
        print(f"ERROR: binary not found: {binary} (run 'make' first)", file=sys.stderr)
        return 2

    command = [
        str(binary),
        "--batch",
        str(args.count),
        "--seed",
        str(args.seed),
        "--width",
        str(args.width),
        "--height",
        str(args.height),
        "--atoms",
        str(Path(args.atoms)),
    ]
    try:
        proc = subprocess.run(command, check=False, capture_output=True, text=True)
    except OSError as exc:
        print(f"ERROR: could not run generator: {exc}", file=sys.stderr)
        return 2
    if proc.returncode != 0:
        print(proc.stderr.strip() or "generator failed", file=sys.stderr)
        return proc.returncode

    records = []
    for line in proc.stdout.splitlines():
        line = line.strip()
        if not line:
            continue
        try:
            records.append(json.loads(line))
        except json.JSONDecodeError:
            print(f"WARNING: non-JSON generator output: {line}", file=sys.stderr)

    if not records:
        print("ERROR: generator produced no records", file=sys.stderr)
        return 2

    valid = [record for record in records if record.get("valid")]
    invalid = [record for record in records if not record.get("valid")]
    attempts = [record.get("attempts", 0) for record in valid]
    density = [record.get("density", 0.0) for record in valid]
    witness = [record.get("witness_steps", 0) for record in valid]
    chains = [record.get("chain_length", 0) for record in valid]
    branches = Counter(record.get("branches", 0) for record in valid)
    atom_uses: Counter[str] = Counter()
    for record in valid:
        for atom in record.get("atoms", []):
            atom_uses[atom] += 1

    summary = {
        "requested": args.count,
        "generated": len(records),
        "valid": len(valid),
        "invalid": len(invalid),
        "solvable_rate": (len(valid) / len(records)) if records else 0.0,
        "avg_attempts": (sum(attempts) / len(attempts)) if attempts else 0.0,
        "max_attempts": max(attempts) if attempts else 0,
        "avg_density": (sum(density) / len(density)) if density else 0.0,
        "avg_witness_steps": (sum(witness) / len(witness)) if witness else 0.0,
        "avg_chain_length": (sum(chains) / len(chains)) if chains else 0.0,
        "branch_distribution": {str(key): value for key, value in sorted(branches.items())},
        "atom_usage": dict(atom_uses.most_common()),
        "width": args.width,
        "height": args.height,
        "first_seed": args.seed,
        "errors": [
            {"seed": record.get("seed"), "error": record.get("error")}
            for record in invalid[:10]
        ],
    }

    if args.json:
        print(json.dumps(summary, indent=2))
        return 0 if not invalid else 1

    print(f"Snake Puzzle generator batch — {args.width}x{args.height}, seeds "
          f"{args.seed}..{args.seed + args.count - 1}")
    print(f"generated          {summary['generated']}")
    print(f"solvable           {summary['valid']} / {summary['generated']} "
          f"({summary['solvable_rate'] * 100:.1f}%)")
    print(f"generation attempts avg {summary['avg_attempts']:.2f}, "
          f"max {summary['max_attempts']}")
    print(f"floor density      avg {summary['avg_density'] * 100:.1f}%")
    print(f"reference witness  avg {summary['avg_witness_steps']:.1f} steps")
    print(f"dependency chain   avg {summary['avg_chain_length']:.1f} atoms")
    print(f"branches           {summary['branch_distribution']}")
    print("atom usage         " + ", ".join(f"{name}:{count}" for name, count in atom_uses.most_common()))
    if invalid:
        print("\nINVALID MAPS", file=sys.stderr)
        for record in invalid[:10]:
            print(f"  seed {record.get('seed')}: {record.get('error')}", file=sys.stderr)
    return 0 if not invalid else 1


if __name__ == "__main__":
    raise SystemExit(main())
