#pragma once

#include "stain/keybind.hpp"
#include "stain/renderable.hpp"

#include <deque>
#include <string>
#include <vector>

namespace stain {

    // Abstract text storage interface. Implement to provide custom backing.
    // TODO: Add rope-based storage implementation for better performance on
    // large texts. The Textarea/EditBuffer APIs should be designed to allow
    // pluggable storage backends.
    class TextStorage {
        public:
        virtual ~TextStorage() = default;
        [[nodiscard]] virtual std::string text() const = 0;
        [[nodiscard]] virtual int length() const noexcept = 0;
        virtual void text(std::string_view text) = 0;
        virtual void insert(int offset, std::string_view text) = 0;
        virtual void erase(int start, int end) = 0;
    };

    // std::string-backed TextStorage.
    class StringStorage : public TextStorage {
        public:
        [[nodiscard]] std::string text() const override {
            return _data;
        }
        [[nodiscard]] int length() const noexcept override {
            return static_cast<int>(_data.size());
        }
        void text(std::string_view text) override {
            _data = std::string(text);
        }
        void insert(int offset, std::string_view text) override {
            _data.insert(static_cast<std::size_t>(offset), text);
        }
        void erase(int start, int end) override {
            _data.erase(
                static_cast<std::size_t>(start),
                static_cast<std::size_t>(end - start)
            );
        }

        private:
        std::string _data;
    };

    // Text buffer with cursor, undo/redo, and selection support.
    // Used internally by Textarea and Input.

    struct UndoEntry {
        std::string text;
        int cursor;
    };

    class EditBuffer {
        public:
        explicit EditBuffer();

        // Full text content.
        [[nodiscard]] std::string plain_text() const;
        [[nodiscard]] int length() const noexcept;
        // Current cursor position as byte offset.
        [[nodiscard]] int cursor_offset() const noexcept;

        // Replace all text.
        void text(std::string_view text);
        // Insert text at cursor position.
        void insert_at_cursor(std::string_view text);
        void delete_char_backward();
        void delete_char_forward();
        void delete_word_backward();
        void delete_to_line_end();

        // Move cursor by byte offset.
        void move_cursor(int offset);
        void move_cursor_to(int offset);
        void move_cursor_left();
        void move_cursor_right();
        void move_word_forward();
        void move_word_backward();
        void move_line_start();
        void move_line_end();
        void move_line_up();
        void move_line_down();
        // Cursor column index (0-based).
        [[nodiscard]] int cursor_column() const;

        void undo();
        void redo();

        // Selection range [start, end). start == end means no selection.
        [[nodiscard]] bool has_selection() const noexcept;
        void select(int start, int end);
        void clear_selection();
        [[nodiscard]] std::pair<int, int> selection() const;
        [[nodiscard]] std::string selected_text() const;
        void delete_selection();

        private:
        void save_undo();
        std::unique_ptr<TextStorage> _storage;
        int _cursor{0};
        int _sel_start{-1}, _sel_end{-1};
        std::deque<UndoEntry> _undo_stack;
        std::deque<UndoEntry> _redo_stack;
    };

    // Default key bindings for Textarea/Input.
    extern const std::vector<KeyBinding> default_textarea_bindings;

    struct TextareaOptions : RenderableOptions {
        std::string placeholder{};
        ItemStyle style{};
        ItemStyle focus_style{};
        // Custom key bindings merged over defaults.
        std::vector<KeyBinding> key_bindings{};
        KeyAliasMap key_aliases{};
        std::function<void(std::string_view)> on_submit;
    };

    // Multi-line text input area with cursor, undo/redo, and selection.
    class Textarea : public Renderable {
        public:
        explicit Textarea(RenderContext* ctx, TextareaOptions opts = {});

        // Create a Textarea with default options.
        [[nodiscard]] static std::shared_ptr<Textarea>
        create(RenderContext* ctx) {
            return std::make_shared<Textarea>(ctx);
        }

        // Get or set the full text.
        [[nodiscard]] std::string plain_text() const;
        Textarea& text(std::string_view text);
        // Insert text at the cursor position.
        Textarea& insert_text(std::string_view text);

        // Access the underlying edit buffer.
        [[nodiscard]] EditBuffer& edit_buffer() noexcept;
        [[nodiscard]] const EditBuffer& edit_buffer() const noexcept;

        // Set placeholder text (shown when empty and not focused).
        Textarea& placeholder(std::string_view v);
        Textarea& style(ItemStyle s);
        Textarea& focus_style(ItemStyle s);
        Textarea& key_bindings(std::vector<KeyBinding> b);
        Textarea& on_submit(std::function<void(std::string_view)> f);

        protected:
        void draw(OptimizedBuffer& buf, double delta) override;
        bool handle_key_press(KeyEvent& key) override;
        void handle_paste(PasteEvent& event) override;
        void process_mouse_event(MouseEvent& event) override;
        virtual bool new_line();

        private:
        void adjust_h_scroll();
        void adjust_v_scroll();
        [[nodiscard]] int byte_offset_at(int cell_x, int cell_y) const;
        [[nodiscard]] int cursor_line() const;
        TextareaOptions _textarea_opts;
        EditBuffer _edit_buffer;
        int _h_scroll{0};
        int _v_scroll{0};
        bool _mouse_selecting{false};
        std::unordered_map<std::string, std::string> _binding_map;
    };

    // Factory function for Textarea.
    inline std::shared_ptr<Textarea> textarea(RenderContext& ctx) {
        return Textarea::create(&ctx);
    }

    struct InputOptions : TextareaOptions {
        std::string value{};
        int max_length{0};
        std::function<void(std::string_view)> on_input;
        std::function<void(std::string_view)> on_change;
        std::function<void(std::string_view)> on_enter;
    };

    // Single-line text input. Extends Textarea with value(), max_length, and
    // input/change/enter callbacks. Newlines are filtered from pastes.
    class Input : public Textarea {
        public:
        explicit Input(RenderContext* ctx, InputOptions opts = {});

        [[nodiscard]] static std::shared_ptr<Input> create(RenderContext* ctx) {
            return std::make_shared<Input>(ctx);
        }

        // Current input value.
        [[nodiscard]] std::string value() const;
        Input& value(std::string_view v);
        Input& max_length(int n);
        // Fired on every keystroke.
        Input& on_input(std::function<void(std::string_view)> f);
        // Fired when value changes (after keystroke processing).
        Input& on_change(std::function<void(std::string_view)> f);
        // Fired when Enter is pressed.
        Input& on_enter(std::function<void(std::string_view)> f);

        protected:
        bool handle_key_press(KeyEvent& key) override;
        void handle_paste(PasteEvent& event) override;
        bool new_line() override {
            return false;
        }

        private:
        InputOptions _input_opts;
        std::string _last_committed_value;
    };

    // Factory function for Input.
    inline std::shared_ptr<Input> input(RenderContext& ctx) {
        return Input::create(&ctx);
    }

} // namespace stain
