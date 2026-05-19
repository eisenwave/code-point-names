#include <algorithm>
#include <cassert>
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

// Returns true for code points whose names are algorithmically derived and
// must not be stored in the compressed dictionary.  These ranges appear as
// individual entries in UnicodeData.txt but are handled at runtime instead.
[[nodiscard]] static bool is_algorithmically_named(const std::uint32_t cp) {
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
[[nodiscard]] std::unordered_map<char32_t, std::string> load_data(const std::string &path) {
    std::ifstream f(path);
    if (!f) {
        fprintf(stderr, "Error: cannot open %s\n", path.c_str());
        return {};
    }
    std::unordered_map<char32_t, std::string> characters;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) {
            continue;
        }
        const auto semi1 = line.find(';');
        if (semi1 == std::string::npos) {
            continue;
        }
        auto semi2 = line.find(';', semi1 + 1);
        if (semi2 == std::string::npos) {
            semi2 = line.size();
        }
        const std::string_view name(line.data() + semi1 + 1, semi2 - semi1 - 1);
        if (name.empty() || name[0] == '<') {
            continue;
        }
        std::uint32_t code = 0;
        const auto [p, ec] = std::from_chars(line.data(), line.data() + semi1, code, 16);
        if (ec != std::errc{}) {
            continue;
        }
        if (is_algorithmically_named(code)) {
            continue;
        }
        characters.emplace(char32_t(code), std::string(name));
    }
    return characters;
}

struct substring_entry {
    std::size_t pos;
    std::string_view str;
};

struct character_name {
    char32_t cp;
    std::string_view name;
    std::vector<substring_entry> sub;
    std::size_t total = 0;

    [[nodiscard]] inline bool complete() const { return total == name.size(); }

    [[nodiscard]] bool add_substring(const std::size_t pos, const std::string_view s) {
        for (const auto used : sub) {
            if (pos >= used.pos && pos < used.pos + used.str.size()) {
                return false;
            }
        }
        sub.push_back({pos, s});
        total += s.size();
        std::ranges::sort(sub, {}, &substring_entry::pos);
        return true;
    }

    [[nodiscard]] std::vector<substring_entry> bits() const {
        std::vector<substring_entry> v;
        std::size_t start = 0;
        for (const auto &used : sub) {
            const auto end = used.pos;
            const std::string_view s = name.substr(start, end - start);
            if (!s.empty()) {
                v.push_back({start, s});
            }
            start = used.pos + used.str.size();
        }
        const std::string_view s = name.substr(start);
        if (!s.empty()) {
            v.push_back({start, s});
        }
        return v;
    }
};

[[nodiscard]] std::unordered_set<std::string_view> substrings(const std::span<character_name> r) {
    std::unordered_set<std::string_view> set;
    for (const character_name &c : r) {
        for (const auto &b : c.bits()) {
            for (const auto i : std::ranges::views::iota(std::size_t(1), b.str.size() + 1)) {
                const auto sub = b.str.substr(0, i);
                if (sub.size() == 0)
                    break;
                set.insert(sub);
            }
        }
    }
    return set;
}

template <typename Map, typename K> void increment(Map &map, const K &k) {
    const auto it = map.find(k);
    if (it != map.end()) {
        it->second++;
        return;
    }
    map.emplace(k, 1);
}

struct block {
  public:
    std::size_t elem_size;
    std::unordered_set<std::string_view> data;
    [[nodiscard]] std::size_t size() const { return data.size(); }
};

