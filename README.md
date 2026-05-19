[![clang-format](https://github.com/eisenwave/code-point-names/actions/workflows/clang-format.yml/badge.svg)](https://github.com/eisenwave/code-point-names/actions/workflows/clang-format.yml)
[![Tests](https://github.com/eisenwave/code-point-names/actions/workflows/cmake.yml/badge.svg)](https://github.com/eisenwave/code-point-names/actions/workflows/cmake.yml)

# `get_code_point_name` — Unicode 14 code-point name lookup

Single-header C++ library (`include/get_code_point_name.hpp`)
that maps a Unicode code point to its official name as defined in the Unicode standard.

## API

The library provides a single function:

```cpp
namespace get_code_point_name {
  std::size_t get_code_point_name(char32_t code_point, char* out) noexcept;
}
```

*Effects*:
If `code_point` has a code point name,
appends that name to `out` and returns the length of the name.
Otherwise, `out` is unmodified, and returns zero.

### Usage example

```cpp
#include <get_code_point_name.hpp>

char buf[get_code_point_name::max_length];
std::size_t len = get_code_point_name::get_code_point_name(U'A', buf);
// buf[0..len-1] == "LATIN CAPITAL LETTER A"
```

## Build

Requires: CMake ≥ 3.18, a C++23-capable compiler (for the generator),
and an internet connection on first configure (to download `UnicodeData.txt`).

```sh
mkdir build && cd build
cmake ..
cmake --build .
ctest
```

The header is regenerated from `UnicodeData.txt` (Unicode 14.0)
by the `namesgen` tool and written to `build/include/get_code_point_name.hpp`.
