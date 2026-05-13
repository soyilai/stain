#include "stain/keybind.hpp"
#include <gtest/gtest.h>

using namespace stain;

TEST(KeybindTest, merge_bindings_defaults_only) {
    std::vector<KeyBinding> defaults = {
        {"a", "action-a"},
        {"b", "action-b"},
    };
    auto result = merge_bindings(defaults, {});
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].key, "a");
    EXPECT_EQ(result[0].action, "action-a");
    EXPECT_EQ(result[1].key, "b");
    EXPECT_EQ(result[1].action, "action-b");
}

TEST(KeybindTest, merge_bindings_override) {
    std::vector<KeyBinding> defaults = {
        {"a", "action-a"},
        {"b", "action-b"},
    };
    std::vector<KeyBinding> overrides = {
        {"a", "custom-a"},
    };
    auto result = merge_bindings(defaults, overrides);
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].action, "custom-a");
    EXPECT_EQ(result[1].action, "action-b");
}

TEST(KeybindTest, merge_bindings_new_keys) {
    std::vector<KeyBinding> defaults = {
        {"a", "action-a"},
    };
    std::vector<KeyBinding> overrides = {
        {"c", "action-c"},
    };
    auto result = merge_bindings(defaults, overrides);
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].key, "a");
    EXPECT_EQ(result[1].key, "c");
}

TEST(KeybindTest, merge_bindings_preserves_order) {
    std::vector<KeyBinding> defaults = {
        {"a", "act-a"},
        {"b", "act-b"},
    };
    std::vector<KeyBinding> overrides = {
        {"c", "act-c"},
        {"a", "override-a"},
    };
    auto result = merge_bindings(defaults, overrides);
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0].key, "a");
    EXPECT_EQ(result[1].key, "b");
    EXPECT_EQ(result[2].key, "c");
}

TEST(KeybindTest, build_binding_map) {
    std::vector<KeyBinding> bindings = {
        {"ctrl+a", "line-home"},
        {"ctrl+e", "line-end"},
        {"return", "new-line"},
    };
    auto map = build_binding_map(bindings);
    EXPECT_EQ(map["ctrl+a"], "line-home");
    EXPECT_EQ(map["ctrl+e"], "line-end");
    EXPECT_EQ(map["return"], "new-line");
    EXPECT_EQ(map.size(), 3);
}

TEST(KeybindTest, build_binding_map_with_aliases) {
    std::vector<KeyBinding> bindings = {
        {"enter", "new-line"},
    };
    KeyAliasMap aliases = {{"enter", "return"}};
    auto map = build_binding_map(bindings, aliases);
    EXPECT_EQ(map["return"], "new-line");
    EXPECT_EQ(map.find("enter"), map.end());
}

TEST(KeybindTest, key_binding_key_no_modifiers) {
    KeyEvent event;
    event.name = "a";
    EXPECT_EQ(key_binding_key(event), "a");
}

TEST(KeybindTest, key_binding_key_ctrl) {
    KeyEvent event;
    event.name = "a";
    event.ctrl = true;
    EXPECT_EQ(key_binding_key(event), "ctrl+a");
}

TEST(KeybindTest, key_binding_key_meta) {
    KeyEvent event;
    event.name = "a";
    event.meta = true;
    EXPECT_EQ(key_binding_key(event), "meta+a");
}

TEST(KeybindTest, key_binding_key_shift) {
    KeyEvent event;
    event.name = "a";
    event.shift = true;
    EXPECT_EQ(key_binding_key(event), "shift+a");
}

TEST(KeybindTest, key_binding_key_all_modifiers) {
    KeyEvent event;
    event.name = "f1";
    event.ctrl = true;
    event.meta = true;
    event.shift = true;
    event.super_ = true;
    EXPECT_EQ(key_binding_key(event), "ctrl+meta+shift+super+f1");
}

TEST(KeybindTest, key_binding_key_special_keys) {
    KeyEvent event;
    event.name = "return";
    EXPECT_EQ(key_binding_key(event), "return");
    event.name = "up";
    EXPECT_EQ(key_binding_key(event), "up");
}

TEST(KeybindTest, key_binding_key_modifier_order) {
    KeyEvent event;
    event.name = "x";
    event.shift = true;
    event.ctrl = true;
    EXPECT_EQ(key_binding_key(event), "ctrl+shift+x");
}
