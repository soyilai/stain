#pragma once

#include "stain/color.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace stain {

    // Text attribute flags. Combine with |.
    enum class Attr : uint32_t {
        None = 0,
        Bold = 1 << 0,
        Dim = 1 << 1,
        Italic = 1 << 2,
        Underline = 1 << 3,
        Blink = 1 << 4,
        Reverse = 1 << 5,
        Strike = 1 << 6,
    };

    constexpr Attr operator|(Attr a, Attr b) noexcept {
        return static_cast<Attr>(
            static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
        );
    }

    constexpr Attr operator&(Attr a, Attr b) noexcept {
        return static_cast<Attr>(
            static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
        );
    }

    constexpr Attr operator~(Attr a) noexcept {
        return static_cast<Attr>(~static_cast<uint32_t>(a));
    }

    constexpr Attr& operator|=(Attr& a, Attr b) noexcept {
        a = a | b;
        return a;
    }

    constexpr Attr& operator&=(Attr& a, Attr b) noexcept {
        a = a & b;
        return a;
    }

    constexpr bool has_attr(Attr set, Attr flag) noexcept {
        return (set & flag) != Attr::None;
    }

    // A single cell in the buffer: character, foreground, background,
    // attributes.
    struct Cell {
        char32_t ch{U' '};
        RGBA fg{RGBA::white()};
        RGBA bg{RGBA::transparent()};
        Attr attr{Attr::None};

        constexpr bool operator==(const Cell&) const noexcept = default;
    };

    // Axis-aligned clipping rectangle.
    struct ClipRect {
        int x{0}, y{0}, w{0}, h{0};

        // Intersect this rect with another. Returns empty rect if no overlap.
        [[nodiscard]] ClipRect intersect(ClipRect other) const noexcept {
            int x1 = std::max(x, other.x);
            int y1 = std::max(y, other.y);
            int x2 = std::min(x + w, other.x + other.w);
            int y2 = std::min(y + h, other.y + other.h);
            if(x2 <= x1 || y2 <= y1)
                return {0, 0, 0, 0};
            return {x1, y1, x2 - x1, y2 - y1};
        }

        // Check if a point is inside the rect.
        [[nodiscard]] constexpr bool contains(int px, int py) const noexcept {
            return px >= x && px < x + w && py >= y && py < y + h;
        }

        [[nodiscard]] constexpr bool empty() const noexcept {
            return w <= 0 || h <= 0;
        }
    };

    // Border style for draw_box.
    enum class BorderStyle { None, Single, Double, Rounded, Heavy };

    // Which sides of the border to draw.
    struct BorderSides {
        bool top{true}, right{true}, bottom{true}, left{true};
        static BorderSides all() {
            return {true, true, true, true};
        }
        static BorderSides none() {
            return {false, false, false, false};
        }
    };

    // Options for constructing an OptimizedBuffer.
    struct BufferOptions {
        int width{80};
        int height{24};
    };

    // Decode one UTF-8 codepoint from a string_view. Advances pos past the
    // decoded bytes. Returns U+FFFD on invalid sequences.
    char32_t decode_utf8(std::string_view s, std::size_t& pos);

    // Approximate display width of a Unicode codepoint. Returns 1 or 2.
    int char_display_width(char32_t cp);

    // Encode one codepoint to UTF-8. Writes to out, returns bytes written.
    int encode_utf8(char32_t cp, char* out);

    // Double-buffered cell grid. Supports scissor (clip) and opacity stacks.
    // Usage:
    //   OptimizedBuffer buf({.width = 80, .height = 24});
    //   buf.draw_text(0, 0, "hello", RGBA::white());
    //   buf.set_cell(1, 1, {U'!', RGBA::red(), RGBA::transparent(),
    //   Attr::Bold});
    class OptimizedBuffer {
        public:
        explicit OptimizedBuffer(BufferOptions opts = {});
        ~OptimizedBuffer();

        OptimizedBuffer(const OptimizedBuffer&) = delete;
        OptimizedBuffer& operator=(const OptimizedBuffer&) = delete;
        OptimizedBuffer(OptimizedBuffer&&) noexcept;
        OptimizedBuffer& operator=(OptimizedBuffer&&) noexcept;

        [[nodiscard]] int width() const noexcept;
        [[nodiscard]] int height() const noexcept;

        // Resize arrays and clear content.
        void resize(int w, int h);

        // Fill every cell with a background color.
        void clear(RGBA bg = RGBA::transparent());

        // Write a single cell. Respects scissor and opacity stacks.
        void set_cell(int x, int y, Cell cell);

        // Read a cell (for diffing).
        [[nodiscard]] Cell get_cell(int x, int y) const;

        // Draw a UTF-8 string. Returns the x position after the last char.
        int draw_text(
            int x,
            int y,
            std::string_view text,
            RGBA fg,
            RGBA bg = RGBA::transparent(),
            Attr attr = Attr::None
        );

        // Fill a rectangle with a background color.
        void fill_rect(int x, int y, int w, int h, RGBA bg);

        // Draw a box border.
        void draw_box(
            int x,
            int y,
            int w,
            int h,
            BorderStyle style,
            RGBA border_color,
            RGBA bg = RGBA::transparent(),
            BorderSides sides = BorderSides::all()
        );

        // Copy another buffer into this one at (dx, dy). Skips empty cells.
        void blit(const OptimizedBuffer& src, int dx, int dy);

        // Push a clipping rect. Intersects with any existing scissor.
        void push_scissor(ClipRect rect);
        void pop_scissor();
        [[nodiscard]] std::optional<ClipRect> current_scissor() const;

        // Push a transparency multiplier. Stacks with existing opacity.
        void push_opacity(float opacity);
        void pop_opacity();
        [[nodiscard]] float current_opacity() const noexcept;

        // Raw cell data access. Diffing tools use this.
        [[nodiscard]] std::span<const Cell> cells() const noexcept;
        [[nodiscard]] std::span<Cell> cells_mut() noexcept;

        private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };

} // namespace stain
