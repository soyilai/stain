#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace stain {
namespace detail {

inline std::string base64_encode(std::string_view data) {
    static constexpr char tbl[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    std::size_t i = 0;
    while(i + 3 <= data.size()) {
        auto a = static_cast<uint8_t>(data[i]);
        auto b = static_cast<uint8_t>(data[i + 1]);
        auto c = static_cast<uint8_t>(data[i + 2]);
        out.push_back(tbl[a >> 2]);
        out.push_back(tbl[((a << 4) | (b >> 4)) & 0x3F]);
        out.push_back(tbl[((b << 2) | (c >> 6)) & 0x3F]);
        out.push_back(tbl[c & 0x3F]);
        i += 3;
    }
    if(i < data.size()) {
        auto a = static_cast<uint8_t>(data[i]);
        out.push_back(tbl[a >> 2]);
        if(i + 1 < data.size()) {
            auto b = static_cast<uint8_t>(data[i + 1]);
            out.push_back(tbl[((a << 4) | (b >> 4)) & 0x3F]);
            out.push_back(tbl[(b << 2) & 0x3F]);
            out.push_back('=');
        } else {
            out.push_back(tbl[(a << 4) & 0x3F]);
            out.push_back('=');
            out.push_back('=');
        }
    }
    return out;
}

} // namespace detail
} // namespace stain
