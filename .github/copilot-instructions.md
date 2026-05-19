# get_code_point_name — project overview

Single-header C++ library that maps a Unicode 17.0 code point to its official name.
No heap allocation anywhere in the lookup path.

Public API (`include/get_code_point_name.hpp`, also committed at that path):
```cpp
#include <get_code_point_name.hpp>
char buf[get_code_point_name::cp_name_max_length]; // 96 bytes
std::size_t len = get_code_point_name::get_code_point_name(U'A', buf);
// buf[0..len-1] == "LATIN CAPITAL LETTER A"; returns 0 if no name.
```

## Key files

| Path | Purpose |
|------|---------|
| `namesgen.cpp` | C++23 code generator — reads `UnicodeData.txt`, runs greedy substring compression, writes compressed name tables into `build/include/get_code_point_name.gen` |
| `names.hpp` | Template file: runtime lookup logic with a `// AUTO-GENERATED CODE HERE` marker that the build splices the tables into |
| `include/get_code_point_name.hpp` | Committed copy of the generated header; CI verifies it is bitwise-identical to the build output |
| `test.cpp` | Test binary; reads `UnicodeData.txt` and verifies every stored name round-trips correctly |
| `CMakeLists.txt` | Single CMake file; downloads `UnicodeData.txt` into the build dir on first configure if absent |
| `cmake/splice.cmake` | Replaces `// AUTO-GENERATED CODE HERE` in `names.hpp` with generated tables, writes final header |

## Build & test

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Requires: CMake ≥ 3.18, GCC 13+ (or equivalent), `clang-format-20` on `PATH`.

`UnicodeData.txt` is downloaded into `build/` automatically on first configure. Pre-downloading it avoids firewall issues:
```sh
curl -fL https://unicode.org/Public/14.0.0/ucd/UnicodeData.txt -o build/UnicodeData.txt
```
