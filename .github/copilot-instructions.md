# Code point names — project overview

Single-header C++17 library that maps a Unicode 14.0 code point to its official name.

Public API (`include/cedilla/cp_to_name.hpp`, generated at build time):
```cpp
#include <cedilla/cp_to_name.hpp>
std::string name = uni::cp_name(U'A').to_string(); // "LATIN CAPITAL LETTER A"
```
Algorithmically-derived names (Hangul syllables, CJK Unified Ideographs, etc.) are not stored; `cp_name()` returns an empty string for those.

## Key files

| Path | Purpose |
|------|---------|
| `namesgen.cpp` | C++23 code generator — reads `UnicodeData.txt`, runs greedy substring compression, writes compressed name tables into `build/include/cedilla/cp_to_name.hpp` |
| `src/names.hpp` | Runtime lookup logic, appended to the generated header |
| `test.cpp` | Single test binary; reads `UnicodeData.txt` and verifies every stored name round-trips correctly |
| `CMakeLists.txt` | Single CMake file; downloads `UnicodeData.txt` into the build dir on first configure if absent |

## Build & test

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Requires: CMake ≥ 3.18, GCC 13+ (or equivalent), `clang-format` on `PATH` (used as a post-generation formatting step — CMake will error if it is missing).

`UnicodeData.txt` is downloaded into `build/` automatically on first configure. Pre-downloading it avoids firewall issues:
```sh
curl -fL https://unicode.org/Public/14.0.0/ucd/UnicodeData.txt -o build/UnicodeData.txt
```
