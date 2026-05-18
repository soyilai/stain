#include "stain/renderer.hpp"
#include "stain/terminal.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <poll.h>
#include <unistd.h>
#include <yoga/Yoga.h>

namespace stain {

    // SIGWINCH handling

    static std::atomic<bool> g_sigwinch_received{false};

    static void sigwinch_handler(int) {
        g_sigwinch_received.store(true, std::memory_order_relaxed);
    }

    struct HitEntry {
        int x, y, w, h;
        int num;
    };

    // Renderer::Impl

    struct Renderer::Impl {
        RendererOptions opts;
        std::unique_ptr<TerminalGuard> terminal;
        std::unique_ptr<Renderable> root;

        OptimizedBuffer current_buf;
        OptimizedBuffer next_buf;

        bool running{false};
        bool render_requested{true};
        int live_count{0};
        Renderable* _focused{nullptr};

        int cursor_col{0}, cursor_row{0};
        bool cursor_visible{false};
        CursorOptions cursor_opts;

        std::vector<HitEntry> hit_grid;
        std::vector<ClipRect> hit_scissor_stack;

        std::vector<Renderable*> lifecycle_renderables;
        std::vector<std::shared_ptr<Timeline>> timelines;

        Signal<KeyEvent&> key_signal;
        Signal<PasteEvent&> paste_signal;

        std::optional<Selection> selection;

        std::chrono::steady_clock::time_point last_frame_time;

        bool in_bracketed_paste{false};
        std::string paste_buffer;

        struct PendingPlacement {
            int x, y;
            std::string data;
        };
        std::vector<PendingPlacement> pending_placements;

        Impl(RendererOptions o)
            : opts(std::move(o))
            , current_buf(BufferOptions{.width = 1, .height = 1})
            , next_buf(BufferOptions{.width = 1, .height = 1}) {
        }
    };

    // Renderer

    std::unique_ptr<Renderer> Renderer::create(RendererOptions opts) {
        auto r = std::unique_ptr<Renderer>(new Renderer(std::move(opts)));
        return r;
    }

    Renderer::Renderer(RendererOptions opts)
        : _impl(std::make_unique<Impl>(std::move(opts))) {
        if(!_impl->opts._testing) {
            _impl->terminal = std::make_unique<TerminalGuard>(
                _impl->opts._screen_mode == ScreenMode::AlternateScreen,
                _impl->opts._use_mouse,
                _impl->opts._use_kitty_keyboard
            );

            if(_impl->opts._width == 0)
                _impl->opts._width = _impl->terminal->info().cols;
            if(_impl->opts._height == 0)
                _impl->opts._height = _impl->terminal->info().rows;
        } else {
            if(_impl->opts._width == 0)
                _impl->opts._width = 80;
            if(_impl->opts._height == 0)
                _impl->opts._height = 24;
        }

        _impl->current_buf.resize(_impl->opts._width, _impl->opts._height);
        _impl->next_buf.resize(_impl->opts._width, _impl->opts._height);

        // Create root renderable.
        RenderableOptions root_opts;
        root_opts.flex_direction = FlexDirection::Column;
        root_opts.flex_grow = 1.f;
        root_opts.width = px(_impl->opts._width);
        root_opts.height = px(_impl->opts._height);
        root_opts.id = "root";
        _impl->root = std::make_unique<Renderable>(this, std::move(root_opts));

        _impl->last_frame_time = std::chrono::steady_clock::now();
    }

    Renderer::~Renderer() {
        if(_impl->root) {
            _impl->root->destroy_recursive();
        }
    }

    int Renderer::width() const noexcept {
        return _impl->opts._width;
    }
    int Renderer::height() const noexcept {
        return _impl->opts._height;
    }

    void Renderer::request_render() {
        _impl->render_requested = true;
    }
    void Renderer::request_live() {
        _impl->live_count++;
    }
    void Renderer::drop_live() {
        if(_impl->live_count > 0)
            _impl->live_count--;
    }

