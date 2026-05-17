#include "stain/scroll.hpp"

#include <algorithm>
#include <yoga/Yoga.h>

namespace stain {

    ScrollBox::ScrollBox(RenderContext* ctx, ScrollBoxOptions opts)
        : Renderable(ctx, static_cast<RenderableOptions&>(opts.root_options))
        , _scroll_opts(std::move(opts)) {

        _focusable = true;

        // Create viewport (overflow hidden). Must be Column so content
        // stretches to width.
        auto vp_opts = _scroll_opts.viewport_options;
        vp_opts.flex_direction = FlexDirection::Column;
        vp_opts.flex_grow = 1.f;
        vp_opts.overflow = Overflow::Hidden;
        _viewport = std::make_shared<Box>(ctx, vp_opts);

        // Create content container.
        auto ct_opts = _scroll_opts.content_options;
        ct_opts.flex_direction = FlexDirection::Column;
        ct_opts.flex_shrink = 0.f;
        _content = std::make_shared<Box>(ctx, ct_opts);

        _viewport->add(_content);
        Renderable::add(_viewport);
    }

    void ScrollBox::scroll_to(int top, int left) {
        _scroll_top = top;
        _scroll_left = left;
        clamp_scroll();
        _content->translate(-_scroll_left, -_scroll_top);
        request_render();
    }

    void ScrollBox::scroll_by(int dy, int dx) {
        scroll_to(_scroll_top + dy, _scroll_left + dx);
    }

    void ScrollBox::scroll_to_top() {
        scroll_to(0, _scroll_left);
    }
    void ScrollBox::scroll_to_bottom() {
        int max_scroll = std::max(0, content_height() - _viewport->layout_h());
        scroll_to(max_scroll, _scroll_left);
    }

    int ScrollBox::content_height() const noexcept {
        return _content->layout_h();
    }
    int ScrollBox::content_width() const noexcept {
        return _content->layout_w();
    }

    int ScrollBox::add(
        std::shared_ptr<BaseRenderable> child,
        std::optional<int> index
    ) {
        return _content->add(std::move(child), index);
    }

    void ScrollBox::remove(std::string_view id) {
        _content->remove(id);
    }

    BaseRenderable* ScrollBox::find(std::string_view id) const {
        return Renderable::find(id);
    }

    void ScrollBox::draw(OptimizedBuffer& buf, double /*delta*/) {
        int vp_h = _viewport->layout_h();
        int vp_w = _viewport->layout_w();
        int vp_x = _viewport->screen_x();
        int vp_y = _viewport->screen_y();
        int c_h = content_height();

        // Vertical scrollbar indicator.
        if(_scroll_opts.scroll_y && c_h > vp_h && vp_w > 1 && vp_h > 2) {
            int sb_x = vp_x + vp_w - 1;
            float visible_ratio =
                static_cast<float>(vp_h) / static_cast<float>(c_h);
            float thumb_h =
                std::max(1.0f, visible_ratio * static_cast<float>(vp_h));
            float max_scroll = static_cast<float>(c_h - vp_h);
            float thumb_pos =
                max_scroll > 0
                    ? (static_cast<float>(_scroll_top) / max_scroll) *
                          static_cast<float>(vp_h - thumb_h)
                    : 0.0f;

            RGBA track_color{0.15f, 0.15f, 0.15f, 0.5f};
            RGBA thumb_color{0.5f, 0.5f, 0.5f, 0.8f};
            for(int row = 0; row < vp_h; row++) {
                bool is_thumb = row >= static_cast<int>(thumb_pos) &&
                                row < static_cast<int>(thumb_pos + thumb_h);
                buf.set_cell(
                    sb_x,
                    vp_y + row,
                    Cell{
                        U' ',
                        RGBA::white(),
                        is_thumb ? thumb_color : track_color,
                        Attr::None
                    }
                );
            }
        }

        // Horizontal scrollbar indicator.
        if(_scroll_opts.scroll_x && content_width() > vp_w && vp_h > 1 &&
           vp_w > 2) {
            int sb_y = vp_y + vp_h - 1;
            int c_w = content_width();
            float visible_ratio =
                static_cast<float>(vp_w) / static_cast<float>(c_w);
            float thumb_w =
                std::max(1.0f, visible_ratio * static_cast<float>(vp_w));
            float max_scroll = static_cast<float>(c_w - vp_w);
            float thumb_pos =
                max_scroll > 0
                    ? (static_cast<float>(_scroll_left) / max_scroll) *
                          static_cast<float>(vp_w - thumb_w)
                    : 0.0f;

            RGBA track_color{0.15f, 0.15f, 0.15f, 0.5f};
            RGBA thumb_color{0.5f, 0.5f, 0.5f, 0.8f};
            for(int col = 0; col < vp_w; col++) {
                bool is_thumb = col >= static_cast<int>(thumb_pos) &&
                                col < static_cast<int>(thumb_pos + thumb_w);
                buf.set_cell(
                    vp_x + col,
                    sb_y,
                    Cell{
                        U' ',
                        RGBA::white(),
                        is_thumb ? thumb_color : track_color,
                        Attr::None
                    }
                );
            }
        }
    }

