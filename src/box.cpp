#include "stain/box.hpp"

#include <yoga/Yoga.h>

namespace stain {

    static void set_yoga_border(
        YGNodeRef node,
        YGEdge edge,
        bool active,
        bool pad_is_auto
    ) {
        if(active && pad_is_auto)
            YGNodeStyleSetBorder(node, edge, 1.f);
        else
            YGNodeStyleSetBorder(node, edge, 0.f);
    }

    static bool is_pad_auto(const Edges& pad, YGEdge edge) {
        switch(edge) {
        case YGEdgeTop:
            return std::holds_alternative<Auto>(pad.top);
        case YGEdgeBottom:
            return std::holds_alternative<Auto>(pad.bottom);
        case YGEdgeLeft:
            return std::holds_alternative<Auto>(pad.left);
        case YGEdgeRight:
            return std::holds_alternative<Auto>(pad.right);
        default:
            return true;
        }
    }

    void Box::sync_border_to_yoga() {
        const auto& sty = _box_opts.style;
        const auto& pad = _opts.padding;
        bool has_border = sty.border_style != BorderStyle::None;

        set_yoga_border(
            yoga_node(),
            YGEdgeTop,
            has_border && sty.border_sides.top,
            is_pad_auto(pad, YGEdgeTop)
        );
        set_yoga_border(
            yoga_node(),
            YGEdgeBottom,
            has_border && sty.border_sides.bottom,
            is_pad_auto(pad, YGEdgeBottom)
        );
        set_yoga_border(
            yoga_node(),
            YGEdgeLeft,
            has_border && sty.border_sides.left,
            is_pad_auto(pad, YGEdgeLeft)
        );
        set_yoga_border(
            yoga_node(),
            YGEdgeRight,
            has_border && sty.border_sides.right,
            is_pad_auto(pad, YGEdgeRight)
        );
    }

    Box::Box(RenderContext* ctx, BoxOptions opts)
        : Renderable(ctx, static_cast<RenderableOptions&>(opts))
        , _box_opts(std::move(opts)) {
        sync_border_to_yoga();
    }

    Box& Box::style(BoxStyle s) {
        _box_opts.style = s;
        sync_border_to_yoga();
        request_render();
        return *this;
    }

    Box& Box::focus_style(BoxFocusStyle s) {
        _box_opts.focus_style = s;
        request_render();
        return *this;
    }

    Box& Box::border(BorderStyle style, BorderSides sides) {
        _box_opts.style.border_style = style;
        _box_opts.style.border_sides = sides;
        sync_border_to_yoga();
        request_render();
        return *this;
    }

    Box& Box::border_style(BorderStyle s) {
        _box_opts.style.border_style = s;
        sync_border_to_yoga();
        request_render();
        return *this;
    }

    Box& Box::border_sides(BorderSides s) {
        _box_opts.style.border_sides = s;
        sync_border_to_yoga();
        request_render();
        return *this;
    }

    Box& Box::title(
        std::string_view text,
        BoxOptions::TitlePos pos,
        BoxOptions::TitleAlign align
    ) {
        if(pos == BoxOptions::TitlePos::Bottom) {
            _box_opts.title_bottom = std::string(text);
        } else {
            _box_opts.title = std::string(text);
            _box_opts.title_align = align;
        }
        request_render();
        return *this;
    }

    void Box::draw(OptimizedBuffer& buf, double /*delta*/) {
        int sx = screen_x();
        int sy = screen_y();
        int w = layout_w();
        int h = layout_h();

        if(w <= 0 || h <= 0)
            return;

        const auto& sty = _box_opts.style;

        // Fill background.
        if(!sty.background_color.is_transparent()) {
            buf.fill_rect(sx, sy, w, h, sty.background_color);
        }

        // Draw border.
        if(sty.border_style != BorderStyle::None && w >= 2 && h >= 2) {
            RGBA bc = focused() ? _box_opts.focus_style.border_color
                                : sty.border_color;
            buf.draw_box(
                sx,
                sy,
                w,
                h,
                sty.border_style,
                bc,
                sty.background_color,
                sty.border_sides
            );

            // Draw title on top border.
            if(!_box_opts.title.empty() && sty.border_sides.top && w > 4) {
                int max_title_len = w - 4;
                std::string_view title_view = _box_opts.title;
                if(static_cast<int>(title_view.size()) > max_title_len)
                    title_view = title_view.substr(
                        0,
                        static_cast<std::size_t>(max_title_len)
                    );

                int title_x = sx + 2;
                if(_box_opts.title_align == BoxOptions::TitleAlign::Center)
                    title_x =
                        sx + (w - static_cast<int>(title_view.size())) / 2;
                else if(_box_opts.title_align == BoxOptions::TitleAlign::Right)
                    title_x = sx + w - static_cast<int>(title_view.size()) - 2;

                buf.draw_text(
                    title_x,
                    sy,
                    title_view,
                    bc,
                    sty.background_color
                );
            }

            // Draw bottom title.
            if(!_box_opts.title_bottom.empty() && sty.border_sides.bottom &&
               w > 4) {
                int max_title_len = w - 4;
                std::string_view title_view = _box_opts.title_bottom;
                if(static_cast<int>(title_view.size()) > max_title_len)
                    title_view = title_view.substr(
                        0,
                        static_cast<std::size_t>(max_title_len)
                    );

                buf.draw_text(
                    sx + 2,
                    sy + h - 1,
                    title_view,
                    bc,
                    sty.background_color
                );
            }
        }
    }

} // namespace stain