    void Renderer::focus_renderable(Renderable* r) {
        if(_impl->_focused && _impl->_focused != r) {
            _impl->_focused->blur();
        }
        _impl->_focused = r;
    }

    Renderable* Renderer::focused() const noexcept {
        return _impl->_focused;
    }

    void Renderer::cursor(int col, int row, bool visible) {
        _impl->cursor_col = col;
        _impl->cursor_row = row;
        _impl->cursor_visible = visible;
    }

    void Renderer::cursor_style(CursorOptions opts) {
        _impl->cursor_opts = opts;
    }

    void Renderer::mouse_pointer(MousePointer /*shape*/) {
        // Terminal mouse pointer is controlled by the terminal emulator; no-op.
    }

    void Renderer::add_to_hit_grid(int x, int y, int w, int h, int num) {
        _impl->hit_grid.push_back(HitEntry{x, y, w, h, num});
    }

    void Renderer::push_hit_scissor(ClipRect rect) {
        _impl->hit_scissor_stack.push_back(rect);
    }

    void Renderer::pop_hit_scissor() {
        if(!_impl->hit_scissor_stack.empty())
            _impl->hit_scissor_stack.pop_back();
    }

    void Renderer::clear_hit_grid() {
        _impl->hit_grid.clear();
        _impl->hit_scissor_stack.clear();
    }

    void Renderer::register_lifecycle(Renderable* r) {
        _impl->lifecycle_renderables.push_back(r);
    }

    void Renderer::unregister_lifecycle(Renderable* r) {
        auto& v = _impl->lifecycle_renderables;
        v.erase(std::remove(v.begin(), v.end(), r), v.end());
    }

    void Renderer::add_timeline(std::shared_ptr<Timeline> tl) {
        _impl->timelines.push_back(std::move(tl));
    }

    void Renderer::remove_timeline(Timeline* tl) {
        auto& v = _impl->timelines;
        v.erase(
            std::remove_if(
                v.begin(),
                v.end(),
                [tl](const auto& ptr) { return ptr.get() == tl; }
            ),
            v.end()
        );
    }

    std::optional<Selection> Renderer::get_selection() const {
        return _impl->selection;
    }
    void Renderer::clear_selection() {
        _impl->selection.reset();
    }
    void Renderer::start_selection(Renderable* r, int x, int y) {
        _impl->selection = Selection{r, x, y, r, x, y};
    }
    void Renderer::update_selection(Renderable* r, int x, int y) {
        if(_impl->selection) {
            _impl->selection->end_renderable = r;
            _impl->selection->end_x = x;
            _impl->selection->end_y = y;
        }
    }

    std::size_t Renderer::on_key(std::function<bool(KeyEvent&)> handler) {
        return _impl->key_signal.connect(std::move(handler));
    }
    void Renderer::off_key(std::size_t id) {
        _impl->key_signal.disconnect(id);
    }

    std::size_t Renderer::on_paste(std::function<void(PasteEvent&)> handler) {
        return _impl->paste_signal.connect(std::move(handler));
    }
    void Renderer::off_paste(std::size_t id) {
        _impl->paste_signal.disconnect(id);
    }

    void Renderer::write_raw(std::string_view data) {
        if(_impl->terminal)
            _impl->terminal->write(data);
    }

    void Renderer::write_after_flush(int x, int y, std::string_view data) {
        _impl->pending_placements.push_back(
            {x, y, std::string(data)}
        );
    }

    const TerminalInfo& Renderer::terminal_info() const {
        static TerminalInfo default_info{};
        if(_impl->terminal)
            return _impl->terminal->info();
        return default_info;
    }

    Renderable& Renderer::root() {
        return *_impl->root;
    }

    // Run loop

