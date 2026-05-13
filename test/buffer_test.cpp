#include "stain/buffer.hpp"
#include <gtest/gtest.h>

using namespace stain;

TEST(ClipRectTest, contains_inside) {
    ClipRect r{10, 10, 20, 20};
    EXPECT_TRUE(r.contains(10, 10));
    EXPECT_TRUE(r.contains(29, 29));
    EXPECT_TRUE(r.contains(15, 15));
}

TEST(ClipRectTest, contains_outside) {
    ClipRect r{10, 10, 20, 20};
    EXPECT_FALSE(r.contains(9, 10));
    EXPECT_FALSE(r.contains(10, 9));
    EXPECT_FALSE(r.contains(30, 10));
    EXPECT_FALSE(r.contains(10, 30));
}

TEST(ClipRectTest, empty) {
    EXPECT_TRUE((ClipRect{0, 0, 0, 10}.empty()));
    EXPECT_TRUE((ClipRect{0, 0, 10, 0}.empty()));
    EXPECT_FALSE((ClipRect{0, 0, 10, 10}.empty()));
}

TEST(ClipRectTest, intersect_overlapping) {
    ClipRect a{0, 0, 10, 10};
    ClipRect b{5, 5, 10, 10};
    ClipRect result = a.intersect(b);
    EXPECT_EQ(result.x, 5);
    EXPECT_EQ(result.y, 5);
    EXPECT_EQ(result.w, 5);
    EXPECT_EQ(result.h, 5);
}

TEST(ClipRectTest, intersect_no_overlap) {
    ClipRect a{0, 0, 10, 10};
    ClipRect b{20, 20, 10, 10};
    ClipRect result = a.intersect(b);
    EXPECT_TRUE(result.empty());
}

TEST(ClipRectTest, intersect_contained) {
    ClipRect a{0, 0, 20, 20};
    ClipRect b{5, 5, 10, 10};
    ClipRect result = a.intersect(b);
    EXPECT_EQ(result.x, 5);
    EXPECT_EQ(result.y, 5);
    EXPECT_EQ(result.w, 10);
    EXPECT_EQ(result.h, 10);
}

TEST(ClipRectTest, intersect_identical) {
    ClipRect a{0, 0, 10, 10};
    ClipRect result = a.intersect(a);
    EXPECT_EQ(result.x, 0);
    EXPECT_EQ(result.y, 0);
    EXPECT_EQ(result.w, 10);
    EXPECT_EQ(result.h, 10);
}

TEST(ClipRectTest, intersect_touching_edge) {
    ClipRect a{0, 0, 10, 10};
    ClipRect b{10, 0, 10, 10};
    ClipRect result = a.intersect(b);
    EXPECT_TRUE(result.empty());
}

TEST(Utf8Test, decode_ascii) {
    std::string s = "hello";
    std::size_t pos = 0;
    EXPECT_EQ(decode_utf8(s, pos), U'h');
    EXPECT_EQ(pos, 1);
}

TEST(Utf8Test, decode_2byte) {
    std::string s = "\xC3\xA9"; // é
    std::size_t pos = 0;
    EXPECT_EQ(decode_utf8(s, pos), U'\xE9');
    EXPECT_EQ(pos, 2);
}

TEST(Utf8Test, decode_3byte) {
    std::string s = "\xE2\x82\xAC"; // €
    std::size_t pos = 0;
    EXPECT_EQ(decode_utf8(s, pos), 0x20AC);
    EXPECT_EQ(pos, 3);
}

TEST(Utf8Test, decode_4byte) {
    std::string s = "\xF0\x9F\x98\x80"; // 😀
    std::size_t pos = 0;
    EXPECT_EQ(decode_utf8(s, pos), 0x1F600);
    EXPECT_EQ(pos, 4);
}

TEST(Utf8Test, decode_invalid_byte) {
    std::string s = "\xFF";
    std::size_t pos = 0;
    EXPECT_EQ(decode_utf8(s, pos), U'\uFFFD');
    EXPECT_EQ(pos, 1);
}

