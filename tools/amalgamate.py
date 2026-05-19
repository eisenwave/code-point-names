#!/usr/bin/env python3
"""Single-header amalgamation script. Drop-in replacement for quom."""

from __future__ import annotations

import re
import sys
from pathlib import Path
from typing import Optional


def resolve_include(name: str, current_dir: Path, include_dirs: list[Path]) -> Optional[Path]:
    # Search relative to the including file first, then -I dirs (C preprocessor order)
    for base in [current_dir, *include_dirs]:
        candidate = Path(base) / name
        if candidate.exists():
            return candidate.resolve()
    return None


def amalgamate(filepath: Path | str, include_dirs: list[Path], seen: set[Path]) -> str:
    filepath = Path(filepath).resolve()
    if filepath in seen:
        return ""
    seen.add(filepath)

    text = filepath.read_text(encoding="utf-8")
    out = []

    for line in text.splitlines(keepends=True):
        # Drop #pragma once — a single one is emitted at the top of the output
        if re.match(r"\s*#\s*pragma\s+once\s*$", line):
            continue

        # Expand local #include "..."
        m = re.match(r'\s*#\s*include\s*"([^"]+)"', line)
        if m:
            resolved = resolve_include(m.group(1), filepath.parent, include_dirs)
            if resolved is not None:
                out.append(amalgamate(resolved, include_dirs, seen))
                continue

        out.append(line)

    return "".join(out)


def main():
    args = sys.argv[1:]
    include_dirs = []
    positional = []
    i = 0
    while i < len(args):
        if args[i] == "-I":
            include_dirs.append(Path(args[i + 1]).resolve())
            i += 2
        else:
            positional.append(args[i])
            i += 1

    if len(positional) != 2:
        print(f"Usage: {sys.argv[0]} <input.hpp> [-I dir] ... <output.hpp>", file=sys.stderr)
        sys.exit(1)

    entry, output = positional
    seen = set()
    body = amalgamate(entry, include_dirs, seen)

    Path(output).write_text("#pragma once\n" + body, encoding="utf-8")


if __name__ == "__main__":
    main()