void print_dict(std::FILE *const f, const std::vector<block> &blocks) {
    struct data {
        std::size_t start;
        std::size_t elem_size;
        std::size_t count;
    };
    std::size_t start = 0;

    int idx = 1;
    std::unordered_map<int, data> table;
    std::fprintf(f, "inline constexpr char name_dict[] = \"");
    for (const auto &b : blocks) {
        for (const auto &str : b.data) {
            for (const auto c : str) {
                fprint(f, "{}", c);
            }
        }
        table[idx++] = data{start, b.elem_size, b.data.size()};
        start += b.elem_size * b.data.size();
    }
    std::fprintf(f, "\";");
    std::fprintf(
        f, "inline constexpr std::string_view get_name_segment(std::size_t b, std::size_t idx) {");
    std::fprintf(f, "switch(b) {");
    for (const auto &[k, v] : table) {
        fprint(f, "case {}:", k);
        fprint(f, "return std::string_view{{name_dict + {0} + idx * {1}, {1} }};", v.start,
               v.elem_size);
    }
    std::fprintf(f, "} return {};}");
}

void print_indexes(
    std::FILE *const f,
    const std::unordered_map<int, std::unordered_map<char32_t, std::uint64_t>> &mapping) {

    struct code_point_entry {
        char32_t cp;
        std::uint64_t index;
    };
    struct jump_segment {
        std::size_t start;
        std::vector<code_point_entry> code_points;
    };
    const auto sorted_jump_table = [&] -> std::vector<jump_segment> {
        std::vector<jump_segment> result;
        std::size_t start = 0;
        std::vector<std::uint64_t> data;

        struct group_entry {
            int id;
            std::unordered_map<char32_t, std::uint64_t> entries;
        };
        std::vector<group_entry> sorted_mapping;
        sorted_mapping.reserve(mapping.size());
        for (const auto &[id, entries] : mapping)
            sorted_mapping.push_back({id, entries});
        std::ranges::sort(sorted_mapping, {}, &group_entry::id);

        for (const auto &g : sorted_mapping) {
            std::vector<code_point_entry> arr;
            arr.reserve(g.entries.size());
            for (const auto &[cp, idx] : g.entries)
                arr.push_back({cp, idx});
            std::ranges::sort(arr, {}, &code_point_entry::cp);
            for (const auto &e : arr)
                data.push_back(e.index);
            result.push_back({start, std::move(arr)});
            start = data.size();
        }

        std::fprintf(f, "inline constexpr unsigned long long name_indexes[] = {");
        for (const auto &elem : data) {
            fprint(f, "{:#018x},", elem);
        }
        std::fprintf(f, "0xFFFF'FFFF'FFFF'FFFF};");
        return result;
    }();

    for (const auto &[index, seg] : std::ranges::views::enumerate(sorted_jump_table)) {
        fprint(f, "inline constexpr unsigned long long name_indexes_{}[] = {{", index);
        bool first = true;
        char32_t prev = 0;
        std::size_t next_start = seg.start;
        for (const auto &c : seg.code_points) {
            if (first || c.cp != prev + 1) {
                fprint(f, "{:#018x},", (std::uint64_t(prev + 1) << 32) | std::uint32_t(0xFFFFFFFF));
                fprint(f, "{:#018x},", (std::uint64_t(c.cp) << 32) | std::uint32_t(next_start));
            }
            first = false;
            prev = c.cp;
            next_start++;
        }
        fprint(f, "{:#018x}}};", (std::uint64_t(prev + 1) << 32) | std::uint32_t(0xFFFFFFFF));
    }

    fprint(f, "inline constexpr name_table_range get_table_index(std::size_t index) {{");
    std::fprintf(f, "switch(index) {");
    for (const auto &[index, seg] : std::ranges::views::enumerate(sorted_jump_table)) {
        fprint(f, "case {}:", index);
        fprint(f,
               "return {{name_indexes_{0}, name_indexes_{0} + "
               "sizeof(name_indexes_{0})/sizeof(unsigned long long)}};",
               index);
    }
    std::fprintf(f, "} return {nullptr, nullptr}; }\n");
}