    void Renderer::run() {
        _impl->running = true;

        // Install SIGWINCH handler.
        struct sigaction sa{};
        sa.sa_handler = sigwinch_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        sigaction(SIGWINCH, &sa, nullptr);

        auto frame_duration =
            std::chrono::microseconds(1'000'000 / _impl->opts._target_fps);

        while(_impl->running) {
            auto frame_start = std::chrono::steady_clock::now();

            // Handle SIGWINCH.
            if(g_sigwinch_received.exchange(false)) {
                if(_impl->terminal)
                    _impl->terminal->update_size();
                int nw = _impl->terminal ? _impl->terminal->info().cols
                                         : _impl->opts._width;
                int nh = _impl->terminal ? _impl->terminal->info().rows
                                         : _impl->opts._height;
                if(nw != _impl->opts._width || nh != _impl->opts._height) {
                    _impl->opts._width = nw;
                    _impl->opts._height = nh;
                    _impl->current_buf.resize(nw, nh);
                    _impl->next_buf.resize(nw, nh);
                    _impl->root->width(px(nw));
                    _impl->root->height(px(nh));
                    _impl->render_requested = true;
                }
            }

            // Poll stdin for input (non-blocking).
            struct pollfd pfd{};
            pfd.fd = STDIN_FILENO;
            pfd.events = POLLIN;
            while(poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN)) {
                char buf[1024];
                auto n = ::read(STDIN_FILENO, buf, sizeof(buf));
                if(n > 0) {
                    handle_stdin(buf, static_cast<std::size_t>(n));
                }
            }

            // Render if needed.
            if(_impl->render_requested || _impl->live_count > 0) {
                activate_frame();
                _impl->last_frame_time = std::chrono::steady_clock::now();
                _impl->render_requested = false;

                if(_impl->live_count > 0) {
                    _impl->render_requested = true;
                }
            }

            // Sleep to maintain target FPS.
            auto elapsed = std::chrono::steady_clock::now() - frame_start;
            if(elapsed < frame_duration) {
                auto sleep_us =
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        frame_duration - elapsed
                    );
                // Use poll with timeout for interruptible sleep.
                struct pollfd pfd2{};
                pfd2.fd = STDIN_FILENO;
                pfd2.events = POLLIN;
                poll(&pfd2, 1, static_cast<int>(sleep_us.count() / 1000));
            }
        }
    }

    void Renderer::activate_frame() {
        auto now = std::chrono::steady_clock::now();
        double delta =
            std::chrono::duration<double>(now - _impl->last_frame_time).count();
        if(delta > 0.1)
            delta = 1.0 / _impl->opts._target_fps;

        // 1. Drive active timelines.
        for(auto& tl : _impl->timelines) {
            tl->update(delta);
        }

        // 2. Lifecycle passes.
        run_lifecycle_passes(delta);

        // 3. Layout.
        update_layout();

        // 4. Render.
        render_frame(delta);

        // 5. Diff and flush.
        diff_and_flush();
    }

    void Renderer::run_lifecycle_passes(double delta) {
        for(auto* r : _impl->lifecycle_renderables) {
            r->on_lifecycle_pass(delta);
        }
    }

    void Renderer::update_layout() {
        auto* root_yoga = _impl->root->yoga_node();
        YGNodeCalculateLayout(
            root_yoga,
            static_cast<float>(_impl->opts._width),
            static_cast<float>(_impl->opts._height),
            YGDirectionLTR
        );
        _impl->root->update_from_layout();
    }

    void Renderer::render_frame(double delta) {
        _impl->next_buf.clear(_impl->opts._background_color);
        clear_hit_grid();
        _impl->root->render(_impl->next_buf, delta);
    }

    void Renderer::diff_and_flush() {
        if(!_impl->terminal)
            return;

        auto& term = *_impl->terminal;
        int w = _impl->opts._width;
        int h = _impl->opts._height;

        RGBA last_fg{-1, -1, -1, -1};
        RGBA last_bg{-1, -1, -1, -1};
        Attr last_attr = static_cast<Attr>(0xFFFFFFFF);
        int last_col = -1, last_row = -1;

        for(int row = 0; row < h; row++) {
            for(int col = 0; col < w; col++) {
                Cell n = _impl->next_buf.get_cell(col, row);
                Cell c = _impl->current_buf.get_cell(col, row);

                if(n == c)
                    continue;

                // Move cursor if needed.
                if(last_row != row || last_col != col) {
                    term.move_to(col, row);
                }

                // Update attributes.
                if(n.attr != last_attr) {
                    term.reset_attr();
                    if(n.attr != Attr::None)
                        term.attr(n.attr);
                    last_attr = n.attr;
                    // Reset colors after attr reset.
                    last_fg = {-1, -1, -1, -1};
                    last_bg = {-1, -1, -1, -1};
                }

                // Update colors.
                if(!(n.fg == last_fg)) {
                    term.fg(n.fg);
                    last_fg = n.fg;
                }
                if(!(n.bg == last_bg)) {
                    term.bg(n.bg);
                    last_bg = n.bg;
                }

                // Write character.
                if(n.ch == U'\0') {
                    // skip.
                    last_col = col + 1;
                    last_row = row;
                    continue;
                }

                char utf8[4];
                int len = encode_utf8(n.ch, utf8);
                term.write(
                    std::string_view(utf8, static_cast<std::size_t>(len))
                );

                last_col = col + 1;
                last_row = row;
            }
        }

        if(_impl->cursor_visible) {
            term.show_cursor(true);
            term.move_to(_impl->cursor_col, _impl->cursor_row);
            term.cursor_style(_impl->cursor_opts);
        } else {
            term.show_cursor(false);
        }

        term.flush();

        // Process image placements (Kitty/iTerm2) after the cell flush
        // so overlays render on top of the cell content.
        if(!_impl->pending_placements.empty()) {
            for(auto& p : _impl->pending_placements) {
                term.move_to(p.x, p.y);
                term.write(p.data);
            }
            _impl->pending_placements.clear();
            term.flush();
        }

        // swap the underlying cell data.
        std::swap(_impl->current_buf, _impl->next_buf);
    }

    // Input parsing

    void Renderer::handle_stdin(const char* data, std::size_t len) {
        // Check for bracketed paste start/end.
        std::string_view sv(data, len);

        // Bracketed paste start: ESC[200~
        if(sv.find("\033[200~") != std::string_view::npos) {
            _impl->in_bracketed_paste = true;
            _impl->paste_buffer.clear();
            auto pos = sv.find("\033[200~");
            auto after = pos + 6;
            if(after < len) {
                auto remaining = sv.substr(after);
                // End marker may be in the same chunk.
                auto end_pos = remaining.find("\033[201~");
                if(end_pos != std::string_view::npos) {
                    _impl->paste_buffer.append(remaining.substr(0, end_pos));
                    _impl->in_bracketed_paste = false;
                    PasteEvent event;
                    event.text = std::move(_impl->paste_buffer);
                    event.raw_bytes.assign(
                        event.text.begin(),
                        event.text.end()
                    );
                    _impl->paste_signal.emit(event);
                    return;
                }
                _impl->paste_buffer.append(remaining);
            }
            return;
        }

        // Bracketed paste end: ESC[201~
        if(_impl->in_bracketed_paste) {
            auto pos = sv.find("\033[201~");
            if(pos != std::string_view::npos) {
                _impl->paste_buffer.append(sv.substr(0, pos));
                _impl->in_bracketed_paste = false;

                PasteEvent event;
                event.text = std::move(_impl->paste_buffer);
                event.raw_bytes.assign(event.text.begin(), event.text.end());
                _impl->paste_signal.emit(event);
                return;
            }
            _impl->paste_buffer.append(sv);
            return;
        }

        parse_key_sequence(data, len);
    }

    void Renderer::parse_key_sequence(const char* data, std::size_t len) {
        // Check for mouse sequences first (SGR: ESC[<...M or ESC[<...m).
        std::string_view sv(data, len);
        if(sv.size() >= 3 && sv[0] == '\033' && sv[1] == '[' && sv[2] == '<') {
            parse_mouse_sequence(data, len);
            return;
        }

        KeyEvent event;
        event.source = "raw";

        if(len == 1) {
            char c = data[0];
            if(c == 3) {
                // Ctrl+C.
                event.name = "c";
                event.ctrl = true;
                if(_impl->opts._exit_on_ctrl_c) {
                    _impl->running = false;
                    return;
                }
            } else if(c == 13 || c == 10) {
                event.name = "return";
            } else if(c == 27) {
                event.name = "escape";
            } else if(c == 9) {
                event.name = "tab";
            } else if(c == 127) {
                event.name = "backspace";
            } else if(c >= 1 && c <= 26) {
                event.name = std::string(1, static_cast<char>('a' + c - 1));
                event.ctrl = true;
            } else if(c >= 32 && c <= 126) {
                event.name = std::string(1, c);
                event.sequence = std::string(1, c);
            } else {
                event.name = "unknown";
            }
        } else if(len >= 3 && data[0] == '\033' && data[1] == '[') {
            // CSI sequences.
            char final_char = data[len - 1];
            bool skip_ansi_modifier = false;
            switch(final_char) {
            case 'A':
                event.name = "up";
                break;
            case 'B':
                event.name = "down";
                break;
            case 'C':
                event.name = "right";
                break;
            case 'D':
                event.name = "left";
                break;
            case 'H':
                event.name = "home";
                break;
            case 'F':
                event.name = "end";
                break;
            case '~': {
                // Parse number before ~.
                std::string num(sv.substr(2, len - 3));
                if(num == "2")
                    event.name = "insert";
                else if(num == "3")
                    event.name = "delete";
                else if(num == "5")
                    event.name = "pageup";
                else if(num == "6")
                    event.name = "pagedown";
                else if(num == "15")
                    event.name = "f5";
                else if(num == "17")
                    event.name = "f6";
                else if(num == "18")
                    event.name = "f7";
                else if(num == "19")
                    event.name = "f8";
                else if(num == "20")
                    event.name = "f9";
                else if(num == "21")
                    event.name = "f10";
                else if(num == "23")
                    event.name = "f11";
                else if(num == "24")
                    event.name = "f12";
                else
                    event.name = "unknown";
                break;
            }
            case 'u': {
                // Kitty keyboard protocol: ESC[code;modsu
                std::string_view inner(sv.substr(2, len - 3));
                auto semi = inner.find(';');
                int code = 0, mod_raw = 1;
                auto parse_num = [&](std::string_view s) -> int {
                    int v = 0;
                    for(char ch : s) {
                        if(ch < '0' || ch > '9') return -1;
                        v = v * 10 + (ch - '0');
                    }
                    return v;
                };
                if(semi != std::string_view::npos) {
                    code = parse_num(inner.substr(0, semi));
                    mod_raw = parse_num(inner.substr(semi + 1));
                } else {
                    code = parse_num(inner);
                }
                if(code < 0) { event.name = "unknown"; break; }

                if(mod_raw >= 2 && mod_raw <= 8) {
                    int bits = mod_raw - 1;
                    event.shift = (bits & 1) != 0;
                    event.meta = (bits & 2) != 0;
                    event.ctrl = (bits & 4) != 0;
                }

                if(code >= 0xE000 && code <= 0xE00B) {
                    event.name = "f" + std::to_string(code - 0xE000 + 1);
                } else if(code == 0xE010)      event.name = "left";
                else if(code == 0xE011)      event.name = "right";
                else if(code == 0xE012)      event.name = "up";
                else if(code == 0xE013)      event.name = "down";
                else if(code == 0xE014)      event.name = "home";
                else if(code == 0xE015)      event.name = "end";
                else if(code == 0xE016)      event.name = "insert";
                else if(code == 0xE017)      event.name = "delete";
                else if(code == 0xE018)      event.name = "pageup";
                else if(code == 0xE019)      event.name = "pagedown";
                else if(code == 27)          event.name = "escape";
                else if(code == 9)           event.name = "tab";
                else if(code == 13)          event.name = "return";
                else if(code == 127)         event.name = "backspace";
                else if(code >= 32 && code <= 126) {
                    event.name = std::string(1, static_cast<char>(code));
                    event.sequence = event.name;
                } else if(code == 8)         event.name = "backspace";
                else if(code == 10)          event.name = "return";
                else {
                    char utf8_buf[5] = {};
                    int n = encode_utf8(static_cast<char32_t>(code), utf8_buf);
                    utf8_buf[n] = '\0';
                    event.name = std::string(utf8_buf);
                    event.sequence = event.name;
                }
                // Kitty 'u' sequences carry their own modifier info.
                skip_ansi_modifier = true;
                break;
            }
            default:
                event.name = "unknown";
                break;
            }

            // Check for modifier parameters (CSI 1;2A = shift+up, etc.)
            // ANSI modifiers are 1-indexed: raw=2 means modifier bits=1
            if(!skip_ansi_modifier && len >= 5 && data[2] == '1' && data[3] == ';') {

                int mod = data[4] - '0';
                if(mod >= 2 && mod <= 8) {
                    int bits = mod - 1;
                    event.shift = (bits & 1) != 0;
                    event.meta = (bits & 2) != 0;
                    event.ctrl = (bits & 4) != 0;
                }
            }
        } else if(len == 2 && data[0] == '\033') {
            // Alt + key.
            event.name = std::string(1, data[1]);
            event.meta = true;
        } else {
            // Multi-byte UTF-8 character.
            event.name = std::string(data, len);
            event.sequence = event.name;
        }

        event.raw = std::string(data, len);
        _impl->key_signal.emit(event);
    }

    void Renderer::parse_mouse_sequence(const char* data, std::size_t len) {
        // SGR mouse: ESC[<button;x;y[Mm]
        std::string_view sv(data, len);
        if(sv.size() < 6) {
            return;
        }

        auto content = sv.substr(3); // skip ESC[<
        bool is_release = content.back() == 'm';

        // Parse button;x;y
        int parts[3] = {0, 0, 0};
        int pi = 0;
        for(std::size_t i = 0; i < content.size() - 1 && pi < 3; i++) {
            if(content[i] == ';') {
                pi++;
                continue;
            }
            if(content[i] >= '0' && content[i] <= '9') {
                parts[pi] = parts[pi] * 10 + (content[i] - '0');
            }
        }

        int button_code = parts[0];
        int mx = parts[1] - 1; // 1-indexed to 0-indexed
        int my = parts[2] - 1;

        MouseEvent event;
        event.x = mx;
        event.y = my;

        // Decode button.
        int base = button_code & 0x03;
        bool motion = (button_code & 32) != 0;
        bool scroll = (button_code & 64) != 0;

        if(scroll) {
            event.type = MouseEventType::Scroll;
            event.scroll_delta = (base == 0) ? -1 : 1; // 0=up, 1=down
        } else if(is_release) {
            event.type = MouseEventType::Up;
        } else if(motion) {
            event.type =
                (base == 3) ? MouseEventType::Move : MouseEventType::Drag;
        } else {
            event.type = MouseEventType::Down;
        }

        switch(base) {
        case 0:
            event.button = MouseButton::Left;
            break;
        case 1:
            event.button = MouseButton::Middle;
            break;
        case 2:
            event.button = MouseButton::Right;
            break;
        default:
            event.button = MouseButton::None;
            break;
        }

        event.ctrl = (button_code & 16) != 0;
        event.shift = (button_code & 4) != 0;
        event.meta = (button_code & 8) != 0;

        // Dispatch through hit grid (reverse order = top-most first).
        for(auto it = _impl->hit_grid.rbegin(); it != _impl->hit_grid.rend();
            ++it) {
            if(mx >= it->x && mx < it->x + it->w && my >= it->y &&
               my < it->y + it->h) {
                auto& reg = Renderable::registry();
                auto it2 = reg.find(it->num);
                if(it2 != reg.end()) {
                    it2->second->dispatch_mouse(event);
                    if(event.propagation_stopped()) {
                        return;
                    }
                }
            }
        }
    }

    void Renderer::clipboard_copy(std::string_view text) {
        if(_impl->terminal) {
            _impl->terminal->clipboard_copy(text);
        }
    }

    void Renderer::destroy() {
        _impl->running = false;
    }

} // namespace stain
