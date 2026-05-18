#pragma once

#include "stain/buffer.hpp"
#include "stain/color.hpp"
#include "stain/event.hpp"
#include "stain/timeline.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>

namespace stain {

    class Renderable;
    struct TerminalInfo;

    // Text selection between two renderable positions.
    struct Selection {
        Renderable* start_renderable{nullptr};
        int start_x{0}, start_y{0};
        Renderable* end_renderable{nullptr};
        int end_x{0}, end_y{0};
    };

    // Mouse pointer shape.
    enum class MousePointer {
        Default,
        Pointer,
        Text,
        Crosshair,
        Move,
        NotAllowed
    };

    // Cursor shapes for the terminal cursor.
    enum class CursorStyle { Block, Line, Underline };

    struct CursorOptions {
        CursorStyle style{CursorStyle::Block};
        bool blinking{false};
        RGBA color{RGBA::white()};
    };

    // Interface the renderer exposes to renderables. Every Renderable holds a
    // RenderContext*. Test doubles can stub this.
    class RenderContext {
        public:
        virtual ~RenderContext() = default;

        // Terminal dimensions.
        [[nodiscard]] virtual int width() const noexcept = 0;
        [[nodiscard]] virtual int height() const noexcept = 0;

        // Schedule a render frame.
        virtual void request_render() = 0;

        // Request/drop continuous render loop (for animation, etc.).
        virtual void request_live() = 0;
        virtual void drop_live() = 0;

        // Focus a renderable. Blurs the previous focus.
        virtual void focus_renderable(Renderable* r) = 0;
        [[nodiscard]] virtual Renderable* focused() const noexcept = 0;

        // Set cursor position and visibility.
        virtual void cursor(int col, int row, bool visible) = 0;
        virtual void cursor_style(CursorOptions opts) = 0;

        // Set mouse pointer shape.
        virtual void mouse_pointer(MousePointer shape) = 0;

        // Hit-test grid for mouse event dispatch.
        virtual void add_to_hit_grid(int x, int y, int w, int h, int num) = 0;
        virtual void push_hit_scissor(ClipRect rect) = 0;
        virtual void pop_hit_scissor() = 0;
        virtual void clear_hit_grid() = 0;

        // Register/unregister for per-frame lifecycle updates.
        virtual void register_lifecycle(Renderable* r) = 0;
        virtual void unregister_lifecycle(Renderable* r) = 0;

        // Register/unregister a timeline-driven animation.
        virtual void add_timeline(std::shared_ptr<Timeline> tl) = 0;
        virtual void remove_timeline(Timeline* tl) = 0;

        // Text selection management.
        [[nodiscard]] virtual std::optional<Selection>
        get_selection() const = 0;
        virtual void clear_selection() = 0;
        virtual void start_selection(Renderable* r, int x, int y) = 0;
        virtual void update_selection(Renderable* r, int x, int y) = 0;

        // Global key handler. Returns a connection ID.
        virtual std::size_t on_key(std::function<bool(KeyEvent&)> handler) = 0;
        virtual void off_key(std::size_t id) = 0;

        // Copy text to system clipboard via OSC52.
        virtual void clipboard_copy(std::string_view text) = 0;

        // Global paste handler. Returns a connection ID.
        virtual std::size_t
        on_paste(std::function<void(PasteEvent&)> handler) = 0;
        virtual void off_paste(std::size_t id) = 0;

        // Write raw bytes to the terminal output buffer.
        virtual void write_raw(std::string_view data) = 0;

        // Queue a raw terminal write to happen after the cell flush,
        // positioned at specific cell coordinates. Used by Image widget
        // for Kitty/iTerm2 protocol sequences (must run after cells
        // are drawn so the image overlay is not overwritten).
        virtual void
        write_after_flush(int x, int y, std::string_view data) = 0;

        // Terminal capability information.
        [[nodiscard]] virtual const TerminalInfo& terminal_info() const = 0;
    };

} // namespace stain
