#include "stain/input.hpp"
#include "stain/buffer.hpp"

#include <algorithm>
#include <cctype>

namespace stain {

    const std::vector<KeyBinding> default_textarea_bindings = {
        {"left", "move-left"},
        {"right", "move-right"},
        {"up", "move-up"},
        {"down", "move-down"},
        {"home", "line-home"},
        {"end", "line-end"},
        {"ctrl+a", "line-home"},
        {"ctrl+e", "line-end"},
        {"ctrl+left", "word-left"},
        {"ctrl+right", "word-right"},
        {"backspace", "delete-backward"},
        {"delete", "delete-forward"},
        {"ctrl+w", "delete-word-backward"},
        {"ctrl+k", "delete-to-line-end"},
        {"ctrl+z", "undo"},
        {"ctrl+y", "redo"},
        {"return", "new-line"},
        {"tab", "insert-tab"},
        {"ctrl+shift+c", "copy"},
    };

    EditBuffer::EditBuffer()
        : _storage(std::make_unique<StringStorage>()) {
    }
    std::string EditBuffer::plain_text() const {
        return _storage->text();
    }
    int EditBuffer::length() const noexcept {
        return _storage->length();
    }
    int EditBuffer::cursor_offset() const noexcept {
        return _cursor;
    }

    void EditBuffer::text(std::string_view text) {
        save_undo();
        _storage->text(text);
        _cursor = std::min(_cursor, length());
        clear_selection();
    }

    void EditBuffer::insert_at_cursor(std::string_view text) {
        if(has_selection())
            delete_selection();
        save_undo();
        _storage->insert(_cursor, text);
        _cursor += static_cast<int>(text.size());
    }

    void EditBuffer::delete_char_backward() {
        if(has_selection()) {
            delete_selection();
            return;
        }
        if(_cursor <= 0)
            return;
        save_undo();
        auto t = _storage->text();
        int start = _cursor - 1;
        while(start > 0 &&
              (static_cast<uint8_t>(t[static_cast<std::size_t>(start)]) &
               0xC0) == 0x80)
            start--;
        _storage->erase(start, _cursor);
        _cursor = start;
    }

    void EditBuffer::delete_char_forward() {
        if(has_selection()) {
            delete_selection();
            return;
        }
        if(_cursor >= length())
            return;
        save_undo();
        auto t = _storage->text();
        int end = _cursor + 1;
        int len = length();
        while(end < len &&
              (static_cast<uint8_t>(t[static_cast<std::size_t>(end)]) & 0xC0) ==
                  0x80)
            end++;
        _storage->erase(_cursor, end);
    }

    void EditBuffer::delete_word_backward() {
        if(_cursor <= 0)
            return;
        save_undo();
        auto t = _storage->text();
        int pos = _cursor - 1;
        while(pos > 0 &&
              std::isspace(
                  static_cast<unsigned char>(t[static_cast<std::size_t>(pos)])
              ))
            pos--;
        while(
            pos > 0 &&
            !std::isspace(
                static_cast<unsigned char>(t[static_cast<std::size_t>(pos - 1)])
            ))
            pos--;
        _storage->erase(pos, _cursor);
        _cursor = pos;
    }

    void EditBuffer::delete_to_line_end() {
        auto t = _storage->text();
        auto nl = t.find('\n', static_cast<std::size_t>(_cursor));
        int end = (nl == std::string::npos) ? length() : static_cast<int>(nl);
        if(end > _cursor) {
            save_undo();
            _storage->erase(_cursor, end);
        }
    }

    void EditBuffer::move_cursor(int offset) {
        _cursor = std::clamp(_cursor + offset, 0, length());
    }
    void EditBuffer::move_cursor_to(int offset) {
        _cursor = std::clamp(offset, 0, length());
    }

    void EditBuffer::move_cursor_left() {
        if(_cursor <= 0)
            return;
        auto t = _storage->text();
        int p = _cursor - 1;
        while(p > 0 && (static_cast<uint8_t>(t[static_cast<std::size_t>(p)]) &
                        0xC0) == 0x80)
            p--;
        _cursor = p;
    }

