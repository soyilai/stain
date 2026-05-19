#include "stain/box.hpp"

#include <yoga/Yoga.h>

namespace stain {

    namespace {

        int display_width(std::string_view s) {
            std::size_t pos = 0;
            int w = 0;
            while(pos < s.size()) {
                char32_t cp = decode_utf8(s, pos);
                if(cp == U'\0')
                    break;
                w += char_display_width(cp);
            }
            return w;
        }

        std::string_view truncate_by_width(std::string_view s, int max_w) {
            if(max_w <= 0)
                return s.substr(0, 0);
            std::size_t pos = 0;
            std::size_t last_pos = 0;
            int w = 0;
            while(pos < s.size()) {
                std::size_t cp_start = pos;
                char32_t cp = decode_utf8(s, pos);
                if(cp == U'\0')
                    break;
                int cw = char_display_width(cp);
                if(w + cw > max_w)
                    break;
                w += cw;
                last_pos = pos;
            }
            return s.substr(0, last_pos);
        }

    } // anonymous namespace

    static void set_yoga_border(YGNodeRef node, YGEdge edge, bool active) {
        YGNodeStyleSetBorder(node, edge, active ? 1.f : 0.f);
    }

    void Box::sync_border_to_yoga() {
        const auto& sty = _box_opts.style;
        bool has_border = sty.border_style != BorderStyle::None;

        set_yoga_border(yoga_node(), YGEdgeTop,
                        has_border && sty.border_sides.top);
        set_yoga_border(yoga_node(), YGEdgeBottom,
                        has_border && sty.border_sides.bottom);
        set_yoga_border(yoga_node(), YGEdgeLeft,
                        has_border && sty.border_sides.left);
        set_yoga_border(yoga_node(), YGEdgeRight,
                        has_border && sty.border_sides.right);
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
        _box_opts.title_align = align;
        if(pos == BoxOptions::TitlePos::Bottom) {
            _box_opts.title_bottom = std::string(text);
        } else {
            _box_opts.title = std::string(text);
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

            auto draw_title = [&](std::string_view title, int y_pos) {
                int max_title_w = w - 4;
                std::string_view title_view = title;
                int title_dw = display_width(title_view);
                if(title_dw > max_title_w) {
                    title_view = truncate_by_width(title_view, max_title_w);
                    title_dw = max_title_w;
                }

                int title_x = sx + 2;
                if(_box_opts.title_align == BoxOptions::TitleAlign::Center)
                    title_x = sx + (w - title_dw) / 2;
                else if(_box_opts.title_align == BoxOptions::TitleAlign::Right)
                    title_x = sx + w - title_dw - 2;

                buf.draw_text(
                    title_x,
                    y_pos,
                    title_view,
                    bc,
                    sty.background_color
                );
            };

            // Draw title on top border.
            if(!_box_opts.title.empty() && sty.border_sides.top && w > 4) {
                draw_title(_box_opts.title, sy);
            }

            // Draw bottom title.
            if(!_box_opts.title_bottom.empty() && sty.border_sides.bottom &&
               w > 4) {
                draw_title(_box_opts.title_bottom, sy + h - 1);
            }
        }
    }

} // namespace stain
