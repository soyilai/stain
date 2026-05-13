#include "stain/input.hpp"
#include <gtest/gtest.h>

using namespace stain;

class EditBufferTest : public ::testing::Test {
    protected:
    EditBuffer buf;
};

TEST_F(EditBufferTest, initially_empty) {
    EXPECT_EQ(buf.plain_text(), "");
    EXPECT_EQ(buf.length(), 0);
    EXPECT_EQ(buf.cursor_offset(), 0);
}

TEST_F(EditBufferTest, set_text) {
    buf.text("hello");
    EXPECT_EQ(buf.plain_text(), "hello");
    EXPECT_EQ(buf.length(), 5);
}

TEST_F(EditBufferTest, insert_at_cursor) {
    buf.text("helo");
    buf.move_cursor_to(3);
    buf.insert_at_cursor("l");
    EXPECT_EQ(buf.plain_text(), "hello");
    EXPECT_EQ(buf.cursor_offset(), 4);
}

TEST_F(EditBufferTest, insert_in_middle) {
    buf.text("hllo");
    buf.move_cursor_to(1);
    buf.insert_at_cursor("e");
    EXPECT_EQ(buf.plain_text(), "hello");
}

TEST_F(EditBufferTest, delete_char_backward) {
    buf.text("hello");
    buf.move_cursor_to(5);
    buf.delete_char_backward();
    EXPECT_EQ(buf.plain_text(), "hell");
    EXPECT_EQ(buf.cursor_offset(), 4);
}

TEST_F(EditBufferTest, delete_char_forward) {
    buf.text("hello");
    buf.move_cursor_to(0);
    buf.delete_char_forward();
    EXPECT_EQ(buf.plain_text(), "ello");
    EXPECT_EQ(buf.cursor_offset(), 0);
}

TEST_F(EditBufferTest, delete_char_backward_at_start) {
    buf.text("hello");
    buf.move_cursor_to(0);
    buf.delete_char_backward();
    EXPECT_EQ(buf.plain_text(), "hello");
    EXPECT_EQ(buf.cursor_offset(), 0);
}

TEST_F(EditBufferTest, delete_char_forward_at_end) {
    buf.text("hello");
    buf.move_cursor_to(5);
    buf.delete_char_forward();
    EXPECT_EQ(buf.plain_text(), "hello");
    EXPECT_EQ(buf.cursor_offset(), 5);
}

TEST_F(EditBufferTest, delete_word_backward) {
    buf.text("hello world");
    buf.move_cursor_to(11);
    buf.delete_word_backward();
    EXPECT_EQ(buf.plain_text(), "hello ");
}

TEST_F(EditBufferTest, delete_to_line_end) {
    buf.text("hello\nworld");
    buf.move_cursor_to(0);
    buf.delete_to_line_end();
    EXPECT_EQ(buf.plain_text(), "\nworld");
}

TEST_F(EditBufferTest, move_cursor_left) {
    buf.text("hello");
    buf.move_cursor_to(5);
    buf.move_cursor_left();
    EXPECT_EQ(buf.cursor_offset(), 4);
    EXPECT_EQ(buf.plain_text(), "hello");
}

TEST_F(EditBufferTest, move_cursor_left_at_start) {
    buf.text("hello");
    buf.move_cursor_to(0);
    buf.move_cursor_left();
    EXPECT_EQ(buf.cursor_offset(), 0);
}

TEST_F(EditBufferTest, move_cursor_right) {
    buf.text("hello");
    buf.move_cursor_to(0);
    buf.move_cursor_right();
    EXPECT_EQ(buf.cursor_offset(), 1);
}

TEST_F(EditBufferTest, move_cursor_right_at_end) {
    buf.text("hello");
    buf.move_cursor_to(5);
    buf.move_cursor_right();
    EXPECT_EQ(buf.cursor_offset(), 5);
}

TEST_F(EditBufferTest, move_cursor) {
    buf.text("hello");
    buf.move_cursor(2);
    EXPECT_EQ(buf.cursor_offset(), 2);
    buf.move_cursor(-1);
    EXPECT_EQ(buf.cursor_offset(), 1);
}

TEST_F(EditBufferTest, move_cursor_clamped) {
    buf.text("hi");
    buf.move_cursor(-10);
    EXPECT_EQ(buf.cursor_offset(), 0);
    buf.move_cursor(10);
    EXPECT_EQ(buf.cursor_offset(), 2);
}

TEST_F(EditBufferTest, move_line_start) {
    buf.text("abc\ndef");
    buf.move_cursor_to(6);
    buf.move_line_start();
    EXPECT_EQ(buf.cursor_offset(), 4);
}

TEST_F(EditBufferTest, move_line_end) {
    buf.text("abc\ndef");
    buf.move_cursor_to(2);
    buf.move_line_end();
    EXPECT_EQ(buf.cursor_offset(), 3);
}

TEST_F(EditBufferTest, move_word_forward) {
    buf.text("hello world foo");
    buf.move_cursor_to(0);
    buf.move_word_forward();
    EXPECT_EQ(buf.cursor_offset(), 6);
}

TEST_F(EditBufferTest, move_word_backward) {
    buf.text("hello world foo");
    buf.move_cursor_to(17);
    buf.move_word_backward();
    EXPECT_EQ(buf.cursor_offset(), 12);
}

