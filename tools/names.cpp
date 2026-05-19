#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <format>
#include <fstream>
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// Drop-in for std::print that works with C++20 (std::print requires GCC 14+)
template <class... Args>
static void fprint(std::FILE *f, std::format_string<Args...> fmt, Args &&...args) {
    fputs(std::format(fmt, std::forward<Args>(args)...).c_str(), f);
}
template <class... Args> static void fprint(std::format_string<Args...> fmt, Args &&...args) {
    fputs(std::format(fmt, std::forward<Args>(args)...).c_str(), stdout);
}
#include <cassert>

namespace ranges {
using namespace std::ranges;

// Polyfill for std::ranges::to (available from C++23/GCC 14+).
// Supports pipe syntax: range | ranges::to<Container>()
// and direct syntax:  ranges::to<Container>(range)
namespace _to_detail {
template <class Container> struct full_t {
    template <std::ranges::range R> friend Container operator|(R &&r, full_t) {
        Container c;
        for (auto &&e : r)
            c.push_back(std::move(e));
        return c;
    }
    template <std::ranges::range R> Container operator()(R &&r) const {
        return std::forward<R>(r) | full_t{};
    }
};
template <template <class...> class Container> struct tmpl_t {
    template <std::ranges::range R> friend auto operator|(R &&r, tmpl_t) {
        using T = std::ranges::range_value_t<std::remove_cvref_t<R>>;
        Container<T> c;
        for (auto &&e : r)
            c.push_back(std::move(e));
        return c;
    }
    template <std::ranges::range R> auto operator()(R &&r) const {
        return std::forward<R>(r) | tmpl_t{};
    }
};
} // namespace _to_detail

template <class Container> auto to() { return _to_detail::full_t<Container>{}; }
template <template <class...> class Container> auto to() { return _to_detail::tmpl_t<Container>{}; }
template <class Container, std::ranges::range R> Container to(R &&r) {
    return std::forward<R>(r) | _to_detail::full_t<Container>{};
}
template <template <class...> class Container, std::ranges::range R> auto to(R &&r) {
    return std::forward<R>(r) | _to_detail::tmpl_t<Container>{};
}
} // namespace ranges

// Returns true for code points whose names are algorithmically derived and
// must not be stored in the compressed dictionary.  These ranges appear as
// individual entries in UnicodeData.txt but are handled at runtime instead.
static bool generated(uint32_t cp) {
    return (cp >= 0xAC00u && cp <= 0xD7A3u) ||   // Hangul Syllables
           (cp >= 0xF900u && cp <= 0xFAD9u) ||   // CJK Compatibility Ideographs
           (cp >= 0x2F800u && cp <= 0x2FA1Du) || // CJK Compat Ideographs Supplement
           (cp >= 0x1B170u && cp <= 0x1B2FBu) || // Nushu
           (cp >= 0x18B00u && cp <= 0x18CD5u) || // Khitan Small Script
           (cp >= 0x18800u && cp <= 0x18AFFu) || // Tangut Components
           (cp >= 0xFE00u && cp <= 0xFE0Fu) ||   // Variation Selectors 1-16
           (cp >= 0xE0100u && cp <= 0xE01EFu);   // Variation Selectors 17-256
}

// Parse UnicodeData.txt (semicolon-separated).
// Field 0: code point (hex), Field 1: character name.
// Lines whose name starts with '<' are range markers or controls without formal
// names and are skipped.
std::unordered_map<char32_t, std::string> load_data(const std::string &path) {
    std::ifstream f(path);
    if (!f) {
        fprintf(stderr, "Error: cannot open %s\n", path.c_str());
        return {};
    }
    std::unordered_map<char32_t, std::string> characters;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty())
            continue;
        auto semi1 = line.find(';');
        if (semi1 == std::string::npos)
            continue;
        auto semi2 = line.find(';', semi1 + 1);
        if (semi2 == std::string::npos)
            semi2 = line.size();
        std::string_view name(line.data() + semi1 + 1, semi2 - semi1 - 1);
        if (name.empty() || name[0] == '<')
            continue;
        uint32_t code = 0;
        auto [p, ec] = std::from_chars(line.data(), line.data() + semi1, code, 16);
        if (ec != std::errc{})
            continue;
        if (generated(code))
            continue;
        characters.emplace(char32_t(code), std::string(name));
    }
    return characters;
}

