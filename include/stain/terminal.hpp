#pragma once

#include "stain/buffer.hpp"
#include "stain/color.hpp"
#include "stain/context.hpp"

#include <memory>
#include <string_view>

namespace stain {

    // Detected terminal capabilities.
    struct TerminalInfo {
        int cols{80};
        int rows{24};
        int pixel_width{0};
        int pixel_height{0};
        bool has_true_color{false};
        bool has_kitty_keyboard{false};
        bool has_mouse_sgr{false};
        bool has_bracketed_paste{false};
        bool has_osc_hyperlinks{false};
        bool has_osc52{false};
    };

    // RAII guard: saves terminal state on construction, restores on
    // destruction. Enters raw mode and alternate screen by default.
    class TerminalGuard {
        public:
        explicit TerminalGuard(
            bool alternate_screen = true,
            bool use_mouse = true,
            bool use_kitty_keys = true
        );
        ~TerminalGuard();

        TerminalGuard(const TerminalGuard&) = delete;
        TerminalGuard& operator=(const TerminalGuard&) = delete;

        // Detected terminal info.
        [[nodiscard]] const TerminalInfo& info() const noexcept;

        // Detect terminal capabilities from environment and queries.
        static TerminalInfo detect();

        // Write raw bytes to stdout (buffered).
        void write(std::string_view data);
        // Flush the write buffer.
        void flush();

        // ANSI escape helpers.
        void move_to(int col, int row);
        void fg(RGBA color);
        void bg(RGBA color);
        void attr(Attr a);
        void reset_attr();
        void show_cursor(bool visible);
        void cursor_style(CursorOptions opts);

        // Copy text to system clipboard via OSC52.
        void clipboard_copy(std::string_view text);

        // Update terminal dimensions (call on SIGWINCH).
        void update_size();

        private:
        struct SavedState;
        std::unique_ptr<SavedState> _saved;
        TerminalInfo _info;
        std::string _write_buf;
        bool _alternate_screen{true};
        bool _use_mouse{true};
    };

} // namespace stain
