// Standalone correctness test for uni::cp_name().
// Usage: test_names <path/to/UnicodeData.txt>
//
// For every line in UnicodeData.txt whose name field is a real name (i.e. does
// not start with '<'), we check that uni::cp_name(cp) returns that exact name.
// Additional spot-checks cover algorithmically-named blocks that only appear
// as range markers in UnicodeData.txt (Hangul, CJK Unified, Tangut, etc.).

#include <cedilla/cp_to_name.hpp>

#include <cassert>
#include <charconv>
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path/to/UnicodeData.txt>\n";
        return 1;
    }

    std::ifstream f(argv[1]);
    if(!f) {
        std::cerr << "Error: cannot open " << argv[1] << '\n';
        return 1;
    }

    int checked = 0;
    int failed = 0;
    std::string line;

    while(std::getline(f, line)) {
        if(line.empty())
            continue;

        // Field 0: codepoint (hex)
        auto semi1 = line.find(';');
        if(semi1 == std::string::npos)
            continue;

        // Field 1: character name
        auto semi2 = line.find(';', semi1 + 1);
        if(semi2 == std::string::npos)
            semi2 = line.size();

        std::string_view name(line.data() + semi1 + 1, semi2 - semi1 - 1);

        uint32_t code = 0;
        auto [p, ec] = std::from_chars(line.data(), line.data() + semi1, code, 16);
        if(ec != std::errc{})
            continue;

        const auto got = uni::cp_name(char32_t(code));

        // Range markers (<... , First>/<... , Last>) represent algorithmically-
        // named blocks and are validated in dedicated spot checks below.
        const bool has_angle_bracket_prefix = name.starts_with('<');
        const bool is_range_marker =
            has_angle_bracket_prefix && (name.ends_with(", First>") || name.ends_with(", Last>"));
        const bool is_nameless_entry =
            name.empty() || (has_angle_bracket_prefix && !is_range_marker);

        // Names starting with '<' are range markers or formal-name-less entries.
        if(name.empty() || has_angle_bracket_prefix) {
            if(is_nameless_entry && !got.empty()) {
                std::cout << "UNEXPECTED-NAME U+" << std::string(line.data(), semi1)
                          << "  expected empty  got='" << got << "'\n";
                ++failed;
            }
            ++checked;
            continue;
        }
        if(got != name) {
            std::cout << "FAIL U+" << std::string(line.data(), semi1) << "  expected='" << name
                      << "'  got='" << got << "'\n";
            ++failed;
        }
        ++checked;
    }

    // Spot-check algorithmically-named blocks that appear only as range markers
    // in UnicodeData.txt and are therefore not covered by the loop above.
    struct spot {
        char32_t cp;
        const char* expected;
    };
    constexpr spot spots[] = {
        {0xAC00, "HANGUL SYLLABLE GA"},
        {0xAC01, "HANGUL SYLLABLE GAG"},
        {0xD7A3, "HANGUL SYLLABLE HIH"},
        {0x4E00, "CJK UNIFIED IDEOGRAPH-4E00"},
        {0x9FFF, "CJK UNIFIED IDEOGRAPH-9FFF"},
        {0x3400, "CJK UNIFIED IDEOGRAPH-3400"},
        {0x17000, "TANGUT IDEOGRAPH-17000"},
        {0x187F7, "TANGUT IDEOGRAPH-187F7"},
        // Tangut Components (decimal-suffix, 3-digit zero-padded)
        {0x18800, "TANGUT COMPONENT-001"},
        {0x18AFF, "TANGUT COMPONENT-768"},
        // Variation Selectors (decimal-suffix)
        {0xFE00, "VARIATION SELECTOR-1"},
        {0xFE0F, "VARIATION SELECTOR-16"},
        {0xE0100, "VARIATION SELECTOR-17"},
        {0xE01EF, "VARIATION SELECTOR-256"},
    };
    for(const auto& s : spots) {
        const auto got = uni::cp_name(s.cp);
        if(got != s.expected) {
            std::cout << "SPOT-FAIL U+" << std::hex << uint32_t(s.cp) << "  expected='"
                      << s.expected << "'  got='" << got << "'\n";
            ++failed;
        }
        ++checked;
    }

    std::cout << "Checked: " << checked << "  Failed: " << failed << '\n';
    assert(failed == 0);
    return failed == 0 ? 0 : 1;
}