TEST(Utf8Test, decode_truncated_sequence) {
    std::string s = "\xC3";
    std::size_t pos = 0;
    EXPECT_EQ(decode_utf8(s, pos), U'\uFFFD');
    EXPECT_EQ(pos, 1);
}

TEST(Utf8Test, decode_multi) {
    std::string s = "a\xC3\xA9\xE2\x82\xAC";
    std::size_t pos = 0;
    EXPECT_EQ(decode_utf8(s, pos), U'a');
    EXPECT_EQ(pos, 1);
    EXPECT_EQ(decode_utf8(s, pos), U'\xE9');
    EXPECT_EQ(pos, 3);
    EXPECT_EQ(decode_utf8(s, pos), 0x20AC);
    EXPECT_EQ(pos, 6);
}

TEST(CharWidthTest, ascii_width_1) {
    EXPECT_EQ(char_display_width(U'a'), 1);
    EXPECT_EQ(char_display_width(U'Z'), 1);
    EXPECT_EQ(char_display_width(U'0'), 1);
}

TEST(CharWidthTest, cjk_width_2) {
    EXPECT_EQ(char_display_width(0x4E00), 2); // 一
    EXPECT_EQ(char_display_width(0x9FFF), 2);
    EXPECT_EQ(char_display_width(0x3001), 2); // 、
}

TEST(CharWidthTest, emoji_width_2) {
    EXPECT_EQ(char_display_width(0x1F600), 2); // 😀
    EXPECT_EQ(char_display_width(0x2600), 2);  // ☀
}

TEST(CharWidthTest, control_chars_width_0) {
    EXPECT_EQ(char_display_width(0x00), 0);
    EXPECT_EQ(char_display_width(0x1F), 0);
    EXPECT_EQ(char_display_width(0x7F), 0);
}

TEST(CharWidthTest, newline_width_0) {
    EXPECT_EQ(char_display_width(U'\n'), 0);
}

TEST(AttrTest, bitwise_ops) {
    Attr a = Attr::Bold;
    Attr b = Attr::Italic;
    EXPECT_TRUE(has_attr(a | b, Attr::Bold));
    EXPECT_TRUE(has_attr(a | b, Attr::Italic));
    EXPECT_FALSE(has_attr(a | b, Attr::Underline));
}

TEST(AttrTest, complement) {
    Attr a = Attr::Bold | Attr::Italic;
    Attr not_a = ~a;
    EXPECT_FALSE(has_attr(not_a, Attr::Bold));
    EXPECT_FALSE(has_attr(not_a, Attr::Italic));
    EXPECT_TRUE(has_attr(not_a, Attr::Strike));
}

TEST(AttrTest, compound_assign_or) {
    Attr a = Attr::None;
    a |= Attr::Bold;
    EXPECT_TRUE(has_attr(a, Attr::Bold));
}

TEST(AttrTest, compound_assign_and) {
    Attr a = Attr::Bold | Attr::Italic;
    a &= Attr::Bold;
    EXPECT_TRUE(has_attr(a, Attr::Bold));
    EXPECT_FALSE(has_attr(a, Attr::Italic));
}

TEST(CellTest, default_cell) {
    Cell c;
    EXPECT_EQ(c.ch, U' ');
    EXPECT_TRUE(c.fg.approx_equal(RGBA::white()));
    EXPECT_TRUE(c.bg.approx_equal(RGBA::transparent()));
    EXPECT_EQ(c.attr, Attr::None);
}

TEST(CellTest, equality) {
    Cell a{U'a', RGBA::white(), RGBA::black(), Attr::Bold};
    Cell b{U'a', RGBA::white(), RGBA::black(), Attr::Bold};
    Cell c{U'b', RGBA::white(), RGBA::black(), Attr::Bold};
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
}

TEST(OptimizedBufferTest, create) {
    OptimizedBuffer buf(BufferOptions{.width = 80, .height = 24});
    EXPECT_EQ(buf.width(), 80);
    EXPECT_EQ(buf.height(), 24);
}