    void EditBuffer::move_cursor_right() {
        if(_cursor >= length())
            return;
        auto t = _storage->text();
        int p = _cursor + 1;
        int len = length();
        while(p < len && (static_cast<uint8_t>(t[static_cast<std::size_t>(p)]) &
                          0xC0) == 0x80)
            p++;
        _cursor = p;
    }

    void EditBuffer::move_word_forward() {
        auto t = _storage->text();
        int pos = _cursor, len = length();
        while(pos < len &&
              !std::isspace(
                  static_cast<unsigned char>(t[static_cast<std::size_t>(pos)])
              ))
            pos++;
        while(pos < len &&
              std::isspace(
                  static_cast<unsigned char>(t[static_cast<std::size_t>(pos)])
              ))
            pos++;
        _cursor = pos;
    }

    void EditBuffer::move_word_backward() {
        auto t = _storage->text();
        int pos = _cursor;
        if(pos > 0)
            pos--;
        while(pos > 0 &&
              std::isspace(
                  static_cast<unsigned char>(t[static_cast<std::size_t>(pos)])
              ))
            pos--;
        while(
            pos > 0 &&
            !std::isspace(
                static_cast<unsigned char>(t[static_cast<std::size_t>(pos - 1)])
            ))
            pos--;
        _cursor = pos;
    }

    void EditBuffer::move_line_start() {
        auto t = _storage->text();
        int pos = _cursor;
        while(pos > 0 && t[static_cast<std::size_t>(pos - 1)] != '\n')
            pos--;
        _cursor = pos;
    }

    void EditBuffer::move_line_end() {
        auto t = _storage->text();
        int pos = _cursor, len = length();
        while(pos < len && t[static_cast<std::size_t>(pos)] != '\n')
            pos++;
        _cursor = pos;
    }

    void EditBuffer::move_line_up() {
        auto t = _storage->text();
        if(_cursor <= 0)
            return;
        int line_start = _cursor;
        while(line_start > 0 &&
              t[static_cast<std::size_t>(line_start - 1)] != '\n')
            line_start--;
        if(line_start == 0) {
            _cursor = 0;
            return;
        }
        int prev_line_end = line_start - 1;
        int prev_line_start = prev_line_end;
        while(prev_line_start > 0 &&
              t[static_cast<std::size_t>(prev_line_start - 1)] != '\n')
            prev_line_start--;
        int col = cursor_column();
        int target = prev_line_start;
        int display_col = 0;
        std::size_t pos = static_cast<std::size_t>(target);
        while(pos < static_cast<std::size_t>(prev_line_end) && display_col < col) {
            char32_t cp = decode_utf8(t, pos);
            display_col += char_display_width(cp);
            target = static_cast<int>(pos);
        }
        _cursor = target;
    }

    void EditBuffer::move_line_down() {
        auto t = _storage->text();
        int len = length();
        if(_cursor >= len)
            return;
        int line_start = _cursor;
        while(line_start > 0 &&
              t[static_cast<std::size_t>(line_start - 1)] != '\n')
            line_start--;
        int line_end = _cursor;
        while(line_end < len && t[static_cast<std::size_t>(line_end)] != '\n')
            line_end++;
        if(line_end >= len) {
            _cursor = len;
            return;
        }
        int next_line_start = line_end + 1;
        int next_line_end = next_line_start;
        while(next_line_end < len &&
              t[static_cast<std::size_t>(next_line_end)] != '\n')
            next_line_end++;
        int col = cursor_column();
        int target = next_line_start;
        int display_col = 0;
        std::size_t pos = static_cast<std::size_t>(target);
        while(pos < static_cast<std::size_t>(next_line_end) && display_col < col) {
            char32_t cp = decode_utf8(t, pos);
            display_col += char_display_width(cp);
            target = static_cast<int>(pos);
        }
        _cursor = target;
    }

    int EditBuffer::cursor_column() const {
        auto t = _storage->text();
        int line_start = _cursor;
        while(line_start > 0 &&
              t[static_cast<std::size_t>(line_start - 1)] != '\n')
            line_start--;
        int col = 0;
        std::size_t pos = static_cast<std::size_t>(line_start);
        while(pos < static_cast<std::size_t>(_cursor)) {
            char32_t cp = decode_utf8(t, pos);
            col += char_display_width(cp);
        }
        return col;
    }

