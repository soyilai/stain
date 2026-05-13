#include "stain/buffer.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <vector>

namespace stain {

    char32_t decode_utf8(std::string_view s, std::size_t& pos) {
        if(pos >= s.size())
            return U'\0';

        auto c0 = static_cast<uint8_t>(s[pos]);

        if(c0 < 0x80) {
            pos++;
            return static_cast<char32_t>(c0);
        }

        int len = 0;
        char32_t cp = 0;
        if((c0 & 0xE0) == 0xC0) {
            len = 2;
            cp = c0 & 0x1F;
        } else if((c0 & 0xF0) == 0xE0) {
            len = 3;
            cp = c0 & 0x0F;
        } else if((c0 & 0xF8) == 0xF0) {
            len = 4;
            cp = c0 & 0x07;
        } else {
            pos++;
            return U'\uFFFD';
        }

        if(pos + len > s.size()) {
            pos++;
            return U'\uFFFD';
        }

        for(int i = 1; i < len; i++) {
            auto ci = static_cast<uint8_t>(s[pos + i]);
            if((ci & 0xC0) != 0x80) {
                pos++;
                return U'\uFFFD';
            }
            cp = (cp << 6) | (ci & 0x3F);
        }

        pos += len;
        return cp;
    }

    int encode_utf8(char32_t cp, char* out) {
        if(cp < 0x80) {
            out[0] = static_cast<char>(cp);
            return 1;
        }
        if(cp < 0x800) {
            out[0] = static_cast<char>(0xC0 | (cp >> 6));
            out[1] = static_cast<char>(0x80 | (cp & 0x3F));
            return 2;
        }
        if(cp < 0x10000) {
            out[0] = static_cast<char>(0xE0 | (cp >> 12));
            out[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out[2] = static_cast<char>(0x80 | (cp & 0x3F));
            return 3;
        }
        out[0] = static_cast<char>(0xF0 | (cp >> 18));
        out[1] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out[2] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out[3] = static_cast<char>(0x80 | (cp & 0x3F));
        return 4;
    }

    int char_display_width(char32_t cp) {
        // Control characters.
        if(cp < 0x20 || (cp >= 0x7F && cp < 0xA0))
            return 0;

        // CJK Unified Ideographs and extensions.
        if((cp >= 0x2E80 && cp <= 0x9FFF) || (cp >= 0xF900 && cp <= 0xFAFF) ||
           (cp >= 0xFE30 && cp <= 0xFE6F) || (cp >= 0xFF01 && cp <= 0xFF60) ||
           (cp >= 0xFFE0 && cp <= 0xFFE6) || (cp >= 0x20000 && cp <= 0x2FFFF) ||
           (cp >= 0x30000 && cp <= 0x3FFFF)) {
            return 2;
        }

        // Common emoji ranges (simplified).
        if((cp >= 0x1F300 && cp <= 0x1F9FF) ||
           (cp >= 0x1FA00 && cp <= 0x1FA6F) ||
           (cp >= 0x1FA70 && cp <= 0x1FAFF) || (cp >= 0x2600 && cp <= 0x27BF)) {
            return 2;
        }

        return 1;
    }

    // Box-drawing character sets.
    struct BoxChars {
        char32_t tl, tr, bl, br; // corners
        char32_t h, v;           // horizontal, vertical
    };

    constexpr BoxChars box_single = {U'┌', U'┐', U'└', U'┘', U'─', U'│'};
    constexpr BoxChars box_double = {U'╔', U'╗', U'╚', U'╝', U'═', U'║'};
    constexpr BoxChars box_rounded = {U'╭', U'╮', U'╰', U'╯', U'─', U'│'};
    constexpr BoxChars box_heavy = {U'┏', U'┓', U'┗', U'┛', U'━', U'┃'};

    const BoxChars& get_box_chars(BorderStyle style) {
        switch(style) {
        case BorderStyle::Double:
            return box_double;
        case BorderStyle::Rounded:
            return box_rounded;
        case BorderStyle::Heavy:
            return box_heavy;
        case BorderStyle::Single:
        default:
            return box_single;
        }
    }

    struct OptimizedBuffer::Impl {
        int width{0};
        int height{0};
        std::vector<Cell> cells;
        std::vector<ClipRect> scissor_stack;
        std::vector<float> opacity_stack;

        Impl(BufferOptions o)
            : width(o.width)
            , height(o.height)
            , cells(static_cast<std::size_t>(o.width * o.height)) {
        }

        Cell& at(int x, int y) {
            return cells[static_cast<std::size_t>(y * width + x)];
        }

        const Cell& at(int x, int y) const {
            return cells[static_cast<std::size_t>(y * width + x)];
        }

        bool in_bounds(int x, int y) const noexcept {
            return x >= 0 && x < width && y >= 0 && y < height;
        }

        bool clipped(int x, int y) const noexcept {
            if(scissor_stack.empty())
                return false;
            return !scissor_stack.back().contains(x, y);
        }

        float effective_opacity() const noexcept {
            if(opacity_stack.empty())
                return 1.0f;
            return opacity_stack.back();
        }
    };

    OptimizedBuffer::OptimizedBuffer(BufferOptions opts)
        : _impl(std::make_unique<Impl>(opts)) {
    }

    OptimizedBuffer::~OptimizedBuffer() = default;

    OptimizedBuffer::OptimizedBuffer(OptimizedBuffer&&) noexcept = default;
    OptimizedBuffer&
    OptimizedBuffer::operator=(OptimizedBuffer&&) noexcept = default;

    int OptimizedBuffer::width() const noexcept {
        return _impl->width;
    }
    int OptimizedBuffer::height() const noexcept {
        return _impl->height;
    }

    void OptimizedBuffer::resize(int w, int h) {
        _impl->width = w;
        _impl->height = h;
        _impl->cells.assign(static_cast<std::size_t>(w * h), Cell{});
    }

    void OptimizedBuffer::clear(RGBA bg) {
        Cell blank{U' ', RGBA::white(), bg, Attr::None};
        std::fill(_impl->cells.begin(), _impl->cells.end(), blank);
    }

    void OptimizedBuffer::set_cell(int x, int y, Cell cell) {
        if(!_impl->in_bounds(x, y))
            return;
        if(_impl->clipped(x, y))
            return;

        // Apply opacity to the cell colors.
        float op = _impl->effective_opacity();
        if(op < 1.0f) {
            cell.fg.a *= op;
            cell.bg.a *= op;
        }

        if(cell.bg.is_transparent()) {
            cell.bg = _impl->at(x, y).bg;
        }

        _impl->at(x, y) = cell;
    }

    Cell OptimizedBuffer::get_cell(int x, int y) const {
        if(!_impl->in_bounds(x, y))
            return Cell{};
        return _impl->at(x, y);
    }

    int OptimizedBuffer::draw_text(
        int x,
        int y,
        std::string_view text,
        RGBA fg,
        RGBA bg,
        Attr attr
    ) {
        std::size_t pos = 0;
        int cx = x;
        while(pos < text.size()) {
            char32_t cp = decode_utf8(text, pos);
            if(cp == U'\0')
                break;

            int w = char_display_width(cp);
            if(w == 0)
                continue;

            set_cell(cx, y, Cell{cp, fg, bg, attr});
            cx++;

            // Wide chars occupy two cells; second is a null continuation.
            if(w == 2) {
                set_cell(cx, y, Cell{U'\0', fg, bg, attr});
                cx++;
            }
        }
        return cx;
    }

    void OptimizedBuffer::fill_rect(int x, int y, int w, int h, RGBA bg) {
        for(int row = y; row < y + h; row++) {
            for(int col = x; col < x + w; col++) {
                set_cell(col, row, Cell{U' ', RGBA::white(), bg, Attr::None});
            }
        }
    }

    void OptimizedBuffer::draw_box(
        int x,
        int y,
        int w,
        int h,
        BorderStyle style,
        RGBA border_color,
        RGBA bg,
        BorderSides sides
    ) {
        if(style == BorderStyle::None || w < 2 || h < 2)
            return;

        const auto& bc = get_box_chars(style);

        // Fill interior.
        if(!bg.is_transparent()) {
            fill_rect(x + 1, y + 1, w - 2, h - 2, bg);
        }

        // Top edge.
        if(sides.top) {
            for(int col = x + 1; col < x + w - 1; col++)
                set_cell(col, y, Cell{bc.h, border_color, bg, Attr::None});
        }

        // Bottom edge.
        if(sides.bottom) {
            for(int col = x + 1; col < x + w - 1; col++)
                set_cell(
                    col,
                    y + h - 1,
                    Cell{bc.h, border_color, bg, Attr::None}
                );
        }

        // Left edge.
        if(sides.left) {
            for(int row = y + 1; row < y + h - 1; row++)
                set_cell(x, row, Cell{bc.v, border_color, bg, Attr::None});
        }

        // Right edge.
        if(sides.right) {
            for(int row = y + 1; row < y + h - 1; row++)
                set_cell(
                    x + w - 1,
                    row,
                    Cell{bc.v, border_color, bg, Attr::None}
                );
        }

        // Corners, only draw if both adjacent sides are active.
        if(sides.top && sides.left)
            set_cell(x, y, Cell{bc.tl, border_color, bg, Attr::None});
        else if(sides.top)
            set_cell(x, y, Cell{bc.h, border_color, bg, Attr::None});
        else if(sides.left)
            set_cell(x, y, Cell{bc.v, border_color, bg, Attr::None});

        if(sides.top && sides.right)
            set_cell(x + w - 1, y, Cell{bc.tr, border_color, bg, Attr::None});
        else if(sides.top)
            set_cell(x + w - 1, y, Cell{bc.h, border_color, bg, Attr::None});
        else if(sides.right)
            set_cell(x + w - 1, y, Cell{bc.v, border_color, bg, Attr::None});

        if(sides.bottom && sides.left)
            set_cell(x, y + h - 1, Cell{bc.bl, border_color, bg, Attr::None});
        else if(sides.bottom)
            set_cell(x, y + h - 1, Cell{bc.h, border_color, bg, Attr::None});
        else if(sides.left)
            set_cell(x, y + h - 1, Cell{bc.v, border_color, bg, Attr::None});

        if(sides.bottom && sides.right)
            set_cell(
                x + w - 1,
                y + h - 1,
                Cell{bc.br, border_color, bg, Attr::None}
            );
        else if(sides.bottom)
            set_cell(
                x + w - 1,
                y + h - 1,
                Cell{bc.h, border_color, bg, Attr::None}
            );
        else if(sides.right)
            set_cell(
                x + w - 1,
                y + h - 1,
                Cell{bc.v, border_color, bg, Attr::None}
            );
    }

    void OptimizedBuffer::blit(const OptimizedBuffer& src, int dx, int dy) {
        for(int row = 0; row < src.height(); row++) {
            for(int col = 0; col < src.width(); col++) {
                Cell cell = src.get_cell(col, row);
                if(cell.bg.is_transparent() && cell.ch == U' ')
                    continue;
                set_cell(dx + col, dy + row, cell);
            }
        }
    }

    void OptimizedBuffer::push_scissor(ClipRect rect) {
        if(!_impl->scissor_stack.empty()) {
            // Intersect with the current scissor.
            rect = _impl->scissor_stack.back().intersect(rect);
        }
        _impl->scissor_stack.push_back(rect);
    }

    void OptimizedBuffer::pop_scissor() {
        assert(!_impl->scissor_stack.empty());
        _impl->scissor_stack.pop_back();
    }

    std::optional<ClipRect> OptimizedBuffer::current_scissor() const {
        if(_impl->scissor_stack.empty())
            return std::nullopt;
        return _impl->scissor_stack.back();
    }

    void OptimizedBuffer::push_opacity(float opacity) {
        float effective = opacity;
        if(!_impl->opacity_stack.empty()) {
            effective *= _impl->opacity_stack.back();
        }
        _impl->opacity_stack.push_back(effective);
    }

    void OptimizedBuffer::pop_opacity() {
        assert(!_impl->opacity_stack.empty());
        _impl->opacity_stack.pop_back();
    }

    float OptimizedBuffer::current_opacity() const noexcept {
        return _impl->effective_opacity();
    }

    std::span<const Cell> OptimizedBuffer::cells() const noexcept {
        return std::span<const Cell>(_impl->cells);
    }

    std::span<Cell> OptimizedBuffer::cells_mut() noexcept {
        return std::span<Cell>(_impl->cells);
    }

} // namespace stain
