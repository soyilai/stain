#pragma once

#include "stain/buffer.hpp"
#include "stain/color.hpp"
#include "stain/context.hpp"
#include "stain/event.hpp"
#include "stain/signal.hpp"

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

struct YGNode;
using YGNodeRef = YGNode*;

namespace stain {

    // Flexbox layout enums, mirror Yoga values.
    enum class FlexDirection { Row, RowReverse, Column, ColumnReverse };
    enum class FlexWrap { NoWrap, Wrap, WrapReverse };
    enum class Align {
        Auto,
        FlexStart,
        Center,
        FlexEnd,
        Stretch,
        Baseline,
        SpaceBetween,
        SpaceAround
    };
    enum class Justify {
        FlexStart,
        Center,
        FlexEnd,
        SpaceBetween,
        SpaceAround,
        SpaceEvenly
    };
    enum class PositionType { Relative, Absolute };
    enum class Overflow { Visible, Hidden, Scroll };
    enum class Display { Flex, None };

    // Dimension: pixel count, percentage (0-100), or auto.
    struct Pixels {
        int value;
    };
    struct Percent {
        float value;
    };
    struct Auto {};
    using Dimension = std::variant<Pixels, Percent, Auto>;

    // Convenience constructors for Dimension.
    inline Dimension px(int v) {
        return Pixels{v};
    }
    inline Dimension pct(float v) {
        return Percent{v};
    }
    inline Dimension auto_dim() {
        return Auto{};
    }

    // Edge values (top, right, bottom, left) for margin, padding, position.
    struct Edges {
        Dimension top{px(0)}, right{px(0)}, bottom{px(0)}, left{px(0)};

        static Edges all(Dimension d) {
            return {d, d, d, d};
        }
        static Edges xy(Dimension x, Dimension y) {
            return {y, x, y, x};
        }
        static Edges zero() {
            return all(px(0));
        }
    };

    // Yoga layout options for flexbox sizing and positioning.
    struct LayoutOptions {
        FlexDirection flex_direction{FlexDirection::Row};
        FlexWrap flex_wrap{FlexWrap::NoWrap};
        Align align_items{Align::Stretch};
        Align align_content{Align::FlexStart};
        Justify justify_content{Justify::FlexStart};

        float flex_grow{0.f};
        float flex_shrink{1.f};
        Dimension flex_basis{Auto{}};
        Align align_self{Align::Auto};

        Dimension width{Auto{}};
        Dimension height{Auto{}};
        Dimension min_width{Auto{}};
        Dimension max_width{Auto{}};
        Dimension min_height{Auto{}};
        Dimension max_height{Auto{}};

        Edges margin{};
        Edges padding{Auto{}, Auto{}, Auto{}, Auto{}};

        PositionType position{PositionType::Relative};
        Dimension pos_top{Auto{}}, pos_right{Auto{}};
        Dimension pos_bottom{Auto{}}, pos_left{Auto{}};

        Overflow overflow{Overflow::Hidden};

        Display display{Display::Flex};

        Dimension gap{Auto{}};
        Dimension row_gap{Auto{}};
        Dimension column_gap{Auto{}};
    };

    // Text and background colors for normal and focus/selected states.
    struct ItemStyle {
        RGBA text{RGBA::white()};
        RGBA bg{RGBA::transparent()};
    };

    // Renderable Options

    // Full options for a Renderable: layout, visibility, z-order, events.
    struct RenderableOptions : LayoutOptions {
        std::string id{};

        int z_index{0};
        bool visible{true};
        // Render to an offscreen buffer, then blit (for caching complex draws).
        bool buffered{false};
        // Request continuous render loop (for animation).
        bool live{false};
        float opacity{1.f};

        // Can receive keyboard focus.
        bool focusable{false};

        // Callback hooks for common events.
        std::function<void()> on_size_change;
        std::function<void(KeyEvent&)> on_key_down;
        std::function<void(PasteEvent&)> on_paste;
        std::function<void(MouseEvent&)> on_mouse_down;
        std::function<void(MouseEvent&)> on_mouse_up;
        std::function<void(MouseEvent&)> on_mouse_move;
        std::function<void(MouseEvent&)> on_mouse_drag;
        std::function<void(MouseEvent&)> on_mouse_scroll;
        // Run before/after the renderable's draw() call.
        std::function<void(OptimizedBuffer&, double)> render_before;
        std::function<void(OptimizedBuffer&, double)> render_after;
    };

    // BaseRenderable

