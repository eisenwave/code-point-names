#ifndef CODE_POINT_TO_NAME_HPP
#define CODE_POINT_TO_NAME_HPP

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace uni {
namespace details {

struct name_table_range {
    const unsigned long long *begin;
    const unsigned long long *end;
};

// These forward declarations are here so we can view this file without getting linter errors.
extern const char name_dict[];                                               // NOLINT
constexpr name_table_range get_table_index(std::size_t);                     // NOLINT
extern const unsigned long long name_indexes[];                              // NOLINT
constexpr std::string_view get_name_segment(std::size_t b, std::size_t idx); // NOLINT

// AUTO-GENERATED CODE HERE

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
        } else
            count = step;
    }
    return first;
}

struct name_view {
    constexpr name_view(char32_t c) : c(c) {};

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

struct algorithmic_range {
    const char *prefix;
    std::uint32_t lo, hi;
};
struct decimal_range {
    const char *prefix;
    std::uint32_t lo, hi, base;
    int width;
};

// All algorithmically named blocks in Unicode 14 (prefix + uppercase hex, min 4 digits).
constexpr algorithmic_range algorithmic_ranges[] = {
    {"CJK UNIFIED IDEOGRAPH-", 0x3400u, 0x4DBFu},
    {"CJK UNIFIED IDEOGRAPH-", 0x4E00u, 0x9FFFu},
    {"CJK UNIFIED IDEOGRAPH-", 0x20000u, 0x2A6DFu},
    {"CJK UNIFIED IDEOGRAPH-", 0x2A700u, 0x2B738u},
    {"CJK UNIFIED IDEOGRAPH-", 0x2B740u, 0x2B81Du},
    {"CJK UNIFIED IDEOGRAPH-", 0x2B820u, 0x2CEA1u},
    {"CJK UNIFIED IDEOGRAPH-", 0x2CEB0u, 0x2EBE0u},
    {"CJK UNIFIED IDEOGRAPH-", 0x30000u, 0x3134Au},
    {"TANGUT IDEOGRAPH-", 0x17000u, 0x187F7u},
    {"NUSHU CHARACTER-", 0x1B170u, 0x1B2FBu},
    {"KHITAN SMALL SCRIPT CHARACTER-", 0x18B00u, 0x18CD5u},
    {"CJK COMPATIBILITY IDEOGRAPH-", 0xF900u, 0xFA6Du},
    {"CJK COMPATIBILITY IDEOGRAPH-", 0xFA70u, 0xFAD9u},
    {"CJK COMPATIBILITY IDEOGRAPH-", 0x2F800u, 0x2FA1Du},
};

// Algorithmically named blocks whose suffix is a decimal number.
// base  = value of the first codepoint in the range
// width = minimum digit count (zero-padded)
constexpr decimal_range decimal_ranges[] = {
    {"TANGUT COMPONENT-", 0x18800u, 0x18AFFu, 0x18800u, 3},        // 001-768
    {"VARIATION SELECTOR-", 0xFE00u, 0xFE0Fu, 0xFE00u, 1},         // 1-16
    {"VARIATION SELECTOR-", 0xE0100u, 0xE01EFu, 0xE0100u - 16, 1}, // 17-256
};

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

// Maximum number of characters in any Unicode 14 code point name
// (including algorithmically derived names). Size output buffers accordingly.
inline constexpr std::size_t cp_name_max_length = 96;

// If `cp` has a code point name,
// appends that name to `out`,
// and return the name length.
// Otherwise, `out` is unmodified, and `0` is returned.
[[nodiscard]] inline std::size_t code_point_name(const char32_t cp, char *const out) {
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
    for (const details::algorithmic_range &r : details::algorithmic_ranges) {
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

} // namespace uni

#endif // CODE_POINT_TO_NAME_HPP
