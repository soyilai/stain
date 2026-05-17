#include "stain/ascii_font.hpp"
#include "stain/buffer.hpp"

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace stain {

namespace {

struct FontDef {
    int glyph_width{0};
    int glyph_height{0};
    std::unordered_map<char32_t, std::vector<std::string>> glyphs;
};

const FontDef& tiny_font() {
    static const FontDef font = [] {
        FontDef f;
        f.glyph_width = 4;
        f.glyph_height = 4;

        auto g = [&](char32_t ch, std::vector<std::string> rows) {
            f.glyphs[ch] = std::move(rows);
        };

        g(U' ', {"    ", "    ", "    ", "    "});

        g(U'A', {" ██ ", "█  █", "████", "█  █"});
        g(U'B', {"███ ", "█  █", "███ ", "█  █"});
        g(U'C', {" ██ ", "█   ", "█   ", " ██ "});
        g(U'D', {"███ ", "█  █", "█  █", "███ "});
        g(U'E', {"████", "█   ", "███ ", "████"});
        g(U'F', {"████", "█   ", "███ ", "█   "});
        g(U'G', {" ██ ", "█   ", "█ ██", " ██ "});
        g(U'H', {"█  █", "████", "█  █", "█  █"});
        g(U'I', {"████", " ██ ", " ██ ", "████"});
        g(U'J', {"  ██", "   █", "█  █", " ██ "});
        g(U'K', {"█  █", "██  ", "█ █ ", "█  █"});
        g(U'L', {"█   ", "█   ", "█   ", "████"});
        g(U'M', {"█  █", "██ █", "█ ██", "█  █"});
        g(U'N', {"█  █", "██ █", "█ ██", "█  █"});
        g(U'O', {" ██ ", "█  █", "█  █", " ██ "});
        g(U'P', {"███ ", "█  █", "███ ", "█   "});
        g(U'Q', {" ██ ", "█  █", "█ ██", " ██▄"});
        g(U'R', {"███ ", "█  █", "███ ", "█ █ "});
        g(U'S', {" ███", "█   ", " ██ ", "███ "});
        g(U'T', {"████", " ██ ", " ██ ", " ██ "});
        g(U'U', {"█  █", "█  █", "█  █", " ██ "});
        g(U'V', {"█  █", "█  █", " ██ ", " ██ "});
        g(U'W', {"█  █", "█  █", "█  █", " ███"});
        g(U'X', {"█  █", " ██ ", " ██ ", "█  █"});
        g(U'Y', {"█  █", " ██ ", " ██ ", " ██ "});
        g(U'Z', {"████", "  █ ", " █  ", "████"});

        g(U'0', {" ██ ", "█  █", "█  █", " ██ "});
        g(U'1', {"  █ ", " ██ ", "  █ ", " ███"});
        g(U'2', {"████", "   █", " ██ ", "█   "});
        g(U'3', {"████", "   █", " ███", "   █"});
        g(U'4', {"█  █", "█  █", "████", "   █"});
        g(U'5', {"████", "█   ", " ███", "   █"});
        g(U'6', {" ██ ", "█   ", "████", "█  █"});
        g(U'7', {"████", "   █", "  █ ", " █  "});
        g(U'8', {" ██ ", "█  █", " ██ ", "█  █"});
        g(U'9', {" ██ ", "█  █", " ███", "   █"});

        g(U'.', {"    ", "    ", "    ", " █  "});
        g(U',', {"    ", "    ", " █  ", " █  "});
        g(U'!', {" █  ", " █  ", "    ", " █  "});
        g(U'?', {" ██ ", "   █", "  █ ", "    "});
        g(U'-', {"    ", "    ", "████", "    "});
        g(U':', {"    ", " █  ", "    ", " █  "});
        g(U'\'', {" █  ", " █  ", "    ", "    "});
        g(U'"', {" █ █", " █ █", "    ", "    "});
        g(U'/', {"   █", "  █ ", " █  ", "█   "});
        g(U'\\', {"█   ", " █  ", "  █ ", "   █"});

        return f;
    }();
    return font;
}

const FontDef& block_font() {
    static const FontDef font = [] {
        FontDef f;
        f.glyph_width = 4;
        f.glyph_height = 3;

        auto g = [&](char32_t ch, std::vector<std::string> rows) {
            f.glyphs[ch] = std::move(rows);
        };

        g(U' ', {"    ", "    ", "    "});

        g(U'A', {" ██ ", "█  █", "████"});
        g(U'B', {"███ ", "█  █", "███ "});
        g(U'C', {" ██ ", "█   ", " ██ "});
        g(U'D', {"███ ", "█  █", "███ "});
        g(U'E', {"████", "█   ", "████"});
        g(U'F', {"████", "█   ", "█   "});
        g(U'G', {" ██ ", "█ ██", " ██ "});
        g(U'H', {"█  █", "████", "█  █"});
        g(U'I', {"████", " ██ ", "████"});
        g(U'J', {"  ██", "   █", " ██ "});
        g(U'K', {"█  █", "██  ", "█ █ "});
        g(U'L', {"█   ", "█   ", "████"});
        g(U'M', {"█  █", "██ █", "█ ██"});
        g(U'N', {"█  █", "██ █", "█ ██"});
        g(U'O', {" ██ ", "█  █", " ██ "});
        g(U'P', {"███ ", "█  █", "███ "});
        g(U'Q', {" ██ ", "█ ██", " ██▄"});
        g(U'R', {"███ ", "█ █ ", "█  █"});
        g(U'S', {" ███", "█   ", "███ "});
        g(U'T', {"████", " ██ ", " ██ "});
        g(U'U', {"█  █", "█  █", " ██ "});
        g(U'V', {"█  █", "█  █", " ██ "});
        g(U'W', {"█  █", "█ ██", " ███"});
        g(U'X', {"█  █", " ██ ", "█  █"});
        g(U'Y', {"█  █", " ██ ", " ██ "});
        g(U'Z', {"████", "  █ ", "████"});

        g(U'0', {" ██ ", "█  █", " ██ "});
        g(U'1', {"  █ ", " ██ ", " ███"});
        g(U'2', {" ██ ", "   █", " ██ "});
        g(U'3', {" ██ ", "  █ ", " ███"});
        g(U'4', {"█  █", "████", "   █"});
        g(U'5', {"████", "█   ", "████"});
        g(U'6', {" ██ ", "█   ", "███ "});
        g(U'7', {"████", "  █ ", " █  "});
        g(U'8', {" ██ ", "█  █", " ██ "});
        g(U'9', {" ██ ", "█  █", " ███"});

        g(U'.', {"    ", "    ", " █  "});
        g(U',', {"    ", "    ", " █  "});
        g(U'!', {" █  ", "    ", " █  "});
        g(U'?', {" ██ ", "  █ ", " █  "});
        g(U'-', {"    ", "████", "    "});
        g(U':', {"    ", " █  ", " █  "});
        g(U'\'', {" █  ", "    ", "    "});
        g(U'"', {" █ █", "    ", "    "});
        g(U'/', {"   █", "  █ ", " █  "});
        g(U'\\', {"█   ", " █  ", "  █ "});

        return f;
    }();
    return font;
}

const FontDef* get_font(AsciiFontType type) {
    switch(type) {
    case AsciiFontType::Tiny:
        return &tiny_font();
    case AsciiFontType::Block:
        return &block_font();
    }
    return nullptr;
}

void render_line(
    OptimizedBuffer& buf,
    int sx, int sy,
    const FontDef* font,
    std::string_view text,
    RGBA fg, RGBA bg
) {
    if(!font || text.empty())
        return;

    int gw = font->glyph_width;
    int gh = font->glyph_height;
    int spacing = 1;

    // Collect codepoints from UTF-8 input.
    std::vector<char32_t> codepoints;
    std::size_t pos = 0;
    while(pos < text.size()) {
        char32_t cp = decode_utf8(text, pos);
        if(cp != U'\0')
            codepoints.push_back(cp);
    }

    for(int row = 0; row < gh; row++) {
        int x = sx;
        std::string line;
        line.reserve(codepoints.size() * (gw + spacing));

        for(char32_t cp : codepoints) {
            char32_t uc = cp;
            // Upper-case ASCII letters.
            if(uc >= U'a' && uc <= U'z')
                uc = uc - U'a' + U'A';

            auto it = font->glyphs.find(uc);
            const auto* glyph =
                it != font->glyphs.end() ? &it->second : nullptr;

            if(glyph && row < static_cast<int>(glyph->size())) {
                line += (*glyph)[row];
            } else {
                line += std::string(gw, ' ');
            }
            line += std::string(spacing, ' ');
        }

        if(!line.empty() && sy + row >= 0) {
            buf.draw_text(x, sy + row, line, fg, bg);
        }
    }
}

} // anonymous namespace

AsciiFont::AsciiFont(RenderContext* ctx, RenderableOptions opts)
    : Renderable(ctx, std::move(opts)) {
}

AsciiFont& AsciiFont::text(std::string_view t) {
    _text = std::string(t);
    request_render();
    return *this;
}

AsciiFont& AsciiFont::font(AsciiFontType f) {
    _font = f;
    request_render();
    return *this;
}

AsciiFont& AsciiFont::fg(RGBA color) {
    _fg = color;
    request_render();
    return *this;
}

AsciiFont& AsciiFont::bg(RGBA color) {
    _bg = color;
    request_render();
    return *this;
}

void AsciiFont::draw(OptimizedBuffer& buf, double) {
    auto* font_def = get_font(_font);
    if(!font_def || _text.empty())
        return;

    int sx = screen_x();
    int sy = screen_y();

    // Split input text into lines by newline.
    std::size_t line_start = 0;
    int line_y = sy;

    while(line_start < _text.size() && line_y < sy + layout_h()) {
        std::size_t line_end = _text.find('\n', line_start);
        if(line_end == std::string_view::npos)
            line_end = _text.size();

        std::string_view line(
            _text.data() + line_start, line_end - line_start
        );

        render_line(
            buf,
            sx, line_y,
            font_def,
            line,
            _fg,
            _bg
        );

        line_y += font_def->glyph_height + 1;
        line_start = line_end + 1;
    }
}

} // namespace stain