    // Base for all renderable tree nodes. Non-layout, no Yoga.
    class BaseRenderable {
        public:
        explicit BaseRenderable(RenderContext* ctx, std::string id = {});
        virtual ~BaseRenderable();

        BaseRenderable(const BaseRenderable&) = delete;
        BaseRenderable& operator=(const BaseRenderable&) = delete;
        BaseRenderable(BaseRenderable&&) = delete;
        BaseRenderable& operator=(BaseRenderable&&) = delete;

        // Unique identifier for this node.
        [[nodiscard]] const std::string& id() const noexcept {
            return _id;
        }
        // Global renderable number (for hit-testing).
        [[nodiscard]] int num() const noexcept {
            return _num;
        }
        [[nodiscard]] bool visible() const noexcept {
            return _visible;
        }
        virtual BaseRenderable& visible(bool v);

        // Parent in the render tree.
        [[nodiscard]] BaseRenderable* parent() const noexcept {
            return _parent;
        }
        [[nodiscard]] bool is_destroyed() const noexcept {
            return _destroyed;
        }

        // Child management.
        virtual int
        add(std::shared_ptr<BaseRenderable> child,
            std::optional<int> index = std::nullopt) = 0;
        virtual void remove(std::string_view id) = 0;
        virtual void insert_before(
            std::shared_ptr<BaseRenderable> child,
            BaseRenderable* anchor
        ) = 0;

        // Query children.
        [[nodiscard]] virtual std::vector<BaseRenderable*> children() const = 0;
        [[nodiscard]] virtual int child_count() const noexcept = 0;
        [[nodiscard]] virtual BaseRenderable*
        find(std::string_view id) const = 0;

        virtual void request_render() = 0;
        virtual void destroy();
        virtual void destroy_recursive();

        // Subscribe/unsubscribe to a typed event.
        // Usage:
        //   auto id = node.on<events::KeyDown>([](events::KeyDown e) { ... });
        //   node.off<events::KeyDown>(id);
        template <typename EventTag>
        std::size_t on(std::function<void(EventTag)> handler) {
            return _emitter.on<EventTag>(std::move(handler));
        }

        template <typename EventTag> void off(std::size_t id) {
            _emitter.off<EventTag>(id);
        }

        protected:
        template <typename EventTag> void emit(EventTag event) {
            _emitter.emit<EventTag>(std::move(event));
        }

        RenderContext* _ctx;
        BaseRenderable* _parent{nullptr};
        Renderable* _renderable_parent{nullptr};
        bool _dirty{false};
        bool _visible{true};
        bool _destroyed{false};
        std::string _id;
        int _num;

        Emitter<
            events::Added,
            events::Removed,
            events::Resized,
            events::Focused,
            events::Blurred,
            events::LayoutChanged,
            events::KeyDown,
            events::KeyUp,
            events::MouseDown,
            events::MouseUp,
            events::MouseMove,
            events::MouseDrag,
            events::MouseScroll,
            events::Paste,
            events::SelectionChanged,
            events::ItemSelected,
            events::InputChanged,
            events::InputEntered,
            events::Submit,
            events::TextLayoutChanged>
            _emitter;

        static inline std::atomic<int> _global_counter{0};

        friend class Renderable;
    };

    // Renderable

    // A layout-aware renderable with Yoga flexbox. The primary building block.
    class Renderable : public BaseRenderable {
        public:
        explicit Renderable(RenderContext* ctx, RenderableOptions opts = {});
        ~Renderable() override;

        // Access the underlying Yoga node for advanced configuration.
        [[nodiscard]] YGNodeRef yoga_node() const noexcept {
            return _yoga_node;
        }

        // Screen-space position (includes parent offsets).
        [[nodiscard]] int screen_x() const noexcept;
        [[nodiscard]] int screen_y() const noexcept;
        // Layout size from Yoga.
        [[nodiscard]] int layout_w() const noexcept {
            return _width;
        }
        [[nodiscard]] int layout_h() const noexcept {
            return _height;
        }

        // Offset child rendering by (dx, dy) for scrolling.
        void translate(int dx, int dy);
        [[nodiscard]] int translate_x() const noexcept {
            return _translate_x;
        }
        [[nodiscard]] int translate_y() const noexcept {
            return _translate_y;
        }

