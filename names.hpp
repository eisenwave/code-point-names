#ifndef GET_CODE_POINT_NAME_HPP
#define GET_CODE_POINT_NAME_HPP

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace get_code_point_name {
namespace details {

struct name_table_range {
    const unsigned long long *begin;
    const unsigned long long *end;
};

struct alias_entry {
    std::uint32_t cp;
    std::string_view alias;
};

// These forward declarations are here so we can view this file without getting linter errors.
extern const char name_dict[];                                               // NOLINT
constexpr name_table_range get_table_index(std::size_t);                     // NOLINT
extern const unsigned long long name_indexes[];                              // NOLINT
constexpr std::string_view get_name_segment(std::size_t b, std::size_t idx); // NOLINT
// Forward declarations for auto-generated alias tables (linter only)
extern const alias_entry alias_correction_table[];      // NOLINT
extern const std::size_t alias_correction_table_size;   // NOLINT
extern const alias_entry alias_control_table[];         // NOLINT
extern const std::size_t alias_control_table_size;      // NOLINT
extern const alias_entry alias_alternate_table[];       // NOLINT
extern const std::size_t alias_alternate_table_size;    // NOLINT
extern const alias_entry alias_figment_table[];         // NOLINT
extern const std::size_t alias_figment_table_size;      // NOLINT
extern const alias_entry alias_abbreviation_table[];    // NOLINT
extern const std::size_t alias_abbreviation_table_size; // NOLINT

// AUTO-GENERATED CODE HERE

[[nodiscard]] constexpr std::string_view
find_alias(const alias_entry *const table, const std::size_t n, const char32_t cp) noexcept {
    std::size_t lo = 0, hi = n;
    while (lo < hi) {
        const std::size_t mid = lo + (hi - lo) / 2;
        if (table[mid].cp < std::uint32_t(cp)) {
            lo = mid + 1;
        } else if (table[mid].cp > std::uint32_t(cp)) {
            hi = mid;
        } else {
            return table[mid].alias;
        }
    }
    return {};
}

[[nodiscard]] constexpr const unsigned long long *upper_bound(const unsigned long long *first,
                                                              const unsigned long long *const last,
                                                              const char32_t value) {
    const unsigned long long *it = first;
    auto count = last - first;
    auto step = count / 2;

    while (count > 0) {
        it = first;
        step = count / 2;
        it += step;
        if (value >= std::uint32_t(*it >> 32)) {
            first = ++it;
            count -= step + 1;
        } else {
            count = step;
        }
    }
    return first;
}

struct name_view {
    constexpr name_view(char32_t c) : c(c) {}

    struct sentinel {};
    struct iterator {

        using value_type = char;
        using iterator_category = std::forward_iterator_tag;

        constexpr iterator(const char32_t c) : m_c(c), m_block(-1) { get_next_segment(); }
        constexpr char32_t operator*() const { return char32_t(m_str[m_str_pos]); }

        constexpr iterator &operator++(int) {
            m_str_pos++;
            if (m_str_pos >= m_str.size()) {
                get_next_segment();
            }
            return *this;
        }
        constexpr iterator operator++() {
            auto c = *this;
            m_str_pos++;
            if (m_str_pos >= m_str.size()) {
                get_next_segment();
            }
            return c;
        }
        constexpr bool operator==(sentinel) const { return m_c == 0 || m_block == -2; };
        constexpr bool operator!=(sentinel) const { return m_c != 0 && m_block != -2; };
        constexpr bool operator==(iterator it) const {
            return m_c && it.m_c && m_block == it.m_block && m_str == it.m_str &&
                   m_str_pos == it.m_str_pos && m_chunk == it.m_chunk &&
                   m_chunk_pos == it.m_chunk_pos;
        };
        constexpr bool operator!=(iterator it) const { return !(*this == it); };

      private:
        constexpr void get_next_segment() {
            if (m_chunk_pos >= 3 || m_block == -1) {
                const name_table_range range = get_table_index(std::size_t(++m_block));
                if (range.begin == nullptr) {
                    m_block = -2;
                    return;
                }

                const auto end = range.end;
                auto it = upper_bound(range.begin, end, m_c);
                if (it == end || it == range.begin) {
                    m_block = -2;
                    return;
                }
                it--;
                auto start = (*it) & 0x00000000FFFF'FFFF;
                if (start == 0xFFFFFFFF) {
                    m_block = -2;
                    return;
                }
                auto offset = m_c - char32_t((*it) >> 32);
                m_chunk = name_indexes[start + offset];
                m_chunk_pos = -1;
            }
            m_chunk_pos++;
            std::uint16_t data = (m_chunk >> (48 - (16 * m_chunk_pos))) & 0x000000000000ffff;
            if (data == 0x0000) {
                m_block = -2;
                return;
            }
            std::uint8_t dict = data >> 8;
            std::uint8_t entry = data & 0x00ff;
            m_str = get_name_segment(dict, entry);
            m_str_pos = 0;
        }

        char32_t m_c;
        std::int8_t m_block = -2;
        std::string_view m_str;
        std::size_t m_str_pos = 0;
        std::uint64_t m_chunk = 0xFFFFFFFFFFFFFFFF;
        std::int8_t m_chunk_pos = -1;
    };

    [[nodiscard]] constexpr iterator begin() const { return iterator{c}; }
    [[nodiscard]] constexpr sentinel end() const { return {}; }

    void write_to(char *out, std::size_t &len) const {
        for (const char c : *this)
            out[len++] = c;
    }

  private:
    char32_t c;
};

// Hangul syllable jamo components (Unicode Standard § 3.12)
constexpr const char *hangul_l[] = {"G",  "GG", "N", "D",  "DD", "R", "M", "B", "BB", "S",
                                    "SS", "",   "J", "JJ", "C",  "K", "T", "P", "H"};
constexpr const char *hangul_v[] = {"A",   "AE", "YA", "YAE", "EO", "E",  "YEO",
                                    "YE",  "O",  "WA", "WAE", "OE", "YO", "U",
                                    "WEO", "WE", "WI", "YU",  "EU", "YI", "I"};
constexpr const char *hangul_t[] = {"",   "G",  "GG", "GS", "N",  "NJ", "NH", "D", "L",  "LG",
                                    "LM", "LB", "LS", "LT", "LP", "LH", "M",  "B", "BS", "S",
                                    "SS", "NG", "J",  "C",  "K",  "T",  "P",  "H"};

struct hex_range {
    const char *prefix;
    // Minimum code point (inclusive).
    std::uint32_t lo;
    // Maximum code point (inclusive).
    std::uint32_t hi;
};

struct decimal_range {
    const char *prefix;
    // Minimum code point (inclusive).
    std::uint32_t lo;
    // Maximum code point (inclusive).
    std::uint32_t hi;
    // The decimal value of the first code point in the range.
    std::uint32_t base;
    // Minimum amount of digits (for zero padding).
    int width;
};

// All algorithmically named blocks in Unicode 17 (prefix + uppercase hex, min 4 digits).
// clang-format off
inline constexpr hex_range hex_ranges[] = {
    {"CJK UNIFIED IDEOGRAPH-",          0x3400,   0x4DBF},  // Extension A
    {"CJK UNIFIED IDEOGRAPH-",          0x4E00,   0x9FFF},  // CJK Unified Ideographs
    {"CJK UNIFIED IDEOGRAPH-",         0x20000,  0x2A6DF},  // Extension B
    {"CJK UNIFIED IDEOGRAPH-",         0x2A700,  0x2B73F},  // Extension C
    {"CJK UNIFIED IDEOGRAPH-",         0x2B740,  0x2B81D},  // Extension D
    {"CJK UNIFIED IDEOGRAPH-",         0x2B820,  0x2CEAD},  // Extension E
    {"CJK UNIFIED IDEOGRAPH-",         0x2CEB0,  0x2EBE0},  // Extension F
    {"CJK UNIFIED IDEOGRAPH-",         0x2EBF0,  0x2EE5D},  // Extension I (Unicode 15.1)
    {"CJK UNIFIED IDEOGRAPH-",         0x30000,  0x3134A},  // Extension G
    {"CJK UNIFIED IDEOGRAPH-",         0x31350,  0x323AF},  // Extension H (Unicode 15)
    {"CJK UNIFIED IDEOGRAPH-",         0x323B0,  0x33479},  // Extension J (Unicode 17)
    {"TANGUT IDEOGRAPH-",              0x17000,  0x187FF},  // Tangut Ideograph
    {"TANGUT IDEOGRAPH-",              0x18D00,  0x18D1E},  // Supplement (Unicode 13)
    {"NUSHU CHARACTER-",               0x1B170,  0x1B2FB},  // Nushu Character
    {"KHITAN SMALL SCRIPT CHARACTER-", 0x18B00,  0x18CD5},  // Khitan Small Script Character
    {"EGYPTIAN HIEROGLYPH-",           0x13460,  0x143FA},  // Extended-A (Unicode 16)
    {"CJK COMPATIBILITY IDEOGRAPH-",    0xF900,   0xFA6D},  // CJK Compatibility Ideograph
    {"CJK COMPATIBILITY IDEOGRAPH-",    0xFA70,   0xFAD9},  // (Continued)
    {"CJK COMPATIBILITY IDEOGRAPH-",   0x2F800,  0x2FA1D},  // Supplement
};

// Algorithmically named blocks whose suffix is a decimal number.
inline constexpr decimal_range decimal_ranges[] = {
    {"TANGUT COMPONENT-",   0x18800, 0x18AFF, 0x18800, 3}, // 001-768
    {"TANGUT COMPONENT-",   0x18D80, 0x18DF2, 0x18A80, 3}, // 769-883 (Unicode 17)
    {"VARIATION SELECTOR-",  0xFE00,  0xFE0F,  0xFE00, 1}, // 1-16
    {"VARIATION SELECTOR-", 0xE0100, 0xE01EF, 0xE0100 - 16, 1}, // 17-256
};
// clang-format on

[[nodiscard]] inline std::size_t format_decimal_zero_padded(const std::uint32_t n, const int width,
                                                            char *const out) {
    char tmp[10];
    int tmp_length = 0;
    std::uint32_t v = n;
    do {
        tmp[tmp_length++] = char('0' + v % 10);
        v /= 10;
    } while (v);
    while (tmp_length < width) {
        tmp[tmp_length++] = '0';
    }
    std::size_t length = 0;
    for (int i = tmp_length - 1; i >= 0; --i) {
        out[length++] = tmp[i];
    }
    return length;
}

[[nodiscard]] inline std::size_t format_hex(const std::uint32_t code_point, char *const out) {
    constexpr const char *hex = "0123456789ABCDEF";
    char tmp[6];
    int n = 0;
    std::uint32_t v = code_point;
    do {
        tmp[n++] = hex[v & 0xF];
        v >>= 4;
    } while (v);
    while (n < 4) {
        tmp[n++] = '0';
    }
    std::size_t length = 0;
    for (int i = n - 1; i >= 0; --i) {
        out[length++] = tmp[i];
    }
    return length;
}

} // namespace details

// Maximum number of characters in any Unicode code point name
// (including algorithmically derived names). Size output buffers accordingly.
inline constexpr std::size_t max_length = 96;

// If `cp` has a code point name,
// appends that name to `out`,
// and return the name length.
// Otherwise, `out` is unmodified, and `0` is returned.
[[nodiscard]] inline std::size_t get_code_point_name(const char32_t cp, char *const out) noexcept {
    std::size_t length = 0;
    // Hangul syllables: algorithmic decomposition (Unicode § 3.12)
    if (cp >= char32_t(0xAC00) && cp <= char32_t(0xD7A3)) {
        constexpr std::uint32_t TCount = 28u, NCount = 21u * 28u;
        const std::uint32_t si = std::uint32_t(cp) - 0xAC00u;
        for (const char *p = "HANGUL SYLLABLE "; *p; ++p) {
            out[length++] = *p;
        }
        for (const char *p = details::hangul_l[si / NCount]; *p; ++p) {
            out[length++] = *p;
        }
        for (const char *p = details::hangul_v[(si % NCount) / TCount]; *p; ++p) {
            out[length++] = *p;
        }
        for (const char *p = details::hangul_t[si % TCount]; *p; ++p) {
            out[length++] = *p;
        }
        return length;
    }

    // Algorithmically-named blocks: prefix + uppercase hex codepoint
    for (const details::hex_range &r : details::hex_ranges) {
        if (std::uint32_t(cp) >= r.lo && std::uint32_t(cp) <= r.hi) {
            for (const char *p = r.prefix; *p; ++p) {
                out[length++] = *p;
            }
            length += details::format_hex(std::uint32_t(cp), out + length);
            return length;
        }
    }

    // Algorithmically-named blocks: prefix + decimal number
    for (const details::decimal_range &r : details::decimal_ranges) {
        if (std::uint32_t(cp) >= r.lo && std::uint32_t(cp) <= r.hi) {
            for (const char *p = r.prefix; *p; ++p) {
                out[length++] = *p;
            }
            length += details::format_decimal_zero_padded(std::uint32_t(cp) - r.base + 1, r.width,
                                                          out + length);
            return length;
        }
    }

    // Compressed dictionary lookup
    details::name_view(cp).write_to(out, length);
    return length;
}

[[nodiscard]] inline std::size_t get_code_point_correction_alias(const char32_t cp,
                                                                 char *const out) noexcept {
    const std::string_view alias = details::find_alias(details::alias_correction_table,
                                                       details::alias_correction_table_size, cp);
    if (alias.empty()) {
        return 0;
    }
    for (std::size_t i = 0; i < alias.size(); ++i) {
        out[i] = alias[i];
    }
    return alias.size();
}

[[nodiscard]] inline std::size_t get_code_point_control_alias(const char32_t cp,
                                                              char *const out) noexcept {
    const std::string_view alias =
        details::find_alias(details::alias_control_table, details::alias_control_table_size, cp);
    if (alias.empty()) {
        return 0;
    }
    for (std::size_t i = 0; i < alias.size(); ++i) {
        out[i] = alias[i];
    }
    return alias.size();
}

[[nodiscard]] inline std::size_t get_code_point_alternate_alias(const char32_t cp,
                                                                char *const out) noexcept {
    const std::string_view alias = details::find_alias(details::alias_alternate_table,
                                                       details::alias_alternate_table_size, cp);
    if (alias.empty()) {
        return 0;
    }
    for (std::size_t i = 0; i < alias.size(); ++i) {
        out[i] = alias[i];
    }
    return alias.size();
}

[[nodiscard]] inline std::size_t get_code_point_figment_alias(const char32_t cp,
                                                              char *const out) noexcept {
    const std::string_view alias =
        details::find_alias(details::alias_figment_table, details::alias_figment_table_size, cp);
    if (alias.empty()) {
        return 0;
    }
    for (std::size_t i = 0; i < alias.size(); ++i) {
        out[i] = alias[i];
    }
    return alias.size();
}

[[nodiscard]] inline std::size_t get_code_point_abbreviation_alias(const char32_t cp,
                                                                   char *const out) noexcept {
    // VS1–VS16: U+FE00–U+FE0F
    if (cp >= 0xFE00u && cp <= 0xFE0Fu) {
        const std::uint32_t n = std::uint32_t(cp) - 0xFE00u + 1u;
        out[0] = 'V';
        out[1] = 'S';
        return 2u + details::format_decimal_zero_padded(n, 1, out + 2);
    }
    // VS17–VS256: U+E0100–U+E01EF
    if (cp >= 0xE0100u && cp <= 0xE01EFu) {
        const std::uint32_t n = std::uint32_t(cp) - 0xE0100u + 17u;
        out[0] = 'V';
        out[1] = 'S';
        return 2u + details::format_decimal_zero_padded(n, 1, out + 2);
    }
    const std::string_view alias = details::find_alias(details::alias_abbreviation_table,
                                                       details::alias_abbreviation_table_size, cp);
    if (alias.empty()) {
        return 0;
    }
    for (std::size_t i = 0; i < alias.size(); ++i) {
        out[i] = alias[i];
    }
    return alias.size();
}

} // namespace get_code_point_name

#endif // GET_CODE_POINT_NAME_HPP