struct character_name {
    char32_t cp;
    std::string_view name;
    std::vector<std::pair<std::size_t, std::string_view>> sub;
    std::size_t total = 0;

    inline bool complete() const { return total == name.size(); }
    bool add_substring(std::size_t pos, std::string_view s) {
        for (const auto used : sub) {
            if (pos >= used.first && pos < used.first + used.second.size())
                return false;
        }
        sub.emplace_back(pos, s);
        total += s.size();
        ranges::sort(sub, std::less<>{}, &std::pair<std::size_t, std::string_view>::first);
        return true;
    }

    std::vector<std::pair<std::size_t, std::string_view>> bits() const {
        std::vector<std::pair<std::size_t, std::string_view>> v;
        std::size_t start = 0;
        for (const auto &used : sub) {
            const auto end = used.first;
            std::string_view s = name.substr(start, end - start);
            if (!s.empty())
                v.emplace_back(start, s);
            start = used.first + used.second.size();
        }
        std::string_view s = name.substr(start);
        if (!s.empty())
            v.emplace_back(start, s);
        return v;
    }
};

template <typename R> auto substrings(R &&r) {
    std::unordered_set<std::string_view, std::hash<std::string_view>> set;
    std::for_each(ranges::begin(r), ranges::end(r), [&](const character_name &c) {
        for (const auto &b : c.bits()) {
            for (auto i : ranges::views::iota(std::size_t(1), b.second.size() + 1)) {
                auto sub = b.second.substr(0, i);
                if (sub.size() == 0)
                    break;
                set.insert(sub);
            }
        }
    });
    return set;
}

template <typename Map, typename K> void increment(Map &map, K &&k) {
    auto it = map.find(k);
    if (it != map.end()) {
        it->second++;
        return;
    }
    map.emplace(k, 1);
}

template <typename Map> auto sorted_by_occurences(Map &map) {
    auto v = map | ranges::to<std::vector<std::pair<typename Map::key_type, int>>>();
    ranges::sort(v, std::greater<>{}, &std::pair<typename Map::key_type, int>::second);
    return v;
}

struct block {
  public:
    std::size_t elem_size;
    std::unordered_set<std::string_view> data;
    std::size_t size() const { return data.size(); }
};

void print_dict(std::FILE *f, const std::vector<block> &blocks) {
    struct data {
        std::size_t start;
        std::size_t elem_size;
        std::size_t count;
    };
    size_t start = 0;

    int idx = 1;
    std::unordered_map<int, data> table;
    fprint(f, "constexpr const char* __name_dict = \"");
    for (const auto &b : blocks) {
        for (const auto &str : b.data) {
            for (auto c : str) {
                fprint(f, "{}", c);
            }
        }
        table[idx++] = data{start, b.elem_size, b.data.size()};
        start += b.elem_size * b.data.size();
    }
    fprint(f, "\";");
    fprint(f, "constexpr std::string_view __get_name_segment(std::size_t b, std::size_t idx) {{");
    fprint(f, "switch(b) {{");
    for (const auto &[k, v] : table) {
        fprint(f, "case {}:", k);
        fprint(f, "return std::string_view{{__name_dict + {0} + idx * {1}, {1} }};", v.start,
               v.elem_size);
    }
    fprint(f, "}} return {{}};}}");
}
void print_indexes(std::FILE *f,
                   const std::unordered_map<int, std::unordered_map<char32_t, uint64_t>> &mapping) {

    std::vector<std::pair<std::size_t, std::vector<std::pair<char32_t, uint64_t>>>>
        sorted_jump_table;
    std::size_t start = 0;
    std::vector<uint64_t> data;

    auto sorted_mapping =
        mapping | ranges::to<std::vector<std::pair<int, std::unordered_map<char32_t, uint64_t>>>>();
    ranges::sort(sorted_mapping, {},
                 &std::pair<int, std::unordered_map<char32_t, uint64_t>>::first);

    for (auto &[k, v] : sorted_mapping) {
        auto arr = v | ranges::to<std::vector<std::pair<char32_t, uint64_t>>>();
        ranges::sort(arr, {}, &std::pair<char32_t, uint64_t>::first);
        for (auto &d : arr) {
            data.push_back(d.second);
        }
        sorted_jump_table.push_back({start, arr});
        start = data.size();
    }

    fprint(f, "constexpr uint64_t __name_indexes[] = {{");
    for (auto &elem : data) {
        fprint(f, "{:#018x},", elem);
    }
    fprint(f, "0xFFFF'FFFF'FFFF'FFFF}};");

    for (const auto &[index, data] : ranges::views::enumerate(sorted_jump_table)) {
        fprint(f, "constexpr uint64_t __name_indexes_{}[] = {{", index);
        bool first = true;
        char32_t prev = 0;
        size_t next_start = data.first;
        for (auto &c : data.second) {
            if (first || c.first != prev + 1) {
                fprint(f, "{:#018x},", (uint64_t(prev + 1) << 32) | uint32_t(0xFFFFFFFF));
                fprint(f, "{:#018x},", (uint64_t(c.first) << 32) | uint32_t(next_start));
            }
            first = false;
            prev = c.first;
            next_start++;
        }
        fprint(f, "{:#018x}}};", (uint64_t(prev + 1) << 32) | uint32_t(0xFFFFFFFF));
    }

    fprint(f, "constexpr std::pair<const uint64_t* const, const uint64_t* const> "
              "__get_table_index(std::size_t index) {{");
    fprint(f, "switch(index) {{");
    for (const auto &[index, data] : ranges::views::enumerate(sorted_jump_table)) {
        fprint(f, "case {}:", index);
        fprint(f,
               "return {{  __name_indexes_{0},  __name_indexes_{0} +  "
               "sizeof(__name_indexes_{0})/sizeof(uint64_t) }};",
               index);
    }
    fprint(f, "}} return {{nullptr, nullptr}}; }}\n");
}