        // Fluent layout setters. Each returns *this for chaining.
        Renderable& width(Dimension d);
        Renderable& height(Dimension d);
        Renderable& min_width(Dimension d);
        Renderable& max_width(Dimension d);
        Renderable& min_height(Dimension d);
        Renderable& max_height(Dimension d);
        Renderable& flex_grow(float v);
        Renderable& flex_shrink(float v);
        Renderable& flex_basis(Dimension d);
        Renderable& flex_direction(FlexDirection d);
        Renderable& flex_wrap(FlexWrap w);
        Renderable& align_items(Align a);
        Renderable& align_content(Align a);
        Renderable& align_self(Align a);
        Renderable& justify_content(Justify j);
        Renderable& margin(Edges e);
        Renderable& padding(Edges e);
        Renderable& position_type(PositionType t);
        Renderable& pos_top(Dimension d);
        Renderable& pos_left(Dimension d);
        Renderable& pos_right(Dimension d);
        Renderable& pos_bottom(Dimension d);
        Renderable& overflow(Overflow o);
        Renderable& display(Display d);
        Renderable& gap(Dimension d);
        Renderable& row_gap(Dimension d);
        Renderable& column_gap(Dimension d);
        Renderable& z_index(int z);
        Renderable& opacity(float o);
        Renderable& visible(bool v) override;
        Renderable& id(std::string id);
        Renderable& focusable(bool v);
        // Enable offscreen buffer for cached rendering.
        Renderable& buffered(bool v);

        [[nodiscard]] bool focusable() const noexcept {
            return _focusable;
        }
        [[nodiscard]] bool focused() const noexcept {
            return _focused;
        }
        // Focus or blur this renderable.
        void focus();
        void blur();

        // Enable continuous render loop (for animations).
        Renderable& live(bool live);
        [[nodiscard]] int live_count() const noexcept {
            return _live_count;
        }

        // Child management.
        int add(
            std::shared_ptr<BaseRenderable> child,
            std::optional<int> index = std::nullopt
        ) override;
        void remove(std::string_view id) override;
        void insert_before(
            std::shared_ptr<BaseRenderable> child,
            BaseRenderable* anchor
        ) override;
        [[nodiscard]] std::vector<BaseRenderable*> children() const override;
        [[nodiscard]] int child_count() const noexcept override;
        [[nodiscard]] BaseRenderable* find(std::string_view id) const override;

        void request_render() override;

        // Called on every frame when live is enabled.
        virtual void on_lifecycle_pass(double delta_time);
        // Called after Yoga layout to sync position/size.
        virtual void update_from_layout();
        // Called every frame to draw this node and its children.
        virtual void render(OptimizedBuffer& buf, double delta_time);

        void destroy() override;
        void destroy_recursive() override;

        // Global registry of all Renderable instances, keyed by num().
        static std::unordered_map<int, Renderable*>& registry();

        [[nodiscard]] const RenderableOptions& opts() const noexcept {
            return _opts;
        }

        // Dispatch a mouse event to this renderable.
        void dispatch_mouse(MouseEvent& event) {
            process_mouse_event(event);
        }

        protected:
        // Override in subclasses to draw custom content.
        virtual void draw(OptimizedBuffer& buf, double delta_time) {
            (void)buf;
            (void)delta_time;
        }

        // Called when layout size changes.
        virtual void on_resize(int w, int h) {
            (void)w;
            (void)h;
        }

        // Handle a key press. Return true if handled.
        virtual bool handle_key_press(KeyEvent& key) {
            (void)key;
            return false;
        }

        // Handle a paste event.
        virtual void handle_paste(PasteEvent& event) {
            (void)event;
        }

        // Handle a mouse event. Dispatched by the renderer hit grid.
        virtual void process_mouse_event(MouseEvent& event);

        void mark_dirty();
        void mark_clean();
        void propagate_live_count(int delta);
        void ensure_z_sorted();

        void apply_layout_options(const LayoutOptions& opts);

        template <auto YGFn, typename T>
        Renderable& yoga_set(YGNodeRef node, T v) {
            YGFn(node, v);
            mark_dirty();
            request_render();
            return *this;
        }

        RenderableOptions _opts;
        YGNodeRef _yoga_node{nullptr};

        int _x{0}, _y{0};
        int _translate_x{0}, _translate_y{0};
        int _width{0}, _height{0};

        bool _focusable{false};
        bool _focused{false};
        std::size_t _key_conn_id{0};
        std::size_t _paste_conn_id{0};

        int _live_count{0};

        std::vector<std::shared_ptr<BaseRenderable>> _children_layout_order;
        std::vector<BaseRenderable*> _children_z_order;
        bool _z_sort_dirty{false};

        std::unordered_map<std::string, BaseRenderable*> _child_map;

        std::optional<OptimizedBuffer> _frame_buffer;
    };

} // namespace stain
