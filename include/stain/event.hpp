#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace stain {

    class BaseRenderable;

    // Keyboard event types.
    enum class KeyEventType { Press, Repeat, Release };

    // A keyboard event.
    // Usage:
    //   renderer.on_key([](KeyEvent& key) -> bool {
    //       if (key.name == "escape") { ... }
    //       return !key.propagation_stopped();
    //   });
    // Modifier flags (shift/ctrl/meta/super) are populated from escape sequence
    // information. In legacy terminal encoding, simple modifier+letter combos
    // (e.g. Shift+A, Ctrl+1) are sent as the resulting character byte without
    // any modifier flag. The flags are reliably set only when the terminal uses
    // the Kitty keyboard protocol (mode 1+ or CSI u sequences) or for ANSI
    // functional-key sequences (arrow keys, etc.).
    struct KeyEvent {
        std::string name; // "a", "up", "return", "f1", etc.
        bool ctrl{false};
        bool shift{false};
        bool meta{false};   // Alt / Option
        bool option{false}; // Explicit ANSI modifier bit
        bool super_{false};
        bool repeated{false};
        KeyEventType type{KeyEventType::Press};
        std::string sequence;  // Printable character or sequence
        std::string raw;       // Raw escape bytes
        std::string source;    // "raw" or "kitty"
        uint32_t base_code{0}; // Physical key codepoint (Kitty protocol)

        void prevent_default() noexcept {
            _default_prevented = true;
        }
        void stop_propagation() noexcept {
            _propagation_stopped = true;
        }
        [[nodiscard]] bool default_prevented() const noexcept {
            return _default_prevented;
        }
        [[nodiscard]] bool propagation_stopped() const noexcept {
            return _propagation_stopped;
        }

        private:
        bool _default_prevented{false};
        bool _propagation_stopped{false};
    };

    enum class MouseButton { Left, Middle, Right, None };
    enum class MouseEventType { Down, Up, Move, Drag, Scroll };

    // A mouse event. Coordinates are relative to the terminal grid.
    struct MouseEvent {
        MouseEventType type{MouseEventType::Move};
        MouseButton button{MouseButton::None};
        int x{0}, y{0};
        int scroll_delta{0}; // +1 down, -1 up
        bool ctrl{false};
        bool shift{false};
        bool meta{false};

        void stop_propagation() noexcept {
            _propagation_stopped = true;
        }
        [[nodiscard]] bool propagation_stopped() const noexcept {
            return _propagation_stopped;
        }

        private:
        bool _propagation_stopped{false};
    };

    // A paste event from bracketed paste mode.
    struct PasteEvent {
        std::string text;
        std::vector<uint8_t> raw_bytes;
    };

    // Typed event tags used with Renderable::on<EventTag>(handler).
    //
    // Usage:
    //   auto id = renderable.on<events::KeyDown>([](events::KeyDown e) {
    //       if (e.key.name == "escape") { ... }
    //   });

    namespace events {
        // A child was added.
        struct Added {
            BaseRenderable* child;
        };
        // A child was removed by id.
        struct Removed {
            std::string id;
        };
        // Renderable was resized.
        struct Resized {
            int w, h;
        };
        // Renderable gained focus.
        struct Focused {};
        // Renderable lost focus.
        struct Blurred {};
        // Yoga layout changed.
        struct LayoutChanged {};
        // Key pressed.
        struct KeyDown {
            ::stain::KeyEvent key;
        };
        // Key released.
        struct KeyUp {
            ::stain::KeyEvent key;
        };
        // Mouse button pressed.
        struct MouseDown {
            ::stain::MouseEvent event;
        };
        // Mouse button released.
        struct MouseUp {
            ::stain::MouseEvent event;
        };
        // Mouse moved.
        struct MouseMove {
            ::stain::MouseEvent event;
        };
        // Mouse dragged (button held + move).
        struct MouseDrag {
            ::stain::MouseEvent event;
        };
        // Mouse wheel scrolled.
        struct MouseScroll {
            ::stain::MouseEvent event;
        };
        // Text pasted.
        struct Paste {
            ::stain::PasteEvent event;
        };
        // Selection changed in a select widget.
        struct SelectionChanged {
            int index;
            std::string label;
            std::string value;
        };
        // An item was activated/selected in a select widget.
        struct ItemSelected {
            int index;
            std::string label;
            std::string value;
        };
        // Input value changed.
        struct InputChanged {
            std::string value;
        };
        // Input enter key pressed.
        struct InputEntered {
            std::string value;
        };
        // Form submitted.
        struct Submit {
            std::string value;
        };
        // Text layout changed (word wrap, etc.).
        struct TextLayoutChanged {};
    } // namespace events

} // namespace stain
