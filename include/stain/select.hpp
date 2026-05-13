#pragma once

#include "stain/keybind.hpp"
#include "stain/renderable.hpp"

#include <functional>
#include <string>
#include <vector>

namespace stain {

    // An option in a Select widget.
    struct SelectOption {
        std::string label;
        std::string value;
    };

    // Default key bindings for Select and TabSelect.
    extern const std::vector<KeyBinding> default_select_bindings;

    struct SelectOptions : RenderableOptions {
        std::vector<SelectOption> options{};
        int selected_index{0};
        ItemStyle style{};
        ItemStyle selected_style{RGBA::white(), RGBA{0, 0.4f, 0.8f, 1}};
        std::vector<KeyBinding> key_bindings{};
        KeyAliasMap key_aliases{};
        std::function<void(int, const SelectOption&)> on_selection_changed;
        std::function<void(int, const SelectOption&)> on_item_selected;
    };

    // A vertical list of selectable items with keyboard navigation.
    class Select : public Renderable {
        public:
        explicit Select(RenderContext* ctx, SelectOptions opts = {});

        [[nodiscard]] static std::shared_ptr<Select>
        create(RenderContext* ctx) {
            return std::make_shared<Select>(ctx);
        }

        // Currently selected index and option.
        [[nodiscard]] int selected_index() const noexcept;
        [[nodiscard]] const SelectOption* selected_option() const noexcept;
        // Navigate the selection.
        void move_up(int steps = 1);
        void move_down(int steps = 1);
        // Activate the current selection (fires on_item_selected).
        void select_current();

        // Fluent setters.
        Select& options(std::vector<SelectOption> opts);
        Select& selected_index(int idx);
        Select& style(ItemStyle s);
        Select& selected_style(ItemStyle s);
        // Fired when the selection highlight changes.
        Select& on_select(std::function<void(int, const SelectOption&)> f);
        // Fired when an item is activated (enter key or click).
        Select& on_activate(std::function<void(int, const SelectOption&)> f);

        protected:
        void draw(OptimizedBuffer& buf, double delta) override;
        bool handle_key_press(KeyEvent& key) override;
        void on_resize(int w, int h) override;
        void process_mouse_event(MouseEvent& event) override;

        private:
        SelectOptions _select_opts;
        int _selected_index{0};
        int _scroll_offset{0};
        std::unordered_map<std::string, std::string> _binding_map;
    };

    // Factory function for Select.
    inline std::shared_ptr<Select> select(RenderContext& ctx) {
        return Select::create(&ctx);
    }

    // An option in a TabSelect widget.
    struct TabOption {
        std::string label;
        std::string value;
        // Tab width in cells. 0 = auto from label length.
        int tab_width{0};
    };

    struct TabSelectOptions : RenderableOptions {
        std::vector<TabOption> options{};
        int selected_index{0};
        ItemStyle style{};
        ItemStyle selected_style{RGBA::white(), RGBA{0, 0.4f, 0.8f, 1}};
        std::vector<KeyBinding> key_bindings{};
        KeyAliasMap key_aliases{};
        std::function<void(int, const TabOption&)> on_selection_changed;
        std::function<void(int, const TabOption&)> on_item_selected;
    };

    // A horizontal tab bar with left/right navigation.
    class TabSelect : public Renderable {
        public:
        explicit TabSelect(RenderContext* ctx, TabSelectOptions opts = {});

        [[nodiscard]] static std::shared_ptr<TabSelect>
        create(RenderContext* ctx) {
            return std::make_shared<TabSelect>(ctx);
        }

        // Currently selected tab index.
        [[nodiscard]] int selected_index() const noexcept;
        void move_left();
        void move_right();
        void select_current();

        TabSelect& options(std::vector<TabOption> opts);
        TabSelect& selected_index(int idx);
        TabSelect& style(ItemStyle s);
        TabSelect& selected_style(ItemStyle s);
        TabSelect& on_select(std::function<void(int, const TabOption&)> f);
        TabSelect& on_activate(std::function<void(int, const TabOption&)> f);

        protected:
        void draw(OptimizedBuffer& buf, double delta) override;
        bool handle_key_press(KeyEvent& key) override;
        void process_mouse_event(MouseEvent& event) override;

        private:
        TabSelectOptions _tab_opts;
        int _selected_index{0};
        std::unordered_map<std::string, std::string> _binding_map;
    };

    inline std::shared_ptr<TabSelect> tab_select(RenderContext& ctx) {
        return TabSelect::create(&ctx);
    }
    inline std::shared_ptr<TabSelect> tabselect(RenderContext& ctx) {
        return TabSelect::create(&ctx);
    }

} // namespace stain