    void ScrollBox::on_resize(int /*w*/, int /*h*/) {
        clamp_scroll();
        if(_scroll_opts.viewport_culling) {
            apply_viewport_culling();
        }
        _content->translate(-_scroll_left, -_scroll_top);
    }

    void ScrollBox::process_mouse_event(MouseEvent& event) {
        if(event.type == MouseEventType::Scroll) {
            if(_scroll_opts.scroll_y) {
                scroll_by(event.scroll_delta * 3, 0);
            } else if(_scroll_opts.scroll_x) {
                scroll_by(0, event.scroll_delta * 3);
            }
            event.stop_propagation();
        }
        Renderable::process_mouse_event(event);
    }

    bool ScrollBox::handle_key_press(KeyEvent& key) {
        int vp_h = _viewport->layout_h();
        int by = static_cast<int>(
            YGNodeLayoutGetBorder(_viewport->yoga_node(), YGEdgeTop)
        );
        int bb = static_cast<int>(
            YGNodeLayoutGetBorder(_viewport->yoga_node(), YGEdgeBottom)
        );
        int page_h = std::max(1, vp_h - by - bb);
        if(key.name == "up" && _scroll_opts.scroll_y) {
            scroll_by(-1, 0);
        } else if(key.name == "down" && _scroll_opts.scroll_y) {
            scroll_by(1, 0);
        } else if(key.name == "pageup" && _scroll_opts.scroll_y) {
            scroll_by(-page_h, 0);
        } else if(key.name == "pagedown" && _scroll_opts.scroll_y) {
            scroll_by(page_h, 0);
        } else if(key.name == "home") {
            scroll_to_top();
        } else if(key.name == "end") {
            scroll_to_bottom();
        } else if(key.name == "left" && _scroll_opts.scroll_x) {
            scroll_by(0, -1);
        } else if(key.name == "right" && _scroll_opts.scroll_x) {
            scroll_by(0, 1);
        } else {
            return false;
        }
        key.stop_propagation();
        return true;
    }

    ScrollBox& ScrollBox::scroll_x(bool v) {
        _scroll_opts.scroll_x = v;
        request_render();
        return *this;
    }
    ScrollBox& ScrollBox::scroll_y(bool v) {
        _scroll_opts.scroll_y = v;
        request_render();
        return *this;
    }
    ScrollBox& ScrollBox::viewport_culling(bool v) {
        _scroll_opts.viewport_culling = v;
        return *this;
    }
    ScrollBox& ScrollBox::sticky_scroll(bool v) {
        _scroll_opts.sticky_scroll = v;
        return *this;
    }
    ScrollBox& ScrollBox::sticky_edge(std::optional<StickyEdge> e) {
        _scroll_opts.sticky_edge = e;
        return *this;
    }