int main(int argc, char **argv) {

    const auto data = load_data(argv[1]);
    auto names = data | ranges::views::transform([](const auto &p) {
                     return character_name{p.first, p.second, {}, 0};
                 }) |
                 ranges::to<std::vector>();

    std::unordered_map<std::string_view, int> all_used;
    std::unordered_map<std::string_view, int, std::hash<std::string_view>> used_substrings;

    auto end = names.end();

    while (true) {
        end = std::partition(names.begin(), end, [](const auto &a) { return !a.complete(); });
        auto incomplete = ranges::views::counted(names.begin(),
                                                 std::size_t(ranges::distance(names.begin(), end)));

        fprint("Count : {}\n", ranges::distance(incomplete));

        if (ranges::empty(incomplete)) {
            break;
        }

        const auto subs = [&incomplete] {
            auto tmp = substrings(incomplete) | ranges::to<std::vector>();
            std::sort(tmp.begin(), tmp.end(),
                      [](const auto &a, const auto &b) { return a.size() > b.size(); });
            return tmp;
        }();

        used_substrings.clear();
        for (const auto &s : subs) {
            used_substrings[s] = 0;
        }

        // Compute a list of all possible substrings for each char
        // the value is the distance

        std::for_each(
            incomplete.begin(), incomplete.end(), [&used_substrings](const character_name &c) {
                const auto bits = c.bits();
                std::for_each(
                    ranges::begin(bits), ranges::end(bits), [&used_substrings, &c](const auto &b) {
                        for (auto i : ranges::views::iota(std::size_t(0), b.second.size() + 1)) {
                            if (i > 0 && i < 5)
                                continue;
                            for (auto j :
                                 ranges::views::iota(std::size_t(i), b.second.size() + 1)) {
                                auto it = used_substrings.find(b.second.substr(i, j));
                                if (it != used_substrings.end()) {
                                    it->second++;
                                }
                            }
                        }
                    });
            });

        fprint("Substrings : {}\n", subs.size());

        std::vector<std::pair<std::string_view, double>> weighted_substrings =
            used_substrings | ranges::views::filter([](const auto &p) { return p.second != 0; }) |
            ranges::views::transform([](const auto &p) {
                const double d = p.first.size() < 5 ? 1.0 : double(p.second) * p.first.size();
                return std::pair<std::string_view, double>{p.first, d};
            }) |
            ranges::to<std::vector>();
        fprint("Used Substrings : {}\n", weighted_substrings.size());
        const auto count = std::size_t(1 + 0.01 * double(weighted_substrings.size()));
        fprint("{}", count);
        auto weighted_substrings_middle =
            std::min(std::ptrdiff_t(count + 1), std::ptrdiff_t(weighted_substrings.size()));
        std::partial_sort(std::begin(weighted_substrings),
                          std::begin(weighted_substrings) + weighted_substrings_middle,
                          std::end(weighted_substrings),
                          [](const auto &a, const auto &b) { return a.second > b.second; });
        auto filtered = weighted_substrings | ranges::views::take(count) |
                        ranges::views::transform([](const auto &p) { return p.first; }) |
                        ranges::to<std::vector<std::string_view>>();

        auto filtered_middle = std::min(std::ptrdiff_t(11), std::ptrdiff_t(filtered.size()));
        std::partial_sort(std::begin(filtered), std::begin(filtered) + filtered_middle,
                          std::end(filtered),
                          [](const auto &a, const auto &b) { return a.size() > b.size(); });

        for (const auto &s : filtered | ranges::views::take(10)) {
            std::for_each(ranges::begin(incomplete), ranges::end(incomplete), [&](auto &c) {
                const auto &bits = c.bits();
                for (const auto &b : bits) {
                    const auto idx = b.second.find(s);
                    if (idx != std::string::npos) {
                        if (c.add_sub(b.first + idx, s))
                            increment(all_used, s);
                    }
                }
            });
        }
        fprint("------\n");
    }
    auto strings = sorted_by_occurences(all_used);

    std::unordered_map<int, block> blocks;
    for (const auto &s : strings) {
        auto it = blocks.find(s.first.size());
        if (it == blocks.end()) {
            it = blocks.emplace(s.first.size(), block{s.first.size()}).first;
        }
        it->second.data.insert((s.first));
    }
    std::vector<block> blocks_by_size;
    for (const auto &[_, b] : blocks) {
        if (b.data.size() <= 0xFE)
            blocks_by_size.push_back(b);
        else {
            for (const auto &c : b.data | ranges::views::chunk(0xFE)) {
                std::vector k = ranges::to<std::vector>(c);
                blocks_by_size.push_back(
                    block{b.elem_size,

                          std::unordered_set<std::string_view>(k.begin(), k.end())});
            }
        }
    }

    std::size_t dict_size = 0;
    for (const auto &b : blocks_by_size) {
        fprint("--- BLOCK : string size : {}, elements  {} -- total {} ({})\n", b.elem_size,
               b.size(), b.elem_size * b.data.size(), dict_size += (b.elem_size * b.data.size()));
    }
    fprint("Total blocks: {}\n", blocks_by_size.size());

    std::size_t index_bytes = 0;
    std::unordered_map<int, int> lengths;
    for (const auto &c : names) {
        increment(lengths, ranges::size(c.sub));
        index_bytes += 2 * ranges::size(c.sub);
    }

    auto sorted_lengths = sorted_by_occurences(lengths);
    // fprint("DICT : \n{}\n ----", strings);
    fprint("LENGTHS : \n");
    for (const auto &[key, occurrences] : sorted_lengths) {
        fprint("({}, {})", key, occurrences);
    }
    fprint("\n ----");

    fprint("KBytes: {}  ( dict {}  + index : {} )\n", (index_bytes + dict_size) / 1024.0,
           dict_size / 1024.0, index_bytes / 1024.0);

    auto f = fopen(argv[2], "w");
    fprint(f, "#pragma once\n#include <string_view>\n#include <array>\n#include <cstdint>\n\n");

    print_dict(f, blocks_by_size);

    struct pos {
        int b;
        int i;
    };

    std::unordered_map<std::string_view, pos> indexes;
    int bi = 1;
    for (const auto &b : blocks_by_size) {
        int idx = 0;
        for (const auto &str : b.data) {
            indexes[str] = pos{bi, idx++};
        }
        bi++;
    }

    std::unordered_map<int, std::unordered_map<char32_t, uint64_t>> mapping;
    for (const auto &c : names) {
        int seq = 0;
        int i = 0;

        uint64_t v = 0x00000000000000;
        while (i < c.sub.size()) {
            const auto n = 64 - 16 * (i % 4);
            const auto pos = indexes[c.sub.at(i).second];
            v |= (uint64_t(pos.b) << (n - 8)) | (uint64_t(pos.i) << (n - 16));
            if (((i % 4) == 3) || i == c.sub.size() - 1) {
                mapping[seq++].insert({c.cp, v});
                v = 0;
            }
            i++;
        }
    }
    print_indexes(f, mapping);

    fclose(f);
}

//    fmt::print("{}\n", names.size());
//}