    void EditBuffer::undo() {
        if(_undo_stack.empty())
            return;
        _redo_stack.push_back({_storage->text(), _cursor});
        auto& e = _undo_stack.back();
        _storage->text(e.text);
        _cursor = e.cursor;
        _undo_stack.pop_back();
    }

    void EditBuffer::redo() {
        if(_redo_stack.empty())
            return;
        _undo_stack.push_back({_storage->text(), _cursor});
        auto& e = _redo_stack.back();
        _storage->text(e.text);
        _cursor = e.cursor;
        _redo_stack.pop_back();
    }

    bool EditBuffer::has_selection() const noexcept {
        return _sel_start >= 0 && _sel_end >= 0 && _sel_start != _sel_end;
    }
    void EditBuffer::select(int start, int end) {
        _sel_start = start;
        _sel_end = end;
    }
    void EditBuffer::clear_selection() {
        _sel_start = _sel_end = -1;
    }
    std::pair<int, int> EditBuffer::selection() const {
        return {_sel_start, _sel_end};
    }

    std::string EditBuffer::selected_text() const {
        if(!has_selection())
            return {};
        auto t = _storage->text();
        int s = std::min(_sel_start, _sel_end),
            e = std::max(_sel_start, _sel_end);
        return t.substr(
            static_cast<std::size_t>(s),
            static_cast<std::size_t>(e - s)
        );
    }

    void EditBuffer::delete_selection() {
        if(!has_selection())
            return;
        save_undo();
        int s = std::min(_sel_start, _sel_end),
            e = std::max(_sel_start, _sel_end);
        _storage->erase(s, e);
        _cursor = s;
        clear_selection();
    }

    void EditBuffer::save_undo() {
        _undo_stack.push_back({_storage->text(), _cursor});
        _redo_stack.clear();
        if(_undo_stack.size() > 100)
            _undo_stack.pop_front();
    }

    Textarea::Textarea(RenderContext* ctx, TextareaOptions opts)
        : Renderable(ctx, static_cast<RenderableOptions&>(opts))
        , _textarea_opts(std::move(opts)) {
        _focusable = true;
        auto merged = merge_bindings(
            default_textarea_bindings,
            _textarea_opts.key_bindings
        );
        _binding_map = build_binding_map(merged, _textarea_opts.key_aliases);
    }

    std::string Textarea::plain_text() const {
        return _edit_buffer.plain_text();
    }
    Textarea& Textarea::text(std::string_view text) {
        _edit_buffer.text(text);
        request_render();
        return *this;
    }
    Textarea& Textarea::insert_text(std::string_view text) {
        _edit_buffer.insert_at_cursor(text);
        request_render();
        return *this;
    }
    EditBuffer& Textarea::edit_buffer() noexcept {
        return _edit_buffer;
    }
    const EditBuffer& Textarea::edit_buffer() const noexcept {
        return _edit_buffer;
    }

    Textarea& Textarea::placeholder(std::string_view v) {
        _textarea_opts.placeholder = std::string(v);
        request_render();
        return *this;
    }
    Textarea& Textarea::style(ItemStyle s) {
        _textarea_opts.style = s;
        request_render();
        return *this;
    }
    Textarea& Textarea::focus_style(ItemStyle s) {
        _textarea_opts.focus_style = s;
        request_render();
        return *this;
    }
    Textarea& Textarea::key_bindings(std::vector<KeyBinding> b) {
        _textarea_opts.key_bindings = std::move(b);
        auto merged = merge_bindings(
            default_textarea_bindings,
            _textarea_opts.key_bindings
        );
        _binding_map = build_binding_map(merged, _textarea_opts.key_aliases);
        return *this;
    }
    Textarea& Textarea::on_submit(std::function<void(std::string_view)> f) {
        _textarea_opts.on_submit = std::move(f);
        return *this;
    }

