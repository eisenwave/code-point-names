# cedilla — Unicode 14 code-point name lookup

Single-header C++ library (`include/cedilla/cp_to_name.hpp`) that maps a Unicode
code point to its official name as defined in Unicode 14.0.

## API

```cpp
#include <cedilla/cp_to_name.hpp>

// Returns a lazy range/view; call .to_string() to materialise.
std::string name = uni::cp_name(U'A').to_string();  // "LATIN CAPITAL LETTER A"
std::string name = uni::cp_name(0x1F600).to_string(); // "GRINNING FACE"
```

Characters whose names are algorithmically derived (Hangul syllables, CJK
Unified Ideographs, Tangut, etc.) are not stored in the table; `cp_name()`
returns an empty string for those.

## Build

Requires: CMake ≥ 3.18, a C++23-capable compiler (for the generator), and an
internet connection on first configure (to download `UnicodeData.txt`).

```sh
mkdir build && cd build
cmake ..
cmake --build .
ctest
```

The header is regenerated from `ucd/14.0/UnicodeData.txt` by the `namesgen`
tool and written to `include/cedilla/cp_to_name.hpp`.