TEST(OptimizedBufferTest, default_size) {
    OptimizedBuffer buf;
    EXPECT_EQ(buf.width(), 80);
    EXPECT_EQ(buf.height(), 24);
}

TEST(OptimizedBufferTest, resize) {
    OptimizedBuffer buf(BufferOptions{.width = 10, .height = 10});
    buf.resize(20, 30);
    EXPECT_EQ(buf.width(), 20);
    EXPECT_EQ(buf.height(), 30);
}

TEST(OptimizedBufferTest, get_set_cell) {
    OptimizedBuffer buf(BufferOptions{.width = 10, .height = 10});
    Cell cell{U'X', RGBA{1, 0, 0, 1}, RGBA{0, 0, 0, 1}, Attr::Bold};
    buf.set_cell(5, 5, cell);
    Cell result = buf.get_cell(5, 5);
    EXPECT_EQ(result.ch, U'X');
    EXPECT_TRUE(result.fg.approx_equal(RGBA{1, 0, 0, 1}));
    EXPECT_EQ(result.attr, Attr::Bold);
}

TEST(OptimizedBufferTest, set_cell_out_of_bounds) {
    OptimizedBuffer buf(BufferOptions{.width = 10, .height = 10});
    buf.set_cell(100, 100, Cell{U'X'});
    EXPECT_EQ(buf.get_cell(100, 100).ch, U' ');
}

TEST(OptimizedBufferTest, get_cell_out_of_bounds) {
    OptimizedBuffer buf(BufferOptions{.width = 10, .height = 10});
    Cell result = buf.get_cell(100, 100);
    EXPECT_EQ(result.ch, U' ');
}

TEST(OptimizedBufferTest, clear) {
    OptimizedBuffer buf(BufferOptions{.width = 10, .height = 10});
    buf.set_cell(
        3,
        3,
        Cell{U'X', RGBA{1, 0, 0, 1}, RGBA{0, 0, 0, 1}, Attr::Bold}
    );
    buf.clear(RGBA::black());
    Cell result = buf.get_cell(3, 3);
    EXPECT_EQ(result.ch, U' ');
    EXPECT_TRUE(result.bg.approx_equal(RGBA::black()));
}

TEST(OptimizedBufferTest, draw_text) {
    OptimizedBuffer buf(BufferOptions{.width = 80, .height = 24});
    int end_x = buf.draw_text(10, 10, "ABC", RGBA{1, 1, 1, 1});
    EXPECT_EQ(end_x, 13);
    EXPECT_EQ(buf.get_cell(10, 10).ch, U'A');
    EXPECT_EQ(buf.get_cell(11, 10).ch, U'B');
    EXPECT_EQ(buf.get_cell(12, 10).ch, U'C');
}

TEST(OptimizedBufferTest, draw_text_with_fg_color) {
    OptimizedBuffer buf(BufferOptions{.width = 80, .height = 24});
    buf.draw_text(0, 0, "X", RGBA{1, 0, 0, 1});
    EXPECT_TRUE(buf.get_cell(0, 0).fg.approx_equal(RGBA{1, 0, 0, 1}));
}

TEST(OptimizedBufferTest, scissor) {
    OptimizedBuffer buf(BufferOptions{.width = 10, .height = 10});
    buf.push_scissor(ClipRect{2, 2, 3, 3});
    buf.set_cell(3, 3, Cell{U'A', RGBA{1, 0, 0, 1}});
    buf.set_cell(10, 10, Cell{U'B', RGBA{1, 0, 0, 1}});
    EXPECT_EQ(buf.get_cell(3, 3).ch, U'A');
    EXPECT_EQ(buf.get_cell(10, 10).ch, U' ');
    EXPECT_EQ(buf.get_cell(1, 1).ch, U' ');
}

TEST(OptimizedBufferTest, scissor_intersection) {
    OptimizedBuffer buf(BufferOptions{.width = 10, .height = 10});
    buf.push_scissor(ClipRect{0, 0, 5, 5});
    buf.push_scissor(ClipRect{3, 3, 5, 5});
    ClipRect current = *buf.current_scissor();
    EXPECT_EQ(current.x, 3);
    EXPECT_EQ(current.y, 3);
    EXPECT_EQ(current.w, 2);
    EXPECT_EQ(current.h, 2);
}

