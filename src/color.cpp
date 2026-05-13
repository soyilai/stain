#include "stain/color.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <string>

namespace stain {

    namespace {

        struct NamedColor {
            const char* name;
            RGBA color;
        };

        // Sorted by name for binary search.
        constexpr std::array<NamedColor, 24> named_colors{{
            {"aqua", {0.0f, 1.0f, 1.0f, 1.0f}},
            {"black", {0.0f, 0.0f, 0.0f, 1.0f}},
            {"blue", {0.0f, 0.0f, 1.0f, 1.0f}},
            {"cyan", {0.0f, 1.0f, 1.0f, 1.0f}},
            {"fuchsia", {1.0f, 0.0f, 1.0f, 1.0f}},
            {"gray", {0.5f, 0.5f, 0.5f, 1.0f}},
            {"green", {0.0f, 0.5f, 0.0f, 1.0f}},
            {"grey", {0.5f, 0.5f, 0.5f, 1.0f}},
            {"lime", {0.0f, 1.0f, 0.0f, 1.0f}},
            {"magenta", {1.0f, 0.0f, 1.0f, 1.0f}},
            {"maroon", {0.5f, 0.0f, 0.0f, 1.0f}},
            {"navy", {0.0f, 0.0f, 0.5f, 1.0f}},
            {"olive", {0.5f, 0.5f, 0.0f, 1.0f}},
            {"orange", {1.0f, 0.647f, 0.0f, 1.0f}},
            {"purple", {0.5f, 0.0f, 0.5f, 1.0f}},
            {"red", {1.0f, 0.0f, 0.0f, 1.0f}},
            {"silver", {0.75f, 0.75f, 0.75f, 1.0f}},
            {"teal", {0.0f, 0.5f, 0.5f, 1.0f}},
            {"transparent", {0.0f, 0.0f, 0.0f, 0.0f}},
            {"turquoise", {0.251f, 0.878f, 0.816f, 1.0f}},
            {"violet", {0.933f, 0.510f, 0.933f, 1.0f}},
            {"white", {1.0f, 1.0f, 1.0f, 1.0f}},
            {"yellow", {1.0f, 1.0f, 0.0f, 1.0f}},
            {"yellowgreen", {0.604f, 0.804f, 0.196f, 1.0f}},
        }};

        uint8_t hex_nibble(char c) {
            if(c >= '0' && c <= '9')
                return static_cast<uint8_t>(c - '0');
            if(c >= 'a' && c <= 'f')
                return static_cast<uint8_t>(c - 'a' + 10);
            if(c >= 'A' && c <= 'F')
                return static_cast<uint8_t>(c - 'A' + 10);
            throw std::invalid_argument(
                std::string("Invalid hex character: ") + c
            );
        }

        uint8_t hex_byte(char hi, char lo) {
            return static_cast<uint8_t>((hex_nibble(hi) << 4) | hex_nibble(lo));
        }

    } // anonymous namespace

    RGBA RGBA::from_hex(std::string_view hex) {
        if(hex.empty() || hex[0] != '#') {
            throw std::invalid_argument("Hex color must start with '#'");
        }
        hex.remove_prefix(1);

        if(hex.size() == 6) {
            return {
                hex_byte(hex[0], hex[1]) / 255.0f,
                hex_byte(hex[2], hex[3]) / 255.0f,
                hex_byte(hex[4], hex[5]) / 255.0f,
                1.0f
            };
        }
        if(hex.size() == 8) {
            return {
                hex_byte(hex[0], hex[1]) / 255.0f,
                hex_byte(hex[2], hex[3]) / 255.0f,
                hex_byte(hex[4], hex[5]) / 255.0f,
                hex_byte(hex[6], hex[7]) / 255.0f
            };
        }
        throw std::invalid_argument(
            "Hex color must be 6 or 8 hex digits after '#'"
        );
    }

    RGBA RGBA::from_name(std::string_view name) {
        // Lowercase the name for case-insensitive lookup.
        std::string lower(name);
        std::transform(
            lower.begin(),
            lower.end(),
            lower.begin(),
            [](unsigned char c) { return std::tolower(c); }
        );

        auto it = std::lower_bound(
            named_colors.begin(),
            named_colors.end(),
            lower,
            [](const NamedColor& nc, const std::string& n) {
                return std::string_view(nc.name) < std::string_view(n);
            }
        );

        if(it != named_colors.end() && std::string_view(it->name) == lower) {
            return it->color;
        }

        throw std::invalid_argument(
            std::string("Unknown color name: ") + std::string(name)
        );
    }

    RGBA resolve_color(const ColorInput& input) {
        return std::visit(
            [](auto&& arg) -> RGBA {
                using T = std::decay_t<decltype(arg)>;
                if constexpr(std::is_same_v<T, RGBA>) {
                    return arg;
                } else {
                    if(!arg.empty() && arg[0] == '#') {
                        return RGBA::from_hex(arg);
                    }
                    return RGBA::from_name(arg);
                }
            },
            input
        );
    }

    RGBA from_hue(float hue) {
        float h = std::fmod(hue, 360.0f);
        if(h < 0)
            h += 360.0f;
        float s = 1.0f, v = 1.0f;
        float c = v * s;
        float hp = h / 60.0f;
        float x = c * (1.0f - std::abs(std::fmod(hp, 2.0f) - 1.0f));
        float r, g, b;
        if(hp < 1) {
            r = c;
            g = x;
            b = 0;
        } else if(hp < 2) {
            r = x;
            g = c;
            b = 0;
        } else if(hp < 3) {
            r = 0;
            g = c;
            b = x;
        } else if(hp < 4) {
            r = 0;
            g = x;
            b = c;
        } else if(hp < 5) {
            r = x;
            g = 0;
            b = c;
        } else {
            r = c;
            g = 0;
            b = x;
        }
        float m = v - c;
        return {r + m, g + m, b + m, 1.0f};
    }

} // namespace stain
