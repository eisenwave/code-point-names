// Standalone correctness test for get_code_point_name::get_code_point_name().
// Usage: test_names <path/to/UnicodeData.txt>
//
// For every line in UnicodeData.txt whose name field is a real name (i.e. does
// not start with '<'), we check that get_code_point_name::get_code_point_name(cp)
// returns that exact name.
// Additional spot-checks cover algorithmically-named blocks that only appear
// as range markers in UnicodeData.txt (Hangul, CJK Unified, Tangut, etc.).

#include <get_code_point_name.hpp>

#include <cassert>
#include <charconv>
#include <fstream>
#include <iostream>
#include <string>

enum struct line_result {
    skipped = 0,
    success = 1,
    failure = -1,
};

[[nodiscard]]
line_result test_line(const std::string_view line) {
    if (line.empty()) {
        return line_result::skipped;
    }

    // Field 0: codepoint (hex)
    const auto semi_index_1 = line.find(';');
    if (semi_index_1 == std::string::npos) {
        return line_result::skipped;
    }

    // Field 1: character name
    auto semi_index_2 = line.find(';', semi_index_1 + 1);
    if (semi_index_2 == std::string::npos)
        semi_index_2 = line.size();

    const std::string_view name(line.data() + semi_index_1 + 1, semi_index_2 - semi_index_1 - 1);

    uint32_t code = 0;
    const auto [p, ec] = std::from_chars(line.data(), line.data() + semi_index_1, code, 16);
    if (ec != std::errc{}) {
        return line_result::skipped;
    }

    char got_buf[get_code_point_name::max_length];
    const std::size_t got_len = get_code_point_name::get_code_point_name(char32_t(code), got_buf);
    const std::string_view got(got_buf, got_len);

    // Range markers (<... , First>/<... , Last>) represent algorithmically-
    // named blocks and are validated in dedicated spot checks below.
    const bool has_angle_bracket_prefix = name.starts_with('<');
    const bool is_range_marker =
        has_angle_bracket_prefix && (name.ends_with(", First>") || name.ends_with(", Last>"));
    const bool is_nameless_entry = name.empty() || (has_angle_bracket_prefix && !is_range_marker);

    // Names starting with '<' are range markers or formal-name-less entries.
    if (name.empty() || has_angle_bracket_prefix) {
        if (is_nameless_entry && !got.empty()) {
            std::cout << "UNEXPECTED-NAME U+" << std::string(line.data(), semi_index_1)
                      << "  expected empty  got='" << got << "'\n";
            return line_result::failure;
        }
        return line_result::success;
    }
    if (got != name) {
        std::cout << "FAIL U+" << std::string(line.data(), semi_index_1) << "  expected='" << name
                  << "'  got='" << got << "'\n";
        return line_result::failure;
    }
    return line_result::success;
}

void test_data_file(const char *const path, int &checked, int &failed) {
    std::ifstream f(path);
    if (!f) {
        std::cerr << "Error: cannot open " << path << '\n';
        std::exit(1);
    }

    std::string line;

    while (std::getline(f, line)) {
        switch (test_line(line)) {
        case line_result::skipped: {
            continue;
        }
        case line_result::success: {
            ++checked;
            continue;
        }
        case line_result::failure: {
            ++checked;
            ++failed;
            continue;
        }
        }
    }
}

void test_algorithmic(int &checked, int &failed) {
    // Spot-check algorithmically-named blocks that appear only as range markers
    // in UnicodeData.txt and are therefore not covered by the loop above.
    struct spot {
        char32_t cp;
        const char *expected;
    };

    static constexpr spot spots[] = {
        {0xAC00, "HANGUL SYLLABLE GA"},
        {0xAC01, "HANGUL SYLLABLE GAG"},
        {0xD7A3, "HANGUL SYLLABLE HIH"},
        {0x4E00, "CJK UNIFIED IDEOGRAPH-4E00"},
        {0x9FFF, "CJK UNIFIED IDEOGRAPH-9FFF"},
        {0x3400, "CJK UNIFIED IDEOGRAPH-3400"},
        {0x17000, "TANGUT IDEOGRAPH-17000"},
        {0x187FF, "TANGUT IDEOGRAPH-187FF"},
        // Tangut Ideograph Supplement
        {0x18D00, "TANGUT IDEOGRAPH-18D00"},
        {0x18D1E, "TANGUT IDEOGRAPH-18D1E"},
        // CJK Extension H (Unicode 15)
        {0x31350, "CJK UNIFIED IDEOGRAPH-31350"},
        {0x323AF, "CJK UNIFIED IDEOGRAPH-323AF"},
        // CJK Extension I (Unicode 15.1)
        {0x2EBF0, "CJK UNIFIED IDEOGRAPH-2EBF0"},
        {0x2EE5D, "CJK UNIFIED IDEOGRAPH-2EE5D"},
        // CJK Extension J (Unicode 17)
        {0x323B0, "CJK UNIFIED IDEOGRAPH-323B0"},
        {0x33479, "CJK UNIFIED IDEOGRAPH-33479"},
    };

    for (const auto &s : spots) {
        char got_buf[get_code_point_name::max_length];
        const std::size_t got_len = get_code_point_name::get_code_point_name(s.cp, got_buf);
        const std::string_view got(got_buf, got_len);
        if (got != s.expected) {
            std::cout << "SPOT-FAIL U+" << std::hex << uint32_t(s.cp) << "  expected='"
                      << s.expected << "'  got='" << got << "'\n";
            ++failed;
        }
        ++checked;
    }
}

int main(const int argc, const char *const *const argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path/to/UnicodeData.txt>\n";
        return 1;
    }

    int checked = 0;
    int failed = 0;

    test_data_file(argv[1], checked, failed);
    test_algorithmic(checked, failed);

    std::cout << "Checked: " << checked << "\nFailed: " << failed << '\n';
    assert(failed == 0);
    return failed == 0 ? 0 : 1;
}