TEST(OptimizedBufferTest, pop_scissor_restores) {
    OptimizedBuffer buf(BufferOptions{.width = 10, .height = 10});
    buf.push_scissor(ClipRect{0, 0, 5, 5});
    buf.push_scissor(ClipRect{3, 3, 5, 5});
    buf.pop_scissor();
    ClipRect current = *buf.current_scissor();
    EXPECT_EQ(current.w, 5);
    EXPECT_EQ(current.h, 5);
}

TEST(OptimizedBufferTest, no_scissor_returns_nullopt) {
    OptimizedBuffer buf(BufferOptions{.width = 10, .height = 10});
    EXPECT_FALSE(buf.current_scissor().has_value());
}

TEST(OptimizedBufferTest, opacity) {
    OptimizedBuffer buf(BufferOptions{.width = 10, .height = 10});
    EXPECT_FLOAT_EQ(buf.current_opacity(), 1.0f);
    buf.push_opacity(0.5f);
    EXPECT_FLOAT_EQ(buf.current_opacity(), 0.5f);
    buf.push_opacity(0.5f);
    EXPECT_FLOAT_EQ(buf.current_opacity(), 0.25f);
    buf.pop_opacity();
    EXPECT_FLOAT_EQ(buf.current_opacity(), 0.5f);
    buf.pop_opacity();
    EXPECT_FLOAT_EQ(buf.current_opacity(), 1.0f);
}

TEST(OptimizedBufferTest, fill_rect) {
    OptimizedBuffer buf(BufferOptions{.width = 10, .height = 10});
    buf.fill_rect(2, 2, 3, 3, RGBA{1, 0, 0, 1});
    for(int y = 2; y < 5; y++) {
        for(int x = 2; x < 5; x++) {
            EXPECT_TRUE(buf.get_cell(x, y).bg.approx_equal(RGBA{1, 0, 0, 1}));
        }
    }
}

TEST(OptimizedBufferTest, blit) {
    OptimizedBuffer src(BufferOptions{.width = 5, .height = 5});
    src.set_cell(0, 0, Cell{U'A', RGBA{1, 0, 0, 1}});
    src.set_cell(1, 0, Cell{U'B', RGBA{0, 1, 0, 1}});

    OptimizedBuffer dst(BufferOptions{.width = 10, .height = 10});
    dst.blit(src, 3, 3);

    EXPECT_EQ(dst.get_cell(3, 3).ch, U'A');
    EXPECT_TRUE(dst.get_cell(3, 3).fg.approx_equal(RGBA{1, 0, 0, 1}));
    EXPECT_EQ(dst.get_cell(4, 3).ch, U'B');
}

TEST(OptimizedBufferTest, blit_skips_transparent) {
    OptimizedBuffer src(BufferOptions{.width = 5, .height = 5});
    src.clear();
    src.set_cell(0, 0, Cell{U'A', RGBA{1, 0, 0, 1}});

    OptimizedBuffer dst(BufferOptions{.width = 10, .height = 10});
    dst.clear(RGBA{0, 0, 0, 1});
    // src(1,0) is transparent+space so it should NOT overwrite this
    dst.set_cell(4, 3, Cell{U'O', RGBA{1, 1, 1, 1}});
    dst.blit(src, 3, 3);

    EXPECT_EQ(dst.get_cell(3, 3).ch, U'A');
    EXPECT_EQ(dst.get_cell(4, 3).ch, U'O');
}

TEST(OptimizedBufferTest, draw_box) {
    OptimizedBuffer buf(BufferOptions{.width = 10, .height = 10});
    buf.draw_box(
        1,
        1,
        5,
        5,
        BorderStyle::Single,
        RGBA::white(),
        RGBA::transparent()
    );
    EXPECT_EQ(buf.get_cell(1, 1).ch, U'┌');
    EXPECT_EQ(buf.get_cell(5, 1).ch, U'┐');
    EXPECT_EQ(buf.get_cell(1, 5).ch, U'└');
    EXPECT_EQ(buf.get_cell(5, 5).ch, U'┘');
}

