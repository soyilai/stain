#pragma once

#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

namespace stain {

    // RGBA color with float components in [0, 1].
    struct RGBA {
        float r{0}, g{0}, b{0}, a{1};

        // Parse "#RRGGBB" or "#RRGGBBAA" hex string.
        static RGBA from_hex(std::string_view hex);

        // Look up a named CSS color ("red", "transparent", "white", etc.).
        static RGBA from_name(std::string_view name);

        static constexpr RGBA transparent() noexcept {
            return {0, 0, 0, 0};
        }
        static constexpr RGBA white() noexcept {
            return {1, 1, 1, 1};
        }
        static constexpr RGBA black() noexcept {
            return {0, 0, 0, 1};
        }

        // Alpha compositing: this over backdrop (source-over).
        [[nodiscard]] constexpr RGBA blend_over(RGBA backdrop) const noexcept {
            if(a >= 1.0f)
                return *this;
            if(a <= 0.0f)
                return backdrop;

            float out_a = a + backdrop.a * (1.0f - a);
            if(out_a < 1e-6f)
                return transparent();

            float inv = 1.0f / out_a;
            return {
                (r * a + backdrop.r * backdrop.a * (1.0f - a)) * inv,
                (g * a + backdrop.g * backdrop.a * (1.0f - a)) * inv,
                (b * a + backdrop.b * backdrop.a * (1.0f - a)) * inv,
                out_a
            };
        }

        [[nodiscard]] constexpr bool
        approx_equal(RGBA other, float eps = 1e-5f) const noexcept {
            return std::abs(r - other.r) < eps && std::abs(g - other.g) < eps &&
                   std::abs(b - other.b) < eps && std::abs(a - other.a) < eps;
        }

        [[nodiscard]] constexpr bool is_transparent() const noexcept {
            return a < 1e-5f;
        }

        // Convert to 8-bit per channel (0-255).
        [[nodiscard]] constexpr uint8_t r8() const noexcept {
            return static_cast<uint8_t>(r * 255.0f + 0.5f);
        }
        [[nodiscard]] constexpr uint8_t g8() const noexcept {
            return static_cast<uint8_t>(g * 255.0f + 0.5f);
        }
        [[nodiscard]] constexpr uint8_t b8() const noexcept {
            return static_cast<uint8_t>(b * 255.0f + 0.5f);
        }
        [[nodiscard]] constexpr uint8_t a8() const noexcept {
            return static_cast<uint8_t>(a * 255.0f + 0.5f);
        }

        constexpr bool operator==(const RGBA&) const noexcept = default;
    };

    // Accepts RGBA, "#RRGGBB", "#RRGGBBAA", or named color strings.
    using ColorInput = std::variant<RGBA, std::string>;

    // Resolve a ColorInput to an RGBA value.
    RGBA resolve_color(const ColorInput& input);

    // Create an RGBA from a hue in degrees [0, 360). Saturation=1, Value=1.
    RGBA from_hue(float hue);

} // namespace stain
