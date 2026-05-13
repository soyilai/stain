#include "stain/color.hpp"
#include <gtest/gtest.h>

using namespace stain;

TEST(ColorTest, RGBA_defaults) {
    RGBA c;
    EXPECT_FLOAT_EQ(c.r, 0);
    EXPECT_FLOAT_EQ(c.g, 0);
    EXPECT_FLOAT_EQ(c.b, 0);
    EXPECT_FLOAT_EQ(c.a, 1);
}

TEST(ColorTest, RGBA_white_black_transparent) {
    EXPECT_TRUE(RGBA::white().approx_equal(RGBA{1, 1, 1, 1}));
    EXPECT_TRUE(RGBA::black().approx_equal(RGBA{0, 0, 0, 1}));
    EXPECT_TRUE(RGBA::transparent().approx_equal(RGBA{0, 0, 0, 0}));
}

TEST(ColorTest, from_hex_6digit) {
    RGBA c = RGBA::from_hex("#FF0000");
    EXPECT_FLOAT_EQ(c.r, 1);
    EXPECT_FLOAT_EQ(c.g, 0);
    EXPECT_FLOAT_EQ(c.b, 0);
    EXPECT_FLOAT_EQ(c.a, 1);
}

TEST(ColorTest, from_hex_8digit) {
    RGBA c = RGBA::from_hex("#FF000080");
    EXPECT_FLOAT_EQ(c.r, 1);
    EXPECT_FLOAT_EQ(c.g, 0);
    EXPECT_FLOAT_EQ(c.b, 0);
    EXPECT_FLOAT_EQ(c.a, 128.0f / 255.0f);
}

TEST(ColorTest, from_hex_green) {
    RGBA c = RGBA::from_hex("#00FF00");
    EXPECT_FLOAT_EQ(c.r, 0);
    EXPECT_FLOAT_EQ(c.g, 1);
    EXPECT_FLOAT_EQ(c.b, 0);
    EXPECT_FLOAT_EQ(c.a, 1);
}

TEST(ColorTest, from_hex_blue) {
    RGBA c = RGBA::from_hex("#0000FF");
    EXPECT_FLOAT_EQ(c.r, 0);
    EXPECT_FLOAT_EQ(c.g, 0);
    EXPECT_FLOAT_EQ(c.b, 1);
    EXPECT_FLOAT_EQ(c.a, 1);
}

TEST(ColorTest, from_hex_missing_hash_throws) {
    EXPECT_THROW(RGBA::from_hex("FF0000"), std::invalid_argument);
}

TEST(ColorTest, from_hex_wrong_length_throws) {
    EXPECT_THROW(RGBA::from_hex("#FFF"), std::invalid_argument);
}

TEST(ColorTest, from_hex_invalid_char_throws) {
    EXPECT_THROW(RGBA::from_hex("#FF00ZZ"), std::invalid_argument);
}

TEST(ColorTest, from_name_white) {
    RGBA c = RGBA::from_name("white");
    EXPECT_TRUE(c.approx_equal(RGBA{1, 1, 1, 1}));
}

TEST(ColorTest, from_name_red) {
    RGBA c = RGBA::from_name("red");
    EXPECT_TRUE(c.approx_equal(RGBA{1, 0, 0, 1}));
}

TEST(ColorTest, from_name_transparent) {
    RGBA c = RGBA::from_name("transparent");
    EXPECT_TRUE(c.approx_equal(RGBA{0, 0, 0, 0}));
}

TEST(ColorTest, from_name_case_insensitive) {
    RGBA c = RGBA::from_name("RED");
    EXPECT_TRUE(c.approx_equal(RGBA{1, 0, 0, 1}));
}

TEST(ColorTest, from_name_unknown_throws) {
    EXPECT_THROW(RGBA::from_name("notacolor"), std::invalid_argument);
}

TEST(ColorTest, from_name_black) {
    RGBA c = RGBA::from_name("black");
    EXPECT_TRUE(c.approx_equal(RGBA{0, 0, 0, 1}));
}

TEST(ColorTest, from_name_navy) {
    RGBA c = RGBA::from_name("navy");
    EXPECT_TRUE(c.approx_equal(RGBA{0, 0, 0.5f, 1}));
}

TEST(ColorTest, resolve_color_rgba) {
    RGBA input{0.5f, 0.5f, 0.5f, 1};
    RGBA result = resolve_color(input);
    EXPECT_TRUE(result.approx_equal(input));
}

TEST(ColorTest, resolve_color_hex_string) {
    RGBA result = resolve_color(std::string("#FF0000"));
    EXPECT_TRUE(result.approx_equal(RGBA{1, 0, 0, 1}));
}

