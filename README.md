[![clang-format](https://github.com/eisenwave/code-point-names/actions/workflows/clang-format.yml/badge.svg)](https://github.com/eisenwave/code-point-names/actions/workflows/clang-format.yml)
[![Tests](https://github.com/eisenwave/code-point-names/actions/workflows/cmake.yml/badge.svg)](https://github.com/eisenwave/code-point-names/actions/workflows/cmake.yml)

# `get_code_point_name` — Unicode 17 code-point name lookup

Single-header C++ library (`include/get_code_point_name.hpp`)
that maps a Unicode code point to its official name as defined in the Unicode standard.

This library is a heavily modified version of
[`cor3ntin/ext-unicode-db`](https://github.com/cor3ntin/ext-unicode-db).

## API

The library provides a function to look up the official character name, plus one function per
Unicode name-alias category (defined in `NameAliases.txt`):

```cpp
namespace get_code_point_name {
  // Official Unicode character name (e.g. "LATIN CAPITAL LETTER A").
  std::size_t get_code_point_name(char32_t code_point, char* out) noexcept;

  // Correction: Corrections for serious problems in the character names
  std::size_t get_code_point_correction_alias(char32_t code_point, char* out) noexcept;

  // Control: ISO 6429 names for C0 and C1 control functions, and other
  //          commonly occurring names for control codes
  std::size_t get_code_point_control_alias(char32_t code_point, char* out) noexcept;

  // Alternate: A few widely used alternate names for format characters
  std::size_t get_code_point_alternate_alias(char32_t code_point, char* out) noexcept;

  // Figment: Several documented labels for C1 control code points which
  //          were never actually approved in any standard
  std::size_t get_code_point_figment_alias(char32_t code_point, char* out) noexcept;

  // Abbreviation: Commonly occurring abbreviations (or acronyms) for control codes,
  //               format characters, spaces, and variation selectors
  std::size_t get_code_point_abbreviation_alias(char32_t code_point, char* out) noexcept;
}
```

Every function writes the alias (or all aliases comma-separated when a code point has multiple
for that category) into `out` and returns the number of bytes written, or returns `0` (leaving
`out` unmodified) when no alias of that category exists for the code point.

### Usage example

```cpp
#include <get_code_point_name.hpp>

char buf[get_code_point_name::max_length];
std::size_t len = get_code_point_name::get_code_point_name(U'A', buf);
// buf[0..len-1] == "LATIN CAPITAL LETTER A"

len = get_code_point_name::get_code_point_correction_alias(U'\N{SCRIPT CAPITAL P}', buf);
// buf[0..len-1] == "WEIERSTRASS ELLIPTIC FUNCTION"

len = get_code_point_name::get_code_point_control_alias(U'\n', buf);
// buf[0..len-1] == "LINE FEED,NEW LINE,END OF LINE"

len = get_code_point_name::get_code_point_alternate_alias(U'\N{BOM}', buf);
// buf[0..len-1] == "BYTE ORDER MARK"

len = get_code_point_name::get_code_point_figment_alias(U'\N{PAD}', buf);
// buf[0..len-1] == "PADDING CHARACTER"

len = get_code_point_name::get_code_point_abbreviation_alias(U'\n', buf);
// buf[0..len-1] == "LF,NL,EOL"
```

## Build

Requires: CMake ≥ 3.18, a C++23-capable compiler (for the generator),
and an internet connection on first configure
(to download `UnicodeData.txt` and `NameAliases.txt`).

```sh
mkdir build && cd build
cmake ..
cmake --build .
ctest
```

The header is regenerated from `UnicodeData.txt` (Unicode 17.0)
by the `namesgen` tool and written to `build/include/get_code_point_name.hpp`.
