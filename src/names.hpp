#include <cstdint>
#include <string>

namespace uni {

namespace details {

template <class ForwardIt, class T, class Compare>
constexpr ForwardIt upper_bound(ForwardIt first, ForwardIt last, const T &value, Compare comp) {
    ForwardIt it = first;
    typename std::iterator_traits<ForwardIt>::difference_type count = std::distance(first, last);
    typename std::iterator_traits<ForwardIt>::difference_type step = count / 2;

    while (count > 0) {
        it = first;
        step = count / 2;
        std::advance(it, step);
        if (!comp(value, *it)) {
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

        constexpr iterator(char32_t c) : m_c(c), m_block(-1) { get_next_segment(); }
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
                const auto range = __get_table_index(std::size_t(++m_block));
                if (range.first == nullptr) {
                    m_block = -2;
                    return;
                }

                const auto end = range.second;
                auto it = upper_bound(range.first, end, m_c, [](char32_t cp, std::uint64_t v) {
                    char32_t c = (v >> 32) & 0x00000000FFFFFFFF;
                    return cp < c;
                });
                if (it == end || it == range.first) {
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
                m_chunk = __name_indexes[start + offset];
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
            m_str = __get_name_segment(dict, entry);
            m_str_pos = 0;
        }

        char32_t m_c;
        std::int8_t m_block = -2;
        std::string_view m_str;
        std::size_t m_str_pos = 0;
        std::uint64_t m_chunk = 0xFFFFFFFFFFFFFFFF;
        std::int8_t m_chunk_pos = -1;
    };

    constexpr iterator begin() const { return iterator{c}; }
    constexpr sentinel end() const { return {}; }

    std::string to_string() const {
        std::string s;
        for (auto c : *this)
            s.push_back(c);
        return s;
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

struct generated_range {
    const char *prefix;
    uint32_t lo, hi;
};
struct decimal_range {
    const char *prefix;
    uint32_t lo, hi, base;
    int width;
};

// All algorithmically-named blocks in Unicode 14 (prefix + uppercase hex, min 4 digits).
constexpr generated_range generated_ranges[] = {
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

// Algorithmically-named blocks whose suffix is a decimal number.
// base  = value of the first codepoint in the range
// width = minimum digit count (zero-padded)
constexpr decimal_range decimal_ranges[] = {
    {"TANGUT COMPONENT-", 0x18800u, 0x18AFFu, 0x18800u, 3},        // 001-768
    {"VARIATION SELECTOR-", 0xFE00u, 0xFE0Fu, 0xFE00u, 1},         // 1-16
    {"VARIATION SELECTOR-", 0xE0100u, 0xE01EFu, 0xE0100u - 16, 1}, // 17-256
};

inline std::string format_cp_decimal(uint32_t n, int width) {
    char buf[10];
    int len = 0;
    uint32_t v = n;
    do {
        buf[len++] = char('0' + v % 10);
        v /= 10;
    } while (v);
    while (len < width)
        buf[len++] = '0';
    std::string s(len, '\0');
    for (int i = 0; i < len; ++i)
        s[i] = buf[len - 1 - i];
    return s;
}

inline std::string format_cp_hex(uint32_t cp) {
    constexpr const char *hex = "0123456789ABCDEF";
    char buf[6];
    int n = 0;
    uint32_t v = cp;
    do {
        buf[n++] = hex[v & 0xF];
        v >>= 4;
    } while (v);
    while (n < 4)
        buf[n++] = '0';
    std::string s(n, '\0');
    for (int i = 0; i < n; ++i)
        s[i] = buf[n - 1 - i];
    return s;
}

} // namespace details

inline std::string cp_name(char32_t cp) {
    // Hangul syllables: algorithmic decomposition (Unicode § 3.12)
    if (cp >= char32_t(0xAC00) && cp <= char32_t(0xD7A3)) {
        constexpr uint32_t TCount = 28u, NCount = 21u * 28u;
        uint32_t si = uint32_t(cp) - 0xAC00u;
        return std::string("HANGUL SYLLABLE ") + details::hangul_l[si / NCount] +
               details::hangul_v[(si % NCount) / TCount] + details::hangul_t[si % TCount];
    }
    // Algorithmically-named blocks: prefix + uppercase hex codepoint
    for (const auto &r : details::generated_ranges)
        if (uint32_t(cp) >= r.lo && uint32_t(cp) <= r.hi)
            return std::string(r.prefix) + details::format_cp_hex(uint32_t(cp));
    // Algorithmically-named blocks: prefix + decimal number
    for (const auto &r : details::decimal_ranges)
        if (uint32_t(cp) >= r.lo && uint32_t(cp) <= r.hi)
            return std::string(r.prefix) +
                   details::format_cp_decimal(uint32_t(cp) - r.base + 1, r.width);
    // Compressed dictionary lookup
    return details::name_view(cp).to_string();
}

} // namespace uni
