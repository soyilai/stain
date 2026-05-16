#pragma once

#include "stain/box.hpp"
#include "stain/renderable.hpp"

#include <memory>
#include <optional>

namespace stain {

    // Edge to which sticky scrolling attaches.
    enum class StickyEdge { Top, Bottom, Left, Right };

    // Options for creating a ScrollBox.
    struct ScrollBoxOptions {
        BoxOptions root_options{};
        BoxOptions viewport_options{};
        BoxOptions content_options{};
        bool scroll_x{false};
        bool scroll_y{true};
        // When enabled, scroll position resets to the sticky edge on layout
        // changes.
        bool sticky_scroll{false};
        std::optional<StickyEdge> sticky_edge{};
        // Skip drawing children outside the visible viewport.
        bool viewport_culling{true};
    };

    // A scrollable viewport with scrollbar indicators. Supports both X and Y
    // scrolling.
    class ScrollBox : public Renderable {
        public:
        explicit ScrollBox(RenderContext* ctx, ScrollBoxOptions opts = {});

        [[nodiscard]] static std::shared_ptr<ScrollBox>
        create(RenderContext* ctx) {
            return std::make_shared<ScrollBox>(ctx);
        }

        // Current scroll position.
        [[nodiscard]] int scroll_top() const noexcept {
            return _scroll_top;
        }
        [[nodiscard]] int scroll_left() const noexcept {
            return _scroll_left;
        }
        // Scroll to an absolute position.
        void scroll_to(int top, int left);
        // Scroll relative to current position.
        void scroll_by(int dy, int dx = 0);
        void scroll_to_top();
        void scroll_to_bottom();

        // Content dimensions (total scrollable area).
        [[nodiscard]] int content_height() const noexcept;
        [[nodiscard]] int content_width() const noexcept;

        // Add/remove/find children in the scrollable content area.
        int add(
            std::shared_ptr<BaseRenderable> child,
            std::optional<int> index = std::nullopt
        ) override;
        void remove(std::string_view id) override;
        BaseRenderable* find(std::string_view id) const override;

        // Fluent scroll behavior setters.
        ScrollBox& scroll_x(bool v);
        ScrollBox& scroll_y(bool v);
        // Skip rendering children outside the visible area.
        ScrollBox& viewport_culling(bool v);
        ScrollBox& sticky_scroll(bool v);
        ScrollBox& sticky_edge(std::optional<StickyEdge> e);

        protected:
        void draw(OptimizedBuffer& buf, double delta) override;
        void update_from_layout() override;
        void on_resize(int w, int h) override;
        void process_mouse_event(MouseEvent& event) override;
        bool handle_key_press(KeyEvent& key) override;

        private:
        void clamp_scroll();

        std::shared_ptr<Box> _viewport;
        std::shared_ptr<Box> _content;

        int _scroll_top{0};
        int _scroll_left{0};
        ScrollBoxOptions _scroll_opts;
    };

    // Factory function for a ScrollBox.
    inline std::shared_ptr<ScrollBox> scrollbox(RenderContext& ctx) {
        return ScrollBox::create(&ctx);
    }

} // namespace stain