    void Textarea::draw(OptimizedBuffer& buf, double /*delta*/) {
        int sx = screen_x(), sy = screen_y(), w = layout_w(), h = layout_h();
        if(w <= 0 || h <= 0)
            return;
        const auto& sty =
            focused() ? _textarea_opts.focus_style : _textarea_opts.style;
        RGBA tc = sty.text;
        RGBA bc = sty.bg;
        RGBA sel_bg{0, 0.4f, 0.8f, 1};
        if(!bc.is_transparent())
            buf.fill_rect(sx, sy, w, h, bc);
        std::string text = _edit_buffer.plain_text();
        if(text.empty() && !_textarea_opts.placeholder.empty() && !focused()) {
            buf.draw_text(
                sx,
                sy,
                _textarea_opts.placeholder,
                {0.5f, 0.5f, 0.5f, 1},
                bc
            );
            return;
        }
        adjust_h_scroll();
        adjust_v_scroll();
        auto sel = _edit_buffer.selection();
        int sel_s = std::min(sel.first, sel.second);
        int sel_e = std::max(sel.first, sel.second);
        bool has_sel = _edit_buffer.has_selection();
        int cx = 0, cy = 0, cursorpos = _edit_buffer.cursor_offset();
        int cdx = -1, cdy = -1;
        std::size_t pos = 0;
        int byte_pos = 0;
        while(pos < text.size()) {
            if(byte_pos == cursorpos) {
                cdx = sx + cx - _h_scroll;
                cdy = sy + cy - _v_scroll;
            }
            char32_t cp = decode_utf8(text, pos);
            if(cp == U'\0')
                break;
            if(cp == U'\n') {
                cx = 0;
                cy++;
                byte_pos = static_cast<int>(pos);
                continue;
            }
            if(cy >= _v_scroll + h)
                break;
            if(cy >= _v_scroll) {
                if(cx >= _h_scroll) {
                    int draw_x = sx + cx - _h_scroll;
                    if(draw_x >= sx + w) {
                        cx++;
                        byte_pos = static_cast<int>(pos);
                        continue;
                    }
                    int cw = char_display_width(cp);
                    if(cw == 0)
                        continue;
                    bool selected = has_sel && byte_pos >= sel_s &&
                                    byte_pos < sel_e;
                    buf.set_cell(
                        draw_x,
                        sy + cy - _v_scroll,
                        Cell{cp, tc, selected ? sel_bg : bc, Attr::None}
                    );
                    cx++;
                    if(cw == 2 && draw_x + 1 < sx + w) {
                        bool wide_selected =
                            has_sel && byte_pos >= sel_s && byte_pos < sel_e;
                        buf.set_cell(
                            draw_x + 1,
                            sy + cy - _v_scroll,
                            Cell{U'\0', tc, wide_selected ? sel_bg : bc, Attr::None}
                        );
                        cx++;
                    }
                } else {
                    cx++;
                    int cw = char_display_width(cp);
                    if(cw == 2)
                        cx++;
                }
            } else {
                cx++;
                int cw = char_display_width(cp);
                if(cw == 2)
                    cx++;
            }
            byte_pos = static_cast<int>(pos);
        }
        if(h == 1) {
            if(_h_scroll > 0)
                buf.set_cell(
                    sx,
                    sy,
                    Cell{U'‹', RGBA{0.45f, 0.45f, 0.45f, 1}, bc, Attr::None}
                );
            if(cx >= _h_scroll + w)
                buf.set_cell(
                    sx + w - 1,
                    sy,
                    Cell{U'›', RGBA{0.45f, 0.45f, 0.45f, 1}, bc, Attr::None}
                );
        }
        if(byte_pos == cursorpos ||
           cursorpos == static_cast<int>(text.size())) {
            cdx = sx + cx - _h_scroll;
            cdy = sy + cy - _v_scroll;
        }
        if(focused() && cdx >= sx && cdx < sx + w && cdy >= sy &&
           cdy < sy + h && _ctx) {
            _ctx->cursor(cdx, cdy, true);
            _ctx->cursor_style({.style = CursorStyle::Line, .blinking = true});
        }
    }

