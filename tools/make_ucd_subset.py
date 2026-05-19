#!/usr/bin/env python3
"""
Generate a reduced ucd.nounihan.flat.xml for fast development iteration.

Keeps:
  - Every Nth <char> element from the repertoire (stride = N)
  - All non-<char> content (description, blocks, named-sequences, etc.)
  - The full XML prologue and structure

Usage:
    python3 tools/make_ucd_subset.py <input.xml> <output.xml> <stride>

Examples:
    # ~11 code points (one every 10 000th)
    python3 tools/make_ucd_subset.py build/ucd_full/14.0/ucd.nounihan.flat.xml \
        build/ucd/14.0/ucd.nounihan.flat.xml 10000

    # ~52 code points (one every 1 000th)
    python3 tools/make_ucd_subset.py ... 1000

    # ~520 code points (one every 100th)
    python3 tools/make_ucd_subset.py ... 100

    # ~5 200 code points (one every 10th)
    python3 tools/make_ucd_subset.py ... 10

    # Full dataset: just copy (stride=1) or restore from backup
"""

from __future__ import annotations

import re
import sys
from pathlib import Path


# Code points that must always be present so that static_assert checks in
# src/unicode.cpp compile correctly regardless of the stride.
ANCHOR_CPS: frozenset[int] = frozenset([
    0x0031,  # '1' DIGIT ONE
    0x0043,  # 'C' LATIN CAPITAL LETTER C
    0x0061,  # 'a' LATIN SMALL LETTER A
    0x00DF,  # 'ß' LATIN SMALL LETTER SHARP S
    0x1F389, # 🎉 PARTY POPPER
    0x1F3F3, # 🏳 WHITE FLAG
    0x1F929, # 🤩 STAR-STRUCK
    0x1F99D, # 🦝 RACCOON
])

_CP_RE = re.compile(r'<char cp="([0-9A-Fa-f]+)"')

def make_subset(src: Path, dst: Path, stride: int) -> None:
    text = src.read_text(encoding="utf-8")

    # Split into lines for easy processing.
    lines = text.splitlines(keepends=True)

    out: list[str] = []
    char_index = 0          # count of <char cp=...> elements seen
    inside_char_block = False   # tracks multi-line <char> elements
    keep_current_block = False  # whether the current multi-line block is kept

    for line in lines:
        stripped = line.lstrip()

        # Detect the start of a <char cp="..."> element.
        if stripped.startswith("<char ") and 'cp="' in stripped:
            m = _CP_RE.match(stripped)
            cp_val = int(m.group(1), 16) if m else -1
            keep_current_block = (char_index % stride == 0) or (cp_val in ANCHOR_CPS)
            if keep_current_block:
                out.append(line)
            # Self-closing element ends here; multi-line needs tracking.
            if not (stripped.rstrip().endswith("/>") or "</char>" in stripped):
                inside_char_block = True
            char_index += 1
            continue

        # If we're inside a multi-line <char> block, follow the keep/skip decision.
        if inside_char_block:
            if keep_current_block:
                out.append(line)
            # Only end the block on </char>, not on self-closing children.
            if "</char>" in stripped:
                inside_char_block = False
            continue

        # Everything else (headers, other sections, non-cp chars) is kept as-is.
        out.append(line)

    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_text("".join(out), encoding="utf-8")

    # Report
    total_in = char_index
    total_out = (char_index + stride - 1) // stride
    print(f"Subset: kept {total_out} of {total_in} <char cp=...> elements (stride={stride})")
    print(f"Output: {dst}  ({dst.stat().st_size // 1024} KB, was {src.stat().st_size // 1024} KB)")


def main() -> None:
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <input.xml> <output.xml> <stride>", file=sys.stderr)
        sys.exit(1)

    src = Path(sys.argv[1])
    dst = Path(sys.argv[2])
    stride = int(sys.argv[3])

    if stride < 1:
        print("stride must be >= 1", file=sys.stderr)
        sys.exit(1)

    make_subset(src, dst, stride)


if __name__ == "__main__":
    main()