int main(int argc, const char *const *const argv) {

    const std::unordered_map<char32_t, std::string> data = load_data(argv[1]);
    auto names_view = data | std::ranges::views::transform([](const auto &p) {
                          return character_name{p.first, p.second, {}, 0};
                      });
    std::vector<character_name> names(names_view.begin(), names_view.end());

    std::unordered_map<std::string_view, int> all_used;
    std::unordered_map<std::string_view, int, std::hash<std::string_view>> used_substrings;

    auto end = names.end();

    while (true) {
        end = std::partition(names.begin(), end,
                             [](const character_name &a) { return !a.complete(); });
        const std::span<character_name> incomplete = std::ranges::views::counted(
            names.begin(), std::size_t(std::ranges::distance(names.begin(), end)));

        fprint("Count : {}\n", std::ranges::distance(incomplete));

        if (std::ranges::empty(incomplete)) {
            break;
        }

        const auto subs = [&incomplete] {
            const std::unordered_set<std::string_view> tmp_view = substrings(incomplete);
            std::vector<std::string_view> tmp(tmp_view.begin(), tmp_view.end());
            std::ranges::sort(tmp, [](const std::string_view a, const std::string_view b) {
                return a.size() > b.size();
            });
            return tmp;
        }();

        used_substrings.clear();
        for (const std::string_view s : subs) {
            used_substrings[s] = 0;
        }

        // Compute a list of all possible substrings for each char
        // the value is the distance

        for (const character_name &c : incomplete) {
            const auto bits = c.bits();
            for (const auto &b : bits) {
                for (const auto i : std::ranges::views::iota(std::size_t(0), b.str.size() + 1)) {
                    if (i > 0 && i < 5) {
                        continue;
                    }
                    for (const auto j :
                         std::ranges::views::iota(std::size_t(i), b.str.size() + 1)) {
                        const auto it = used_substrings.find(b.str.substr(i, j));
                        if (it != used_substrings.end()) {
                            it->second++;
                        }
                    }
                }
            }
        }

        fprint("Substrings : {}\n", subs.size());

        struct weighted_sub {
            std::string_view str;
            double weight;
        };
        auto weighted_substrings_view =
            used_substrings |
            std::ranges::views::filter([](const auto &p) { return p.second != 0; }) |
            std::ranges::views::transform([](const auto &p) {
                const double d = p.first.size() < 5 ? 1.0 : double(p.second) * p.first.size();
                return weighted_sub{p.first, d};
            });
        std::vector<weighted_sub> weighted_substrings(weighted_substrings_view.begin(),
                                                      weighted_substrings_view.end());
        fprint("Used Substrings : {}\n", weighted_substrings.size());
        const auto count = std::size_t(1 + 0.01 * double(weighted_substrings.size()));
        fprint("{}", count);
        const auto weighted_substrings_middle =
            std::min(std::ptrdiff_t(count + 1), std::ptrdiff_t(weighted_substrings.size()));
        std::partial_sort(std::begin(weighted_substrings),
                          std::begin(weighted_substrings) + weighted_substrings_middle,
                          std::end(weighted_substrings),
                          [](const auto &a, const auto &b) { return a.weight > b.weight; });
        auto filtered_view =
            weighted_substrings |             //
            std::ranges::views::take(count) | //
            std::ranges::views::transform([](const weighted_sub &p) { return p.str; });
        std::vector<std::string_view> filtered(filtered_view.begin(), filtered_view.end());

        const auto filtered_middle = std::min(std::ptrdiff_t(11), std::ptrdiff_t(filtered.size()));
        std::partial_sort(std::begin(filtered), std::begin(filtered) + filtered_middle,
                          std::end(filtered),
                          [](const auto &a, const auto &b) { return a.size() > b.size(); });

        for (const std::string_view s : filtered | std::ranges::views::take(10)) {
            for (character_name &c : incomplete) {
                const auto &bits = c.bits();
                for (const substring_entry &b : bits) {
                    const auto idx = b.str.find(s);
                    if (idx != std::string::npos && c.add_substring(b.pos + idx, s)) {
                        increment(all_used, s);
                    }
                }
            }
        }
        fprint("------\n");
    }

    const auto blocks_by_size = [&] -> std::vector<block> {
        struct string_count {
            std::string_view str;
            int count;
        };
        const std::vector<string_count> string_sorted_by_count = [&all_used] {
            std::vector<string_count> result;
            result.reserve(all_used.size());
            for (const auto &[str, count] : all_used) {
                result.push_back({str, count});
            }
            std::ranges::sort(result, std::greater<>{}, &string_count::count);
            return result;
        }();

        std::unordered_map<int, block> blocks;
        for (const auto &s : string_sorted_by_count) {
            auto it = blocks.find(s.str.size());
            if (it == blocks.end()) {
                it = blocks.emplace(s.str.size(), block{s.str.size()}).first;
            }
            it->second.data.insert(s.str);
        }

        std::vector<block> blocks_by_size;
        for (const auto &[_, b] : blocks) {
            if (b.data.size() <= 0xFE) {
                blocks_by_size.push_back(b);
                continue;
            }
            for (const auto &c : b.data | std::ranges::views::chunk(0xFE)) {
                auto common_chunk = c | std::ranges::views::common;
                std::vector<std::string_view> chunk_elements(common_chunk.begin(),
                                                             common_chunk.end());
                blocks_by_size.push_back(
                    block{b.elem_size, std::unordered_set<std::string_view>(chunk_elements.begin(),
                                                                            chunk_elements.end())});
            }
        }

        return blocks_by_size;
    }();

    std::size_t dict_size = 0;
    for (const auto &b : blocks_by_size) {
        fprint("--- BLOCK : string size : {}, elements  {} -- total {} ({})\n", b.elem_size,
               b.size(), b.elem_size * b.data.size(), dict_size += (b.elem_size * b.data.size()));
    }
    fprint("Total blocks: {}\n", blocks_by_size.size());

    std::size_t index_bytes = 0;
    std::unordered_map<int, int> lengths;
    for (const auto &c : names) {
        increment(lengths, std::ranges::size(c.sub));
        index_bytes += 2 * std::ranges::size(c.sub);
    }

    {
        struct length_count {
            int length;
            int count;
        };
        std::vector<length_count> sorted_lengths;
        sorted_lengths.reserve(lengths.size());
        for (const auto &[len, count] : lengths)
            sorted_lengths.push_back({len, count});
        std::ranges::sort(sorted_lengths, std::ranges::greater{}, &length_count::count);
        // fprint("DICT : \n{}\n ----", strings);
        fprint("LENGTHS : \n");
        for (const auto &[key, occurrences] : sorted_lengths) {
            fprint("({}, {})", key, occurrences);
        }
        fprint("\n ----");
    }

    fprint("KBytes: {}  ( dict {}  + index : {} )\n", (index_bytes + dict_size) / 1024.0,
           dict_size / 1024.0, index_bytes / 1024.0);

    const auto f = fopen(argv[2], "w");

    print_dict(f, blocks_by_size);

    {
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

        std::unordered_map<int, std::unordered_map<char32_t, std::uint64_t>> mapping;
        for (const auto &c : names) {
            int seq = 0;
            int i = 0;

            std::uint64_t v = 0x00000000000000;
            while (i < c.sub.size()) {
                const auto n = 64 - 16 * (i % 4);
                const auto pos = indexes[c.sub.at(i).str];
                v |= (std::uint64_t(pos.b) << (n - 8)) | (std::uint64_t(pos.i) << (n - 16));
                if (((i % 4) == 3) || i == c.sub.size() - 1) {
                    mapping[seq++].insert({c.cp, v});
                    v = 0;
                }
                i++;
            }
        }
        print_indexes(f, mapping);
    }

    fclose(f);
}

//    fmt::print("{}\n", names.size());
//}