    int Textarea::byte_offset_at(int cell_x, int cell_y) const {
        int sx = screen_x(), sy = screen_y(), w = layout_w(), h = layout_h();
        if(cell_x < sx || cell_x >= sx + w || cell_y < sy ||
           cell_y >= sy + h)
            return -1;
        std::string text = _edit_buffer.plain_text();
        int cx = 0, cy = 0;
        std::size_t pos = 0;
        while(pos < text.size()) {
            int byte_pos = static_cast<int>(pos);
            char32_t cp = decode_utf8(text, pos);
            if(cp == U'\0')
                break;
            if(cp == U'\n') {
                if(cell_y == sy + cy - _v_scroll) {
                    return byte_pos;
                }
                cx = 0;
                cy++;
                continue;
            }
            if(cy >= _v_scroll + h)
                break;
            int cw = char_display_width(cp);
            if(cw == 0)
                continue;
            if(cy >= _v_scroll) {
                if(cx >= _h_scroll) {
                    int draw_x = sx + cx - _h_scroll;
                    if(draw_x >= sx + w) {
                        cx += cw;
                        continue;
                    }
                    if(draw_x == cell_x &&
                       sy + cy - _v_scroll == cell_y) {
                        return byte_pos;
                    }
                    cx += cw;
                } else {
                    cx += cw;
                }
            } else {
                cx += cw;
            }
        }
        return _edit_buffer.length();
    }

    void Textarea::process_mouse_event(MouseEvent& event) {
        if(event.type == MouseEventType::Down &&
           event.button == MouseButton::Left) {
            if(_focusable)
                focus();
            int offset = byte_offset_at(event.x, event.y);
            if(offset >= 0) {
                _edit_buffer.move_cursor_to(offset);
                _edit_buffer.select(offset, offset);
                _mouse_selecting = true;
                request_render();
                event.stop_propagation();
            }
            return;
        }
        if(event.type == MouseEventType::Drag &&
           event.button == MouseButton::Left && _mouse_selecting) {
            int offset = byte_offset_at(event.x, event.y);
            if(offset >= 0) {
                int anchor = _edit_buffer.selection().first;
                _edit_buffer.select(anchor, offset);
                request_render();
                event.stop_propagation();
            }
            return;
        }
        if(event.type == MouseEventType::Up &&
           event.button == MouseButton::Left && _mouse_selecting) {
            _mouse_selecting = false;
            if(_edit_buffer.has_selection() && _ctx) {
                _ctx->clipboard_copy(_edit_buffer.selected_text());
            }
            request_render();
            event.stop_propagation();
            return;
        }
        Renderable::process_mouse_event(event);
    }

    void Textarea::adjust_h_scroll() {
        if(!focused())
            return;
        int _cursorcol = _edit_buffer.cursor_column();
        int w = layout_w();
        if(_cursorcol < _h_scroll)
            _h_scroll = _cursorcol;
        if(_cursorcol >= _h_scroll + w)
            _h_scroll = _cursorcol - w + 1;
    }

    void Textarea::adjust_v_scroll() {
        int h = layout_h();
        if(h <= 0)
            return;
        int cl = cursor_line();
        if(cl < _v_scroll)
            _v_scroll = cl;
        if(cl >= _v_scroll + h)
            _v_scroll = cl - h + 1;
    }

    int Textarea::cursor_line() const {
        auto text = _edit_buffer.plain_text();
        int cursor = _edit_buffer.cursor_offset();
        int line = 0;
        for(int i = 0; i < cursor && i < static_cast<int>(text.size()); i++) {
            if(text[i] == '\n')
                line++;
        }
        return line;
    }

