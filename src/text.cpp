#include "stain/text.hpp"

#include <sys/wait.h>
#include <unistd.h>

namespace stain {

    // TextBuilder

    TextBuilder&
    TextBuilder::append(std::string_view text, RGBA fg, RGBA bg, Attr attr) {
        _chunks.push_back(TextChunk{std::string(text), fg, bg, attr});
        return *this;
    }

    TextBuilder& TextBuilder::bold(std::string_view text) {
        return append(text, RGBA::white(), RGBA::transparent(), Attr::Bold);
    }

    TextBuilder& TextBuilder::italic(std::string_view text) {
        return append(text, RGBA::white(), RGBA::transparent(), Attr::Italic);
    }

    TextBuilder& TextBuilder::fg(RGBA color, std::string_view text) {
        return append(text, color);
    }

    TextBuilder&
    TextBuilder::link(std::string_view url, std::string_view label) {
        TextChunk chunk;
        chunk.text = std::string(label);
        chunk.fg = RGBA{0.3f, 0.6f, 1.0f, 1.0f}; // link blue
        chunk.attr = Attr::Underline;
        chunk.link_url = std::string(url);
        _chunks.push_back(std::move(chunk));
        return *this;
    }

    // Text

    Text::Text(RenderContext* ctx, TextOptions opts)
        : Renderable(ctx, static_cast<RenderableOptions&>(opts))
        , _text_opts(std::move(opts))
        , _text(_text_opts.content) {
    }

    Text& Text::content(StyledText text) {
        _text = std::move(text);
        request_render();
        return *this;
    }

    Text& Text::content(std::string_view plain_text) {
        _text = {TextChunk{
            std::string(plain_text),
            _text_opts.fg,
            _text_opts.bg,
            _text_opts.attr
        }};
        request_render();
        return *this;
    }

    const StyledText& Text::content() const noexcept {
        return _text;
    }

    Text& Text::fg(RGBA color) {
        _text_opts.fg = color;
        request_render();
        return *this;
    }
    Text& Text::bg(RGBA color) {
        _text_opts.bg = color;
        request_render();
        return *this;
    }
    Text& Text::attr(Attr a) {
        _text_opts.attr = a;
        request_render();
        return *this;
    }
    Text& Text::wrap(WrapMode m) {
        _text_opts.wrap_mode = m;
        request_render();
        return *this;
    }

    void Text::draw(OptimizedBuffer& buf, double /*delta*/) {
        int sx = screen_x();
        int sy = screen_y();
        int w = layout_w();
        int h = layout_h();
        if(w <= 0 || h <= 0)
            return;

        _link_regions.clear();

        // Fill background if set.
        if(!_text_opts.bg.is_transparent()) {
            buf.fill_rect(sx, sy, w, h, _text_opts.bg);
        }

        // Render styled chunks with word wrapping.
        int cx = 0;
        int cy = 0;

        for(const auto& chunk : _text) {
            RGBA fg = chunk.fg;
            RGBA bg = chunk.bg.is_transparent() ? _text_opts.bg : chunk.bg;
            Attr attr = chunk.attr;
            bool is_link = !chunk.link_url.empty();
            int link_start_cx = cx;

            if(_text_opts.wrap_mode == WrapMode::None) {
                if(cy < h) {
                    int start_col = cx;
                    cx = buf.draw_text(
                             sx + cx,
                             sy + cy,
                             chunk.text,
                             fg,
                             bg,
                             attr
                         ) -
                         sx;
                    if(is_link) {
                        _link_regions.push_back(
                            {sx + start_col,
                             sy + cy,
                             cx - start_col,
                             chunk.link_url}
                        );
                    }
                }
            } else if(_text_opts.wrap_mode == WrapMode::Word) {
                std::size_t s = 0;
                while(s < chunk.text.size()) {
                    if(cy >= h)
                        break;
                    std::size_t e = s;
                    bool is_nl = false;
                    bool is_sp = false;
                    while(e < chunk.text.size()) {
                        if(chunk.text[e] == '\n') {
                            is_nl = true;
                            break;
                        }
                        if(chunk.text[e] == ' ') {
                            is_sp = true;
                            break;
                        }
                        e++;
                    }
                    std::string_view word(chunk.text.data() + s, e - s);
                    if(is_nl) {
                        if(is_link && cx > link_start_cx) {
                            _link_regions.push_back(
                                {sx + link_start_cx,
                                 sy + cy,
                                 cx - link_start_cx,
                                 chunk.link_url}
                            );
                        }
                        cx = 0;
                        cy++;
                        link_start_cx = cx;
                        s = e + 1;
                        continue;
                    }
                    if(!word.empty()) {
                        int ww = 0;
                        std::size_t wp = 0;
                        while(wp < word.size()) {
                            char32_t wcp = decode_utf8(word, wp);
                            int wcw = char_display_width(wcp);
                            if(wcw > 0)
                                ww += wcw;
                        }
                        if(cx + ww > w && cx > 0) {
                            if(is_link && cx > link_start_cx) {
                                _link_regions.push_back(
                                    {sx + link_start_cx,
                                     sy + cy,
                                     cx - link_start_cx,
                                     chunk.link_url}
                                );
                            }
                            cx = 0;
                            cy++;
                            link_start_cx = cx;
                            if(cy >= h)
                                break;
                        }
                        wp = 0;
                        while(wp < word.size()) {
                            char32_t wcp = decode_utf8(word, wp);
                            int wcw = char_display_width(wcp);
                            if(wcw == 0)
                                continue;
                            if(cx >= w) {
                                if(is_link && cx > link_start_cx) {
                                    _link_regions.push_back(
                                        {sx + link_start_cx,
                                         sy + cy,
                                         cx - link_start_cx,
                                         chunk.link_url}
                                    );
                                }
                                cx = 0;
                                cy++;
                                link_start_cx = cx;
                                if(cy >= h)
                                    break;
                            }
                            buf.set_cell(
                                sx + cx,
                                sy + cy,
                                Cell{wcp, fg, bg, attr}
                            );
                            cx++;
                            if(wcw == 2) {
                                if(cx < sx + w)
                                    buf.set_cell(
                                        sx + cx,
                                        sy + cy,
                                        Cell{U'\0', fg, bg, attr}
                                    );
                                cx++;
                            }
                        }
                    }
                    if(is_sp && cx < w && cy < h) {
                        buf.set_cell(
                            sx + cx,
                            sy + cy,
                            Cell{U' ', fg, bg, attr}
                        );
                        cx++;
                    }
                    if(is_nl)
                        s = e + 1;
                    else if(is_sp)
                        s = e + 1;
                    else
                        s = chunk.text.size();
                }
                if(is_link && cx > link_start_cx) {
                    _link_regions.push_back(
                        {sx + link_start_cx,
                         sy + cy,
                         cx - link_start_cx,
                         chunk.link_url}
                    );
                }
            } else {
                std::size_t pos = 0;
                while(pos < chunk.text.size()) {
                    if(cy >= h)
                        break;

                    char32_t cp = decode_utf8(chunk.text, pos);
                    if(cp == U'\0')
                        break;

                    if(cp == U'\n') {
                        if(is_link && cx > link_start_cx) {
                            _link_regions.push_back(
                                {sx + link_start_cx,
                                 sy + cy,
                                 cx - link_start_cx,
                                 chunk.link_url}
                            );
                        }
                        cx = 0;
                        cy++;
                        link_start_cx = cx;
                        continue;
                    }

                    int cw = char_display_width(cp);
                    if(cw == 0)
                        continue;

                    if(cx >= w) {
                        if(is_link && cx > link_start_cx) {
                            _link_regions.push_back(
                                {sx + link_start_cx,
                                 sy + cy,
                                 cx - link_start_cx,
                                 chunk.link_url}
                            );
                        }
                        cx = 0;
                        cy++;
                        link_start_cx = cx;
                        if(cy >= h)
                            break;
                    }

                    buf.set_cell(sx + cx, sy + cy, Cell{cp, fg, bg, attr});
                    cx++;

                    if(cw == 2) {
                        buf.set_cell(
                            sx + cx,
                            sy + cy,
                            Cell{U'\0', fg, bg, attr}
                        );
                        cx++;
                    }
                }
                if(is_link && cx > link_start_cx) {
                    _link_regions.push_back(
                        {sx + link_start_cx,
                         sy + cy,
                         cx - link_start_cx,
                         chunk.link_url}
                    );
                }
            }
        }
    }