    void ScrollBox::clamp_scroll() {
        int vp_h = _viewport->layout_h();
        int vp_w = _viewport->layout_w();
        int by = static_cast<int>(
            YGNodeLayoutGetBorder(_viewport->yoga_node(), YGEdgeTop)
        );
        int bb = static_cast<int>(
            YGNodeLayoutGetBorder(_viewport->yoga_node(), YGEdgeBottom)
        );
        int bl = static_cast<int>(
            YGNodeLayoutGetBorder(_viewport->yoga_node(), YGEdgeLeft)
        );
        int br = static_cast<int>(
            YGNodeLayoutGetBorder(_viewport->yoga_node(), YGEdgeRight)
        );
        int inner_h = std::max(1, vp_h - by - bb);
        int inner_w = std::max(1, vp_w - bl - br);
        int max_y = std::max(0, content_height() - inner_h);
        int max_x = std::max(0, content_width() - inner_w);
        _scroll_top = std::clamp(_scroll_top, 0, max_y);
        _scroll_left = std::clamp(_scroll_left, 0, max_x);
    }

    void ScrollBox::update_from_layout() {
        Renderable::update_from_layout();
        if(_scroll_opts.viewport_culling) {
            apply_viewport_culling();
        }
        if(!_scroll_opts.sticky_scroll || !_scroll_opts.sticky_edge)
            return;

        int vp_h = _viewport->layout_h();
        int vp_w = _viewport->layout_w();
        int by = static_cast<int>(
            YGNodeLayoutGetBorder(_viewport->yoga_node(), YGEdgeTop)
        );
        int bb = static_cast<int>(
            YGNodeLayoutGetBorder(_viewport->yoga_node(), YGEdgeBottom)
        );
        int bl = static_cast<int>(
            YGNodeLayoutGetBorder(_viewport->yoga_node(), YGEdgeLeft)
        );
        int br = static_cast<int>(
            YGNodeLayoutGetBorder(_viewport->yoga_node(), YGEdgeRight)
        );
        int inner_h = std::max(1, vp_h - by - bb);
        int inner_w = std::max(1, vp_w - bl - br);

        bool changed = false;
        switch(*_scroll_opts.sticky_edge) {
            case StickyEdge::Bottom: {
                int max_y = std::max(0, content_height() - inner_h);
                if(_scroll_top != max_y) {
                    _scroll_top = max_y;
                    changed = true;
                }
                break;
            }
            case StickyEdge::Top:
                if(_scroll_top != 0) {
                    _scroll_top = 0;
                    changed = true;
                }
                break;
            case StickyEdge::Right: {
                int max_x = std::max(0, content_width() - inner_w);
                if(_scroll_left != max_x) {
                    _scroll_left = max_x;
                    changed = true;
                }
                break;
            }
            case StickyEdge::Left:
                if(_scroll_left != 0) {
                    _scroll_left = 0;
                    changed = true;
                }
                break;
        }
        if(changed) {
            _content->translate(-_scroll_left, -_scroll_top);
        }
    }

    void ScrollBox::apply_viewport_culling() {
        int vp_x = _viewport->screen_x();
        int vp_y = _viewport->screen_y();
        int vp_h = _viewport->layout_h();
        int vp_w = _viewport->layout_w();

        int by = static_cast<int>(
            YGNodeLayoutGetBorder(_viewport->yoga_node(), YGEdgeTop)
        );
        int bb = static_cast<int>(
            YGNodeLayoutGetBorder(_viewport->yoga_node(), YGEdgeBottom)
        );
        int bl = static_cast<int>(
            YGNodeLayoutGetBorder(_viewport->yoga_node(), YGEdgeLeft)
        );
        int br = static_cast<int>(
            YGNodeLayoutGetBorder(_viewport->yoga_node(), YGEdgeRight)
        );

        int inner_x = vp_x + bl;
        int inner_y = vp_y + by;
        int inner_w = std::max(1, vp_w - bl - br);
        int inner_h = std::max(1, vp_h - by - bb);

        for(auto* child : _content->children()) {
            auto* r = dynamic_cast<Renderable*>(child);
            if(!r)
                continue;
            int cx = r->screen_x();
            int cy = r->screen_y();
            int cw = r->layout_w();
            int ch = r->layout_h();
            bool visible = (cx + cw > inner_x && cx < inner_x + inner_w &&
                            cy + ch > inner_y && cy < inner_y + inner_h);
            r->visible(visible);
        }
    }

} // namespace stain
