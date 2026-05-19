# Build Recovery Notes — `simplify-dependencies` branch

## What was refactored

The `simplify-dependencies` branch removed several heavy dependencies
(`fmt`, `range-v3`, TBB, `<execution>`) and upgraded the project to C++23.
The tools now use `std::print`, `std::ranges`, and `std::format` instead
of their library equivalents. ASAN+UBSan was added to the test targets to
catch bugs shaken loose by the refactor.

## Current broken state

| Issue | File | Status |
|---|---|---|
| `cp_to_name.hpp` missing | `build/include/cedilla/` | Not yet generated — `namesgen` never ran after refactor |
| `name_to_cp.hpp` stale | `build/include/cedilla/` | Contains `cp_name` from old combined file; needs regeneration |
| Bug: `name & ~0xC0` instead of `h & ~0xC0` | `src/name_to_cp.hpp` | Fixed in working tree (uncommitted) |
| `compare_node` returns `std::tuple` → structured binding issue | `src/name_to_cp.hpp` | Refactored to return struct (uncommitted) |
| Various missing `std::` prefixes on `uint*_t` | multiple | Fixed in HEAD commit `e7331b5` |

## Build pipeline (how headers are generated)

```
ucd.nounihan.flat.xml
  ├─ gen.py ──────────────────────────► build/cedilla/generated_props.hpp
  │                                     build/cedilla/generated_props_extra.hpp
  │   amalgamate.py (src/all.hpp)
  └─────────────────────────────────► build/include/cedilla/properties.hpp

  namesgen (tools/names.cpp)
    reads XML ───────────────────────► build/include/cedilla/cp_to_name.hpp
    cat src/names.hpp ──────────────►   (appended)

  namesreversegen (tools/namesreverse.cpp)
    reads XML ───────────────────────► build/include/cedilla/name_to_cp.hpp
    cat src/name_to_cp.hpp ─────────►   (appended)
```

## Strategy for fast iteration

Because the UCD XML has ~52,000 code points and the generated C++ headers
embed all their data, compiling the tests takes many minutes with Debug+ASAN.

**Solution:** use a subset XML during development.

`tools/make_ucd_subset.py` creates a reduced `ucd.nounihan.flat.xml` that
keeps only every *N*th code point (plus the full XML structure and non-char
sections). Start with N=10000 (~11 code points), verify tests pass, then
reduce N progressively until you have the full dataset.

```
# Create subset (keep every 500th code point — ~104 entries)
python3 tools/make_ucd_subset.py \
    build/ucd_full/14.0/ucd.nounihan.flat.xml \
    build/ucd/14.0/ucd.nounihan.flat.xml \
    500

# After tests pass, increase coverage:
python3 tools/make_ucd_subset.py ... 100   # ~520 entries
python3 tools/make_ucd_subset.py ... 10    # ~5200 entries
# Full dataset: restore from build/ucd_full/
```

## Release build plan

Once all tests pass with the full dataset at Debug+ASAN:
1. Turn off ASAN: comment out `-fsanitize=...` lines in `tests/CMakeLists.txt`
   and the `target_compile_options`/`link_options` in the root `CMakeLists.txt`
2. Build Release: `cmake -DCMAKE_BUILD_TYPE=Release ..` + full rebuild
3. Run tests in Release
4. Re-enable ASAN for final Release+ASAN pass (optional)

## Key source files

| File | Purpose |
|---|---|
| `src/name_to_cp.hpp` | Trie traversal for `uni::cp_from_name()` — appended to generated header |
| `src/names.hpp` | `name_view` iterator for `uni::cp_name()` — appended to generated header |
| `tools/names.cpp` | Generator: XML → cp_to_name.hpp data tables |
| `tools/namesreverse.cpp` | Generator: XML → name_to_cp.hpp trie data |
| `tools/gen.py` | Generator: XML → properties.hpp data tables |
| `tools/amalgamate.py` | Single-header amalgamation (replaced quom) |
| `tools/make_ucd_subset.py` | Subset XML generator for fast dev iteration |