TEST(ColorTest, resolve_color_name_string) {
    RGBA result = resolve_color(std::string("lime"));
    EXPECT_TRUE(result.approx_equal(RGBA{0, 1, 0, 1}));
}

TEST(ColorTest, blend_over_opaque_foreground) {
    RGBA fg{1, 0, 0, 1};
    RGBA bg{0, 1, 0, 1};
    RGBA result = fg.blend_over(bg);
    EXPECT_TRUE(result.approx_equal(fg));
}

TEST(ColorTest, blend_over_transparent_foreground) {
    RGBA fg{0, 0, 0, 0};
    RGBA bg{0, 0, 1, 1};
    RGBA result = fg.blend_over(bg);
    EXPECT_TRUE(result.approx_equal(bg));
}

TEST(ColorTest, blend_over_semi_transparent) {
    RGBA fg{1, 0, 0, 0.5f};
    RGBA bg{0, 0, 1, 1};
    RGBA result = fg.blend_over(bg);
    // out_a = 0.5 + 1.0 * 0.5 = 1.0
    // r = (1.0 * 0.5 + 0.0 * 1.0 * 0.5) / 1.0 = 0.5
    // b = (0.0 * 0.5 + 1.0 * 1.0 * 0.5) / 1.0 = 0.5
    EXPECT_NEAR(result.r, 0.5f, 1e-5f);
    EXPECT_NEAR(result.g, 0.0f, 1e-5f);
    EXPECT_NEAR(result.b, 0.5f, 1e-5f);
    EXPECT_NEAR(result.a, 1.0f, 1e-5f);
}

TEST(ColorTest, approx_equal_precise) {
    RGBA a{0.5f, 0.5f, 0.5f, 0.5f};
    RGBA b{0.5f, 0.5f, 0.5f, 0.5f};
    EXPECT_TRUE(a.approx_equal(b));
}

TEST(ColorTest, approx_equal_different) {
    RGBA a{0.5f, 0.5f, 0.5f, 0.5f};
    RGBA b{0.6f, 0.5f, 0.5f, 0.5f};
    EXPECT_FALSE(a.approx_equal(b));
}

TEST(ColorTest, is_transparent) {
    EXPECT_TRUE(RGBA::transparent().is_transparent());
    EXPECT_FALSE(RGBA::white().is_transparent());
}

TEST(ColorTest, to_8bit) {
    RGBA c{1, 0.5f, 0, 1};
    EXPECT_EQ(c.r8(), 255);
    EXPECT_EQ(c.g8(), 128);
    EXPECT_EQ(c.b8(), 0);
    EXPECT_EQ(c.a8(), 255);
}

TEST(ColorTest, equality) {
    EXPECT_TRUE((RGBA{1, 1, 1, 1} == RGBA{1, 1, 1, 1}));
    EXPECT_FALSE((RGBA{1, 0, 0, 1} == RGBA{0, 1, 0, 1}));
}

TEST(ColorTest, all_named_colors_roundtrip) {
    struct {
        const char* name;
        RGBA color;
    } checks[] = {
        {"aqua", {0, 1, 1, 1}},        {"black", {0, 0, 0, 1}},
        {"blue", {0, 0, 1, 1}},        {"cyan", {0, 1, 1, 1}},
        {"fuchsia", {1, 0, 1, 1}},     {"gray", {0.5f, 0.5f, 0.5f, 1}},
        {"green", {0, 0.5f, 0, 1}},    {"lime", {0, 1, 0, 1}},
        {"magenta", {1, 0, 1, 1}},     {"maroon", {0.5f, 0, 0, 1}},
        {"navy", {0, 0, 0.5f, 1}},     {"olive", {0.5f, 0.5f, 0, 1}},
        {"orange", {1, 0.647f, 0, 1}}, {"purple", {0.5f, 0, 0.5f, 1}},
        {"red", {1, 0, 0, 1}},         {"silver", {0.75f, 0.75f, 0.75f, 1}},
        {"teal", {0, 0.5f, 0.5f, 1}},  {"white", {1, 1, 1, 1}},
        {"yellow", {1, 1, 0, 1}},
    };
    for(auto& c : checks) {
        RGBA result = RGBA::from_name(c.name);
        EXPECT_TRUE(result.approx_equal(c.color, 1e-3f))
            << "named color '" << c.name << "' mismatch";
    }
}

TEST(ColorTest, blend_over_same_color) {
    RGBA c{0.5f, 0.5f, 0.5f, 0.8f};
    RGBA result = c.blend_over(c);
    EXPECT_FLOAT_EQ(result.r, 0.5f);
    EXPECT_FLOAT_EQ(result.g, 0.5f);
    EXPECT_FLOAT_EQ(result.b, 0.5f);
}