TEST(OptimizedBufferTest, draw_box_none) {
    OptimizedBuffer buf(BufferOptions{.width = 10, .height = 10});
    buf.draw_box(1, 1, 5, 5, BorderStyle::None, RGBA::white());
    EXPECT_EQ(buf.get_cell(1, 1).ch, U' ');
}

TEST(OptimizedBufferTest, draw_box_too_small) {
    OptimizedBuffer buf(BufferOptions{.width = 10, .height = 10});
    buf.draw_box(1, 1, 1, 1, BorderStyle::Single, RGBA::white());
    EXPECT_EQ(buf.get_cell(1, 1).ch, U' ');
}

TEST(OptimizedBufferTest, draw_box_double) {
    OptimizedBuffer buf(BufferOptions{.width = 10, .height = 10});
    buf.draw_box(1, 1, 5, 5, BorderStyle::Double, RGBA::white());
    EXPECT_EQ(buf.get_cell(1, 1).ch, U'╔');
    EXPECT_EQ(buf.get_cell(5, 1).ch, U'╗');
    EXPECT_EQ(buf.get_cell(1, 5).ch, U'╚');
    EXPECT_EQ(buf.get_cell(5, 5).ch, U'╝');
}

TEST(OptimizedBufferTest, draw_box_rounded) {
    OptimizedBuffer buf(BufferOptions{.width = 10, .height = 10});
    buf.draw_box(1, 1, 5, 5, BorderStyle::Rounded, RGBA::white());
    EXPECT_EQ(buf.get_cell(1, 1).ch, U'╭');
    EXPECT_EQ(buf.get_cell(5, 1).ch, U'╮');
    EXPECT_EQ(buf.get_cell(1, 5).ch, U'╰');
    EXPECT_EQ(buf.get_cell(5, 5).ch, U'╯');
}

TEST(OptimizedBufferTest, draw_box_heavy) {
    OptimizedBuffer buf(BufferOptions{.width = 10, .height = 10});
    buf.draw_box(1, 1, 5, 5, BorderStyle::Heavy, RGBA::white());
    EXPECT_EQ(buf.get_cell(1, 1).ch, U'┏');
    EXPECT_EQ(buf.get_cell(5, 1).ch, U'┓');
    EXPECT_EQ(buf.get_cell(1, 5).ch, U'┗');
    EXPECT_EQ(buf.get_cell(5, 5).ch, U'┛');
}

TEST(OptimizedBufferTest, draw_box_border_color) {
    OptimizedBuffer buf(BufferOptions{.width = 10, .height = 10});
    buf.draw_box(1, 1, 5, 5, BorderStyle::Single, RGBA{1, 0, 0, 1});
    EXPECT_TRUE(buf.get_cell(1, 1).fg.approx_equal(RGBA{1, 0, 0, 1}));
}

TEST(OptimizedBufferTest, cells_span) {
    OptimizedBuffer buf(BufferOptions{.width = 3, .height = 2});
    EXPECT_EQ(buf.cells().size(), 6);
}

TEST(OptimizedBufferTest, move_constructor) {
    OptimizedBuffer buf(BufferOptions{.width = 10, .height = 10});
    buf.set_cell(5, 5, Cell{U'A'});
    OptimizedBuffer moved(std::move(buf));
    EXPECT_EQ(moved.width(), 10);
    EXPECT_EQ(moved.get_cell(5, 5).ch, U'A');
}

TEST(OptimizedBufferTest, set_cell_respects_opacity) {
    OptimizedBuffer buf(BufferOptions{.width = 10, .height = 10});
    buf.push_opacity(0.5f);
    buf.set_cell(0, 0, Cell{U'X', RGBA{1, 0, 0, 1}, RGBA{0, 0, 1, 1}});
    EXPECT_EQ(buf.get_cell(0, 0).ch, U'X');
    EXPECT_FLOAT_EQ(buf.get_cell(0, 0).fg.a, 0.5f);
}
