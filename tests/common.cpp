#include "common.h"
#include "pugixml.hpp"
#include <string>
#include <sstream>
#include <array>
#include <charconv>
#include <cassert>

bool generated(char32_t c) {
    using P = std::pair<char32_t, char32_t>;
    const std::array ranges = {
        P{0xAC00, 0xD7A3},   P{0x3400, 0x4DBF},   P{0x4E00, 0x9FFF},
        P{0x20000, 0x2A6DF}, P{0x2A700, 0x2B738}, P{0x2B740, 0x2B81D},
        P{0x2B820, 0x2CEA1}, P{0x2CEB0, 0x2EBE0}, P{0x17000, 0x187EC},
        P{0x1B170, 0x1B2FB}, P{0x0F900, 0xFA6D},  P{0x0FA70, 0xD7F9},
        P{0x2F800, 0x2FA1D}, P{0x30000, 0x3134A}};
    for(auto r : ranges) {
        if(c >= r.first && c <= r.second)
            return true;
    }
    return false;
}

std::string fixup_name(std::string n, std::string_view cp) {
    auto p = n.find("#");
    if(p == std::string::npos)
        return n;
    auto c = n;
    c.replace(p, 1, cp);
    return c;
}

std::unordered_map<char32_t, cp_test_data> load_test_data() {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(UCDXML_FILE);
    assert(result.status == pugi::status_ok);

    std::unordered_map<char32_t, cp_test_data> db;

    pugi::xml_node rep = doc.child("ucd").child("repertoire");
    for(pugi::xml_node cp : rep.children("char")) {
        try {
            uint32_t first;
            uint32_t last;

            if(!cp.attribute("cp").empty())
            {
                first = last = char32_t(std::stoi(cp.attribute("cp").value(), 0, 16));
            }
            else
            {
                first = char32_t(std::stoi(cp.attribute("first-cp").value(), 0, 16));
                last  = char32_t(std::stoi(cp.attribute("last-cp").value(), 0, 16));

            }
            std::string na_template = cp.attribute("na").value();
            bool na_has_hash = na_template.find('#') != std::string::npos;
            std::string cp_attr = cp.attribute("cp").value();
            auto age = uni::detail::age_from_string(cp.attribute("age").value());
            auto category = uni::detail::category_from_string(cp.attribute("gc").value());
            auto block = uni::detail::block_from_string(cp.attribute("blk").value());
            auto script = uni::detail::script_from_string(cp.attribute("sc").value());

            std::istringstream iss(cp.attribute("scx").value());
            std::vector<std::string> results((std::istream_iterator<std::string>(iss)),
                                             std::istream_iterator<std::string>());
            std::vector<uni::script> exts;
            for(auto&& x : results) {
                auto n = uni::detail::script_from_string(x);
                if(n == uni::script::unknown)
                    continue;
                exts.push_back(uni::detail::script_from_string(x));
            }

            std::string nv = cp.attribute("nv").value();
            int64_t d = 0;
            int64_t n = 0;
            if(nv != "NaN") {
                d = 1;
                auto idx = nv.find('/');
                std::from_chars(nv.data(), nv.data() + idx, n);
                if(idx != std::string::npos) {
                    std::from_chars(nv.data() + idx + 1, nv.data() + nv.size(), d);
                }
            }
            for(auto code = first; code <= last; code++) {
                std::string name;
                if(na_has_hash && cp_attr.empty()) {
                    // Range entry: replace # with the hex codepoint value per character
                    char hex_buf[8];
                    auto [p, ec] = std::to_chars(hex_buf, hex_buf + sizeof(hex_buf),
                                                 static_cast<uint32_t>(code), 16);
                    // to_chars writes lowercase; convert to uppercase
                    for(auto * it = hex_buf; it != p; ++it)
                        *it = (*it >= 'a' && *it <= 'f') ? char(*it - 32) : *it;
                    name = fixup_name(na_template, std::string_view(hex_buf, static_cast<std::size_t>(p - hex_buf)));
                } else {
                    name = fixup_name(na_template, cp_attr);
                }
                db[code] = {code, name, age, category, block, script, exts, n, d, generated(code)};
            }
        } catch(...) {    // stoi...
        }
    }
    return db;
}
