#pragma once

#include "stain/renderable.hpp"

namespace stain {

    // Background color, border style, sides, and border color.
    struct BoxStyle {
        RGBA background_color{RGBA::transparent()};
        BorderStyle border_style{BorderStyle::None};
        BorderSides border_sides{BorderSides::all()};
        RGBA border_color{RGBA::white()};
    };

    // Border color override when the box is focused.
    struct BoxFocusStyle {
        RGBA border_color{RGBA{0, 0.67f, 1, 1}};
    };

    struct BoxOptions : RenderableOptions {
        BoxStyle style{};
        BoxFocusStyle focus_style{};
        std::string title{};
        std::string title_bottom{};
        // Title text alignment on the top border.
        enum class TitleAlign {
            Left,
            Center,
            Right
        } title_align{TitleAlign::Left};
        // Title position: top or bottom border.
        enum class TitlePos { Top, Bottom };
    };

    // A bordered container with an optional title.
    // Usage:
    //   auto b = box(ctx);
    //   b->style({.background_color = RGBA::black(), .border_style =
    //   BorderStyle::Single}); b->title("my box");
    class Box : public Renderable {
        public:
        explicit Box(RenderContext* ctx, BoxOptions opts = {});

        // Create a Box with default options.
        [[nodiscard]] static std::shared_ptr<Box> create(RenderContext* ctx) {
            return std::make_shared<Box>(ctx);
        }

        Box& style(BoxStyle s);
        Box& focus_style(BoxFocusStyle s);
        // Set border style and which sides are drawn.
        Box& border(BorderStyle style, BorderSides sides = BorderSides::all());
        Box& border_style(BorderStyle s);
        Box& border_sides(BorderSides s);
        // Set title text. Position and alignment are set through options.
        Box& title(
            std::string_view text,
            BoxOptions::TitlePos pos = BoxOptions::TitlePos::Top,
            BoxOptions::TitleAlign align = BoxOptions::TitleAlign::Left
        );

        protected:
        void draw(OptimizedBuffer& buf, double delta) override;

        private:
        void sync_border_to_yoga();
        BoxOptions _box_opts;
    };

    // Factory function for a Box.
    inline std::shared_ptr<Box> box(RenderContext& ctx) {
        return Box::create(&ctx);
    }

} // namespace stain
