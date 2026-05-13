#include "stain/terminal.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace stain {

    // SavedState, holds original termios

    struct TerminalGuard::SavedState {
        struct termios orig_termios;
        bool have_termios{false};
    };

    // TerminalGuard

    TerminalGuard::TerminalGuard(
        bool alternate_screen,
        bool use_mouse,
        bool use_kitty_keys
    )
        : _saved(std::make_unique<SavedState>())
        , _alternate_screen(alternate_screen)
        , _use_mouse(use_mouse) {

        _info = detect();

        // Save original terminal settings.
        if(tcgetattr(STDIN_FILENO, &_saved->orig_termios) == 0) {
            _saved->have_termios = true;

            // Enter raw mode.
            struct termios raw = _saved->orig_termios;
            raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
            raw.c_oflag &= ~(OPOST);
            raw.c_cflag |= CS8;
            raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        }

        // Enter alternate screen.
        if(_alternate_screen) {
            write("\033[?1049h");
        }

        // Hide cursor initially.
        write("\033[?25l");

        // Enable mouse tracking (SGR mode).
        if(_use_mouse) {
            write("\033[?1000h"); // button events
            write("\033[?1002h"); // button + motion events
            write("\033[?1003h"); // all motion events
            write("\033[?1006h"); // SGR encoding
        }

        // Enable bracketed paste.
        write("\033[?2004h");

        // Enable Kitty keyboard protocol.
        if(use_kitty_keys) {
            write("\033[>1u");
        }

        flush();
    }

    TerminalGuard::~TerminalGuard() {
        // Disable bracketed paste.
        write("\033[?2004l");

        // Disable mouse.
        if(_use_mouse) {
            write("\033[?1006l");
            write("\033[?1003l");
            write("\033[?1002l");
            write("\033[?1000l");
        }

        // Disable Kitty keyboard protocol.
        write("\033[<1u");

        // Show cursor.
        write("\033[?25h");

        // Reset attributes.
        write("\033[0m");

        // Leave alternate screen.
        if(_alternate_screen) {
            write("\033[?1049l");
        }

        flush();

        // Restore terminal settings.
        if(_saved && _saved->have_termios) {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &_saved->orig_termios);
        }
    }

    const TerminalInfo& TerminalGuard::info() const noexcept {
        return _info;
    }

    TerminalInfo TerminalGuard::detect() {
        TerminalInfo info;

        // Get terminal size.
        struct winsize ws{};
        if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
            info.cols = ws.ws_col;
            info.rows = ws.ws_row;
            info.pixel_width = ws.ws_xpixel;
            info.pixel_height = ws.ws_ypixel;
        }

        // Check for true color support.
        const char* colorterm = std::getenv("COLORTERM");
        if(colorterm && (std::strcmp(colorterm, "truecolor") == 0 ||
                         std::strcmp(colorterm, "24bit") == 0)) {
            info.has_true_color = true;
        }

        // Check for Kitty keyboard protocol support via TERM.
        const char* term = std::getenv("TERM");
        if(term && std::strstr(term, "kitty")) {
            info.has_kitty_keyboard = true;
        }

        info.has_mouse_sgr =
            true; // Assume SGR mouse support (widely available).
        info.has_bracketed_paste = true;

        return info;
    }

    void TerminalGuard::write(std::string_view data) {
        _write_buf.append(data);
    }

    void TerminalGuard::flush() {
        if(_write_buf.empty())
            return;
        ::write(STDOUT_FILENO, _write_buf.data(), _write_buf.size());
        _write_buf.clear();
    }

    void TerminalGuard::move_to(int col, int row) {
        char buf[32];
        int n =
            std::snprintf(buf, sizeof(buf), "\033[%d;%dH", row + 1, col + 1);
        write(std::string_view(buf, static_cast<std::size_t>(n)));
    }

    void TerminalGuard::fg(RGBA color) {
        char buf[32];
        int n = std::snprintf(
            buf,
            sizeof(buf),
            "\033[38;2;%d;%d;%dm",
            color.r8(),
            color.g8(),
            color.b8()
        );
        write(std::string_view(buf, static_cast<std::size_t>(n)));
    }

    void TerminalGuard::bg(RGBA color) {
        char buf[32];
        int n = std::snprintf(
            buf,
            sizeof(buf),
            "\033[48;2;%d;%d;%dm",
            color.r8(),
            color.g8(),
            color.b8()
        );
        write(std::string_view(buf, static_cast<std::size_t>(n)));
    }

    void TerminalGuard::attr(Attr a) {
        if(has_attr(a, Attr::Bold))
            write("\033[1m");
        if(has_attr(a, Attr::Dim))
            write("\033[2m");
        if(has_attr(a, Attr::Italic))
            write("\033[3m");
        if(has_attr(a, Attr::Underline))
            write("\033[4m");
        if(has_attr(a, Attr::Blink))
            write("\033[5m");
        if(has_attr(a, Attr::Reverse))
            write("\033[7m");
        if(has_attr(a, Attr::Strike))
            write("\033[9m");
    }

    void TerminalGuard::reset_attr() {
        write("\033[0m");
    }

    void TerminalGuard::show_cursor(bool visible) {
        write(visible ? "\033[?25h" : "\033[?25l");
    }

    void TerminalGuard::cursor_style(CursorOptions opts) {
        // DECSCUSR: 1=block blink, 2=block steady, 3=underline blink,
        // 4=underline steady,
        //           5=bar blink, 6=bar steady
        int code = 2; // default: block steady
        switch(opts.style) {
        case CursorStyle::Block:
            code = opts.blinking ? 1 : 2;
            break;
        case CursorStyle::Underline:
            code = opts.blinking ? 3 : 4;
            break;
        case CursorStyle::Line:
            code = opts.blinking ? 5 : 6;
            break;
        }
        char buf[16];
        int n = std::snprintf(buf, sizeof(buf), "\033[%d q", code);
        write(std::string_view(buf, static_cast<std::size_t>(n)));
    }

    void TerminalGuard::update_size() {
        struct winsize ws{};
        if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
            _info.cols = ws.ws_col;
            _info.rows = ws.ws_row;
            _info.pixel_width = ws.ws_xpixel;
            _info.pixel_height = ws.ws_ypixel;
        }
    }

} // namespace stain