TEST_F(EditBufferTest, move_line_up) {
    buf.text("abc\ndef\nghi");
    buf.move_cursor_to(8);
    buf.move_line_up();
    EXPECT_EQ(buf.cursor_offset(), 4);
}

TEST_F(EditBufferTest, move_line_down) {
    buf.text("abc\ndef\nghi");
    buf.move_cursor_to(4);
    buf.move_line_down();
    EXPECT_EQ(buf.cursor_offset(), 8);
}

TEST_F(EditBufferTest, move_line_up_at_first_line) {
    buf.text("abc\ndef");
    buf.move_cursor_to(1);
    buf.move_line_up();
    EXPECT_EQ(buf.cursor_offset(), 0);
}

TEST_F(EditBufferTest, move_line_down_at_last_line) {
    buf.text("abc\ndef");
    buf.move_cursor_to(6);
    buf.move_line_down();
    EXPECT_EQ(buf.cursor_offset(), 7);
}

TEST_F(EditBufferTest, cursor_column) {
    buf.text("abc\ndefg");
    buf.move_cursor_to(6);
    EXPECT_EQ(buf.cursor_column(), 2);
}

TEST_F(EditBufferTest, undo) {
    buf.text("hello");
    buf.move_cursor_to(5);
    buf.insert_at_cursor(" world");
    EXPECT_EQ(buf.plain_text(), "hello world");
    buf.undo();
    EXPECT_EQ(buf.plain_text(), "hello");
}

TEST_F(EditBufferTest, undo_multiple) {
    buf.text("a");
    buf.move_cursor_to(1);
    buf.insert_at_cursor("b");
    buf.insert_at_cursor("c");
    EXPECT_EQ(buf.plain_text(), "abc");
    buf.undo();
    EXPECT_EQ(buf.plain_text(), "ab");
    buf.undo();
    EXPECT_EQ(buf.plain_text(), "a");
}

TEST_F(EditBufferTest, redo) {
    buf.text("hello");
    buf.move_cursor_to(5);
    buf.insert_at_cursor(" world");
    buf.undo();
    EXPECT_EQ(buf.plain_text(), "hello");
    buf.redo();
    EXPECT_EQ(buf.plain_text(), "hello world");
}

TEST_F(EditBufferTest, undo_clears_redo_on_new_action) {
    buf.text("hello");
    buf.insert_at_cursor(" world");
    buf.undo();
    buf.text("goodbye");
    buf.redo();
    EXPECT_NE(buf.plain_text(), "hello world");
    EXPECT_EQ(buf.plain_text(), "goodbye");
}

TEST_F(EditBufferTest, selection) {
    buf.text("hello world");
    buf.select(0, 5);
    EXPECT_TRUE(buf.has_selection());
    EXPECT_EQ(buf.selected_text(), "hello");
}

TEST_F(EditBufferTest, no_selection_initially) {
    EXPECT_FALSE(buf.has_selection());
}

TEST_F(EditBufferTest, clear_selection) {
    buf.text("hello world");
    buf.select(0, 5);
    buf.clear_selection();
    EXPECT_FALSE(buf.has_selection());
}

TEST_F(EditBufferTest, delete_selection) {
    buf.text("hello world");
    buf.select(0, 5);
    buf.delete_selection();
    EXPECT_EQ(buf.plain_text(), " world");
    EXPECT_EQ(buf.cursor_offset(), 0);
}

TEST_F(EditBufferTest, insert_at_cursor_replaces_selection) {
    buf.text("hello world");
    buf.select(6, 11);
    buf.insert_at_cursor("there");
    EXPECT_EQ(buf.plain_text(), "hello there");
}

TEST_F(EditBufferTest, delete_backward_with_selection) {
    buf.text("hello world");
    buf.select(0, 5);
    buf.delete_char_backward();
    EXPECT_EQ(buf.plain_text(), " world");
}

TEST_F(EditBufferTest, delete_forward_with_selection) {
    buf.text("hello world");
    buf.select(6, 11);
    buf.delete_char_forward();
    EXPECT_EQ(buf.plain_text(), "hello ");
}

TEST_F(EditBufferTest, cursor_clamped_after_set_text) {
    buf.text("hello");
    buf.move_cursor_to(10);
    EXPECT_EQ(buf.cursor_offset(), 5);
    buf.text("hi");
    EXPECT_EQ(buf.cursor_offset(), 2);
}

TEST_F(EditBufferTest, insert_utf8) {
    buf.text("hel");
    buf.move_cursor_to(3);
    buf.insert_at_cursor("lo");
    EXPECT_EQ(buf.plain_text(), "hello");
    EXPECT_EQ(buf.cursor_offset(), 5);
}

TEST_F(EditBufferTest, move_cursor_utf8) {
    buf.text("héllo");
    buf.move_cursor_to(5);
    // cursor at 5 (after 'o'), move left past 'o', 'l', 'l' to byte 1 (after
    // 'h', before 'é')
    buf.move_cursor_left();
    buf.move_cursor_left();
    buf.move_cursor_left();
    // now cursor is at the start of 'é' (byte 1). deleting backward removes
    // 'h', leaving "éllo"
    buf.delete_char_backward();
    EXPECT_EQ(buf.plain_text(), "éllo");
}