    void Text::process_mouse_event(MouseEvent& event) {
        if(event.type == MouseEventType::Up &&
           event.button == MouseButton::Left) {
            for(auto& reg : _link_regions) {
                if(event.x >= reg.x && event.x < reg.x + reg.w &&
                   event.y == reg.y) {
#ifdef _WIN32
                    std::string cmd = "start \"\" \"";
                    cmd += reg.url;
                    cmd += "\"";
                    std::system(cmd.c_str());
#elif __APPLE__
                    pid_t pid = fork();
                    if(pid == 0) {
                        execlp("open", "open", reg.url.c_str(), nullptr);
                        _exit(127);
                    } else if(pid > 0) {
                        waitpid(pid, nullptr, 0);
                    }
#else
                    pid_t pid = fork();
                    if(pid == 0) {
                        execlp(
                            "xdg-open",
                            "xdg-open",
                            reg.url.c_str(),
                            nullptr
                        );
                        _exit(127);
                    } else if(pid > 0) {
                        waitpid(pid, nullptr, 0);
                    }
#endif
                    event.stop_propagation();
                    return;
                }
            }
        }
        Renderable::process_mouse_event(event);
    }

    // TextNode

    TextNode::TextNode(RenderContext* ctx, std::string text, RGBA fg, Attr attr)
        : BaseRenderable(ctx)
        , _text(std::move(text))
        , _fg(fg)
        , _attr(attr) {
    }

    void TextNode::text(std::string_view text) {
        _text = std::string(text);
        request_render();
    }

    void TextNode::fg(RGBA color) {
        _fg = color;
        request_render();
    }
    void TextNode::attr(Attr a) {
        _attr = a;
        request_render();
    }

    int TextNode::add(
        std::shared_ptr<BaseRenderable> child,
        std::optional<int> index
    ) {
        auto* tn = dynamic_cast<TextNode*>(child.get());
        if(!tn)
            return -1;
        int idx = index.value_or(static_cast<int>(_text_children.size()));
        idx = std::clamp(idx, 0, static_cast<int>(_text_children.size()));
        _text_children.insert(
            _text_children.begin() + idx,
            std::static_pointer_cast<TextNode>(std::move(child))
        );
        return idx;
    }

    StyledText TextNode::gather(
        RGBA inherited_fg,
        RGBA inherited_bg,
        Attr inherited_attr
    ) const {
        StyledText result;
        RGBA fg = _fg.is_transparent() ? inherited_fg : _fg;
        Attr attr = (_attr == Attr::None) ? inherited_attr : _attr;

        if(!_text.empty()) {
            result.push_back(TextChunk{_text, fg, inherited_bg, attr});
        }

        for(const auto& child : _text_children) {
            auto child_chunks = child->gather(fg, inherited_bg, attr);
            result
                .insert(result.end(), child_chunks.begin(), child_chunks.end());
        }

        return result;
    }

    void TextNode::request_render() {
        if(_parent && !_destroyed) {
            _parent->request_render();
        }
    }

} // namespace stain