    bool Textarea::handle_key_press(KeyEvent& key) {
        std::string combo = key_binding_key(key);
        auto it = _binding_map.find(combo);
        if(it != _binding_map.end()) {
            const auto& action = it->second;
            if(action == "move-left")
                _edit_buffer.move_cursor_left();
            else if(action == "move-right")
                _edit_buffer.move_cursor_right();
            else if(action == "move-up")
                _edit_buffer.move_line_up();
            else if(action == "move-down")
                _edit_buffer.move_line_down();
            else if(action == "line-home")
                _edit_buffer.move_line_start();
            else if(action == "line-end")
                _edit_buffer.move_line_end();
            else if(action == "word-left")
                _edit_buffer.move_word_backward();
            else if(action == "word-right")
                _edit_buffer.move_word_forward();
            else if(action == "delete-backward")
                _edit_buffer.delete_char_backward();
            else if(action == "delete-forward")
                _edit_buffer.delete_char_forward();
            else if(action == "delete-word-backward")
                _edit_buffer.delete_word_backward();
            else if(action == "delete-to-line-end")
                _edit_buffer.delete_to_line_end();
            else if(action == "undo")
                _edit_buffer.undo();
            else if(action == "redo")
                _edit_buffer.redo();
            else if(action == "new-line" && new_line()) {
                request_render();
                return true;
            } else if(action == "insert-tab")
                _edit_buffer.insert_at_cursor("  ");
            else if(action == "copy") {
                if(_edit_buffer.has_selection() && _ctx) {
                    _ctx->clipboard_copy(_edit_buffer.selected_text());
                }
                request_render();
                return true;
            } else {
                return false;
            }
            request_render();
            return true;
        }
        if(!key.ctrl && !key.meta && !key.sequence.empty()) {
            _edit_buffer.insert_at_cursor(key.sequence);
            request_render();
            return true;
        } else if(
            !key.ctrl && !key.meta && key.name.size() == 1 &&
            key.name[0] >= 32 && key.name[0] <= 126
        ) {
            _edit_buffer.insert_at_cursor(key.name);
            request_render();
            return true;
        }
        return false;
    }

    void Textarea::handle_paste(PasteEvent& event) {
        _edit_buffer.insert_at_cursor(event.text);
        emit(events::Paste{event});
        request_render();
    }
    bool Textarea::new_line() {
        _edit_buffer.insert_at_cursor("\n");
        return true;
    }

    void Input::handle_paste(PasteEvent& event) {
        // Strip newlines for single-line input.
        std::string filtered;
        for(char c : event.text) {
            if(c != '\n' && c != '\r')
                filtered += c;
        }
        event.text = std::move(filtered);
        Textarea::handle_paste(event);
        if(_input_opts.on_input)
            _input_opts.on_input(value());
        std::string current = value();
        if(current != _last_committed_value) {
            if(_input_opts.on_change)
                _input_opts.on_change(current);
            _last_committed_value = current;
        }
    }

    Input::Input(RenderContext* ctx, InputOptions opts)
        : Textarea(ctx, static_cast<TextareaOptions&>(opts))
        , _input_opts(std::move(opts)) {
        if(!_input_opts.value.empty())
            text(_input_opts.value);
    }

    std::string Input::value() const {
        return plain_text();
    }
    Input& Input::value(std::string_view v) {
        text(v);
        _last_committed_value = std::string(v);
        return *this;
    }
    Input& Input::max_length(int n) {
        _input_opts.max_length = n;
        return *this;
    }
    Input& Input::on_input(std::function<void(std::string_view)> f) {
        _input_opts.on_input = std::move(f);
        return *this;
    }
    Input& Input::on_change(std::function<void(std::string_view)> f) {
        _input_opts.on_change = std::move(f);
        return *this;
    }
    Input& Input::on_enter(std::function<void(std::string_view)> f) {
        _input_opts.on_enter = std::move(f);
        return *this;
    }

    bool Input::handle_key_press(KeyEvent& key) {
        if(key.name == "return" && !key.ctrl) {
            if(_input_opts.on_enter)
                _input_opts.on_enter(value());
            emit(events::InputEntered{value()});
            return true;
        }
        bool handled = Textarea::handle_key_press(key);
        if(handled) {
            if(_input_opts.on_input)
                _input_opts.on_input(value());
            std::string current = value();
            if(current != _last_committed_value) {
                if(_input_opts.on_change)
                    _input_opts.on_change(current);
                emit(events::InputChanged{current});
                _last_committed_value = current;
            }
        }
        return handled;
    }

} // namespace stain
