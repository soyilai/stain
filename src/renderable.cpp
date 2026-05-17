#include "stain/renderable.hpp"

#include <algorithm>
#include <cassert>
#include <yoga/Yoga.h>

namespace stain {

    // Yoga enum mapping helpers

    namespace {

        YGFlexDirection to_yg(FlexDirection d) {
            switch(d) {
            case FlexDirection::Row:
                return YGFlexDirectionRow;
            case FlexDirection::RowReverse:
                return YGFlexDirectionRowReverse;
            case FlexDirection::Column:
                return YGFlexDirectionColumn;
            case FlexDirection::ColumnReverse:
                return YGFlexDirectionColumnReverse;
            }
            return YGFlexDirectionRow;
        }

        YGWrap to_yg(FlexWrap w) {
            switch(w) {
            case FlexWrap::NoWrap:
                return YGWrapNoWrap;
            case FlexWrap::Wrap:
                return YGWrapWrap;
            case FlexWrap::WrapReverse:
                return YGWrapWrapReverse;
            }
            return YGWrapNoWrap;
        }

        YGAlign to_yg(Align a) {
            switch(a) {
            case Align::Auto:
                return YGAlignAuto;
            case Align::FlexStart:
                return YGAlignFlexStart;
            case Align::Center:
                return YGAlignCenter;
            case Align::FlexEnd:
                return YGAlignFlexEnd;
            case Align::Stretch:
                return YGAlignStretch;
            case Align::Baseline:
                return YGAlignBaseline;
            case Align::SpaceBetween:
                return YGAlignSpaceBetween;
            case Align::SpaceAround:
                return YGAlignSpaceAround;
            }
            return YGAlignAuto;
        }

        YGJustify to_yg(Justify j) {
            switch(j) {
            case Justify::FlexStart:
                return YGJustifyFlexStart;
            case Justify::Center:
                return YGJustifyCenter;
            case Justify::FlexEnd:
                return YGJustifyFlexEnd;
            case Justify::SpaceBetween:
                return YGJustifySpaceBetween;
            case Justify::SpaceAround:
                return YGJustifySpaceAround;
            case Justify::SpaceEvenly:
                return YGJustifySpaceEvenly;
            }
            return YGJustifyFlexStart;
        }

        YGPositionType to_yg(PositionType p) {
            switch(p) {
            case PositionType::Relative:
                return YGPositionTypeRelative;
            case PositionType::Absolute:
                return YGPositionTypeAbsolute;
            }
            return YGPositionTypeRelative;
        }

        YGOverflow to_yg(Overflow o) {
            switch(o) {
            case Overflow::Visible:
                return YGOverflowVisible;
            case Overflow::Hidden:
                return YGOverflowHidden;
            case Overflow::Scroll:
                return YGOverflowScroll;
            }
            return YGOverflowVisible;
        }

        YGDisplay to_yg(Display d) {
            switch(d) {
            case Display::Flex:
                return YGDisplayFlex;
            case Display::None:
                return YGDisplayNone;
            }
            return YGDisplayFlex;
        }

        // Apply a Dimension to a Yoga style setter trio.
        using SetPx = void (*)(YGNodeRef, float);
        using SetPct = void (*)(YGNodeRef, float);
        using SetAuto = void (*)(YGNodeRef);

        void apply_dim(
            YGNodeRef node,
            const Dimension& d,
            SetPx spx,
            SetPct spct,
            SetAuto sauto
        ) {
            std::visit(
                [&](auto&& v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr(std::is_same_v<T, Pixels>) {
                        if(spx)
                            spx(node, static_cast<float>(v.value));
                    } else if constexpr(std::is_same_v<T, Percent>) {
                        if(spct)
                            spct(node, v.value);
                    } else if constexpr(std::is_same_v<T, Auto>) {
                        if(sauto)
                            sauto(node);
                    }
                },
                d
            );
        }

        // Apply a Dimension to an edge setter.
        using SetEdgePx = void (*)(YGNodeRef, YGEdge, float);
        using SetEdgePct = void (*)(YGNodeRef, YGEdge, float);
        using SetEdgeAuto = void (*)(YGNodeRef, YGEdge);

        void apply_edge_dim(
            YGNodeRef node,
            YGEdge edge,
            const Dimension& d,
            SetEdgePx spx,
            SetEdgePct spct,
            SetEdgeAuto sauto
        ) {
            std::visit(
                [&](auto&& v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr(std::is_same_v<T, Pixels>) {
                        if(spx)
                            spx(node, edge, static_cast<float>(v.value));
                    } else if constexpr(std::is_same_v<T, Percent>) {
                        if(spct)
                            spct(node, edge, v.value);
                    } else if constexpr(std::is_same_v<T, Auto>) {
                        if(sauto)
                            sauto(node, edge);
                    }
                },
                d
            );
        }

        // Apply Edges to margin/padding setters.
        void apply_edges_margin(YGNodeRef node, const Edges& e) {
            struct {
                YGEdge edge;
                Dimension dim;
            } edges[4] = {
                {YGEdgeTop, e.top},
                {YGEdgeRight, e.right},
                {YGEdgeBottom, e.bottom},
                {YGEdgeLeft, e.left},
            };
            for(auto& ed : edges) {
                apply_edge_dim(
                    node,
                    ed.edge,
                    ed.dim,
                    YGNodeStyleSetMargin,
                    YGNodeStyleSetMarginPercent,
                    YGNodeStyleSetMarginAuto
                );
            }
        }

        void apply_edges_padding(YGNodeRef node, const Edges& e) {
            auto set_pad = [&](YGEdge edge, const Dimension& d) {
                std::visit(
                    [&](auto&& v) {
                        using T = std::decay_t<decltype(v)>;
                        if constexpr(std::is_same_v<T, Pixels>) {
                            YGNodeStyleSetPadding(
                                node,
                                edge,
                                static_cast<float>(v.value)
                            );
                        } else if constexpr(std::is_same_v<T, Percent>) {
                            YGNodeStyleSetPaddingPercent(node, edge, v.value);
                        }
                    },
                    d
                );
            };
            struct {
                YGEdge edge;
                Dimension dim;
            } edges[4] = {
                {YGEdgeTop, e.top},
                {YGEdgeRight, e.right},
                {YGEdgeBottom, e.bottom},
                {YGEdgeLeft, e.left},
            };
            for(auto& ed : edges) {
                set_pad(ed.edge, ed.dim);
            }
        }

        void apply_gap_dim(
            YGNodeRef node,
            YGGutter gutter,
            const Dimension& d
        ) {
            std::visit(
                [&](auto&& v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr(std::is_same_v<T, Pixels>) {
                        YGNodeStyleSetGap(
                            node,
                            gutter,
                            static_cast<float>(v.value)
                        );
                    } else if constexpr(std::is_same_v<T, Percent>) {
                        YGNodeStyleSetGapPercent(node, gutter, v.value);
                    }
                    // No-op for gap.
                },
                d
            );
        }

        std::string generate_id(int num) {
            return "r" + std::to_string(num);
        }

    } // anonymous namespace

    // BaseRenderable

    BaseRenderable::BaseRenderable(RenderContext* ctx, std::string id)
        : _ctx(ctx)
        , _num(_global_counter++) {
        _id = id.empty() ? generate_id(_num) : std::move(id);
    }

    BaseRenderable::~BaseRenderable() = default;

    BaseRenderable& BaseRenderable::visible(bool v) {
        _visible = v;
        return *this;
    }

    void BaseRenderable::destroy() {
        _destroyed = true;
    }

    void BaseRenderable::destroy_recursive() {
        destroy();
    }

    // Renderable

    std::unordered_map<int, Renderable*>& Renderable::registry() {
        static std::unordered_map<int, Renderable*> reg;
        return reg;
    }

    Renderable::Renderable(RenderContext* ctx, RenderableOptions opts)
        : BaseRenderable(ctx, opts.id)
        , _opts(std::move(opts)) {
        _yoga_node = YGNodeNew();
        YGNodeSetContext(_yoga_node, this);

        _focusable = _opts.focusable;
        _visible = _opts.visible;

        apply_layout_options(_opts);

        // FlexShrink default: 0 when explicit pixel width or height.
        if(std::holds_alternative<Pixels>(_opts.width) ||
           std::holds_alternative<Pixels>(_opts.height)) {
            if(_opts.flex_shrink == 1.f) {
                YGNodeStyleSetFlexShrink(_yoga_node, 0.f);
            }
        }

        if(!_opts.visible) {
            YGNodeStyleSetDisplay(_yoga_node, YGDisplayNone);
        }

        registry()[_num] = this;

        if(_opts.live) {
            live(true);
        }
    }

    Renderable::~Renderable() {
        registry().erase(_num);
        if(_yoga_node) {
            YGNodeFree(_yoga_node);
            _yoga_node = nullptr;
        }
    }

    void Renderable::apply_layout_options(const LayoutOptions& lo) {
        auto n = _yoga_node;

        YGNodeStyleSetFlexDirection(n, to_yg(lo.flex_direction));
        YGNodeStyleSetFlexWrap(n, to_yg(lo.flex_wrap));
        YGNodeStyleSetAlignItems(n, to_yg(lo.align_items));
        YGNodeStyleSetAlignContent(n, to_yg(lo.align_content));
        YGNodeStyleSetJustifyContent(n, to_yg(lo.justify_content));

        YGNodeStyleSetFlexGrow(n, lo.flex_grow);
        YGNodeStyleSetFlexShrink(n, lo.flex_shrink);
        apply_dim(
            n,
            lo.flex_basis,
            YGNodeStyleSetFlexBasis,
            YGNodeStyleSetFlexBasisPercent,
            YGNodeStyleSetFlexBasisAuto
        );
        YGNodeStyleSetAlignSelf(n, to_yg(lo.align_self));

        apply_dim(
            n,
            lo.width,
            YGNodeStyleSetWidth,
            YGNodeStyleSetWidthPercent,
            YGNodeStyleSetWidthAuto
        );
        apply_dim(
            n,
            lo.height,
            YGNodeStyleSetHeight,
            YGNodeStyleSetHeightPercent,
            YGNodeStyleSetHeightAuto
        );
        apply_dim(
            n,
            lo.min_width,
            YGNodeStyleSetMinWidth,
            YGNodeStyleSetMinWidthPercent,
            nullptr
        );
        apply_dim(
            n,
            lo.max_width,
            YGNodeStyleSetMaxWidth,
            YGNodeStyleSetMaxWidthPercent,
            nullptr
        );
        apply_dim(
            n,
            lo.min_height,
            YGNodeStyleSetMinHeight,
            YGNodeStyleSetMinHeightPercent,
            nullptr
        );
        apply_dim(
            n,
            lo.max_height,
            YGNodeStyleSetMaxHeight,
            YGNodeStyleSetMaxHeightPercent,
            nullptr
        );

        apply_edges_margin(n, lo.margin);
        apply_edges_padding(n, lo.padding);

        YGNodeStyleSetPositionType(n, to_yg(lo.position));
        apply_edge_dim(
            n,
            YGEdgeTop,
            lo.pos_top,
            YGNodeStyleSetPosition,
            YGNodeStyleSetPositionPercent,
            YGNodeStyleSetPositionAuto
        );
        apply_edge_dim(
            n,
            YGEdgeRight,
            lo.pos_right,
            YGNodeStyleSetPosition,
            YGNodeStyleSetPositionPercent,
            YGNodeStyleSetPositionAuto
        );
        apply_edge_dim(
            n,
            YGEdgeBottom,
            lo.pos_bottom,
            YGNodeStyleSetPosition,
            YGNodeStyleSetPositionPercent,
            YGNodeStyleSetPositionAuto
        );
        apply_edge_dim(
            n,
            YGEdgeLeft,
            lo.pos_left,
            YGNodeStyleSetPosition,
            YGNodeStyleSetPositionPercent,
            YGNodeStyleSetPositionAuto
        );

        YGNodeStyleSetOverflow(n, to_yg(lo.overflow));
        YGNodeStyleSetDisplay(n, to_yg(lo.display));

        apply_gap_dim(n, YGGutterAll, lo.gap);
        apply_gap_dim(n, YGGutterRow, lo.row_gap);
        apply_gap_dim(n, YGGutterColumn, lo.column_gap);
    }

    int Renderable::screen_x() const noexcept {
        int sx = _x + _translate_x;
        if(_renderable_parent) {
            sx += _renderable_parent->screen_x();
        }
        return sx;
    }

    int Renderable::screen_y() const noexcept {
        int sy = _y + _translate_y;
        if(_renderable_parent) {
            sy += _renderable_parent->screen_y();
        }
        return sy;
    }

    void Renderable::translate(int dx, int dy) {
        _translate_x = dx;
        _translate_y = dy;
        request_render();
    }

    Renderable& Renderable::width(Dimension d) {
        apply_dim(
            _yoga_node,
            d,
            YGNodeStyleSetWidth,
            YGNodeStyleSetWidthPercent,
            YGNodeStyleSetWidthAuto
        );
        if(std::holds_alternative<Pixels>(d))
            YGNodeStyleSetFlexShrink(_yoga_node, 0.f);
        mark_dirty();
        request_render();
        return *this;
    }

    Renderable& Renderable::height(Dimension d) {
        apply_dim(
            _yoga_node,
            d,
            YGNodeStyleSetHeight,
            YGNodeStyleSetHeightPercent,
            YGNodeStyleSetHeightAuto
        );
        if(std::holds_alternative<Pixels>(d))
            YGNodeStyleSetFlexShrink(_yoga_node, 0.f);
        mark_dirty();
        request_render();
        return *this;
    }

    Renderable& Renderable::min_width(Dimension d) {
        apply_dim(
            _yoga_node,
            d,
            YGNodeStyleSetMinWidth,
            YGNodeStyleSetMinWidthPercent,
            nullptr
        );
        mark_dirty();
        request_render();
        return *this;
    }

    Renderable& Renderable::max_width(Dimension d) {
        apply_dim(
            _yoga_node,
            d,
            YGNodeStyleSetMaxWidth,
            YGNodeStyleSetMaxWidthPercent,
            nullptr
        );
        mark_dirty();
        request_render();
        return *this;
    }

    Renderable& Renderable::min_height(Dimension d) {
        apply_dim(
            _yoga_node,
            d,
            YGNodeStyleSetMinHeight,
            YGNodeStyleSetMinHeightPercent,
            nullptr
        );
        mark_dirty();
        request_render();
        return *this;
    }

    Renderable& Renderable::max_height(Dimension d) {
        apply_dim(
            _yoga_node,
            d,
            YGNodeStyleSetMaxHeight,
            YGNodeStyleSetMaxHeightPercent,
            nullptr
        );
        mark_dirty();
        request_render();
        return *this;
    }

    Renderable& Renderable::flex_grow(float v) {
        return yoga_set<YGNodeStyleSetFlexGrow>(_yoga_node, v);
    }
    Renderable& Renderable::flex_shrink(float v) {
        return yoga_set<YGNodeStyleSetFlexShrink>(_yoga_node, v);
    }
    Renderable& Renderable::flex_basis(Dimension d) {
        apply_dim(
            _yoga_node,
            d,
            YGNodeStyleSetFlexBasis,
            YGNodeStyleSetFlexBasisPercent,
            YGNodeStyleSetFlexBasisAuto
        );
        mark_dirty();
        request_render();
        return *this;
    }
    Renderable& Renderable::flex_direction(FlexDirection d) {
        YGNodeStyleSetFlexDirection(_yoga_node, to_yg(d));
        mark_dirty();
        request_render();
        return *this;
    }
    Renderable& Renderable::flex_wrap(FlexWrap w) {
        YGNodeStyleSetFlexWrap(_yoga_node, to_yg(w));
        mark_dirty();
        request_render();
        return *this;
    }
    Renderable& Renderable::align_items(Align a) {
        YGNodeStyleSetAlignItems(_yoga_node, to_yg(a));
        mark_dirty();
        request_render();
        return *this;
    }
    Renderable& Renderable::align_content(Align a) {
        YGNodeStyleSetAlignContent(_yoga_node, to_yg(a));
        mark_dirty();
        request_render();
        return *this;
    }
    Renderable& Renderable::align_self(Align a) {
        YGNodeStyleSetAlignSelf(_yoga_node, to_yg(a));
        mark_dirty();
        request_render();
        return *this;
    }
    Renderable& Renderable::justify_content(Justify j) {
        YGNodeStyleSetJustifyContent(_yoga_node, to_yg(j));
        mark_dirty();
        request_render();
        return *this;
    }
    Renderable& Renderable::margin(Edges e) {
        apply_edges_margin(_yoga_node, e);
        mark_dirty();
        request_render();
        return *this;
    }
    Renderable& Renderable::padding(Edges e) {
        apply_edges_padding(_yoga_node, e);
        mark_dirty();
        request_render();
        return *this;
    }
    Renderable& Renderable::position_type(PositionType t) {
        YGNodeStyleSetPositionType(_yoga_node, to_yg(t));
        mark_dirty();
        request_render();
        return *this;
    }
    Renderable& Renderable::pos_top(Dimension d) {
        apply_edge_dim(
            _yoga_node,
            YGEdgeTop,
            d,
            YGNodeStyleSetPosition,
            YGNodeStyleSetPositionPercent,
            YGNodeStyleSetPositionAuto
        );
        mark_dirty();
        request_render();
        return *this;
    }
    Renderable& Renderable::pos_left(Dimension d) {
        apply_edge_dim(
            _yoga_node,
            YGEdgeLeft,
            d,
            YGNodeStyleSetPosition,
            YGNodeStyleSetPositionPercent,
            YGNodeStyleSetPositionAuto
        );
        mark_dirty();
        request_render();
        return *this;
    }
    Renderable& Renderable::pos_right(Dimension d) {
        apply_edge_dim(
            _yoga_node,
            YGEdgeRight,
            d,
            YGNodeStyleSetPosition,
            YGNodeStyleSetPositionPercent,
            YGNodeStyleSetPositionAuto
        );
        mark_dirty();
        request_render();
        return *this;
    }
    Renderable& Renderable::pos_bottom(Dimension d) {
        apply_edge_dim(
            _yoga_node,
            YGEdgeBottom,
            d,
            YGNodeStyleSetPosition,
            YGNodeStyleSetPositionPercent,
            YGNodeStyleSetPositionAuto
        );
        mark_dirty();
        request_render();
        return *this;
    }
    Renderable& Renderable::overflow(Overflow o) {
        YGNodeStyleSetOverflow(_yoga_node, to_yg(o));
        mark_dirty();
        request_render();
        return *this;
    }
    Renderable& Renderable::display(Display d) {
        YGNodeStyleSetDisplay(_yoga_node, to_yg(d));
        mark_dirty();
        request_render();
        return *this;
    }
    Renderable& Renderable::gap(Dimension d) {
        apply_gap_dim(_yoga_node, YGGutterAll, d);
        mark_dirty();
        request_render();
        return *this;
    }
    Renderable& Renderable::row_gap(Dimension d) {
        apply_gap_dim(_yoga_node, YGGutterRow, d);
        mark_dirty();
        request_render();
        return *this;
    }
    Renderable& Renderable::column_gap(Dimension d) {
        apply_gap_dim(_yoga_node, YGGutterColumn, d);
        mark_dirty();
        request_render();
        return *this;
    }

    Renderable& Renderable::z_index(int z) {
        _opts.z_index = z;
        if(_renderable_parent) {
            _renderable_parent->_z_sort_dirty = true;
        }
        request_render();
        return *this;
    }

    Renderable& Renderable::opacity(float o) {
        _opts.opacity = o;
        request_render();
        return *this;
    }

    Renderable& Renderable::visible(bool v) {
        if(_visible == v)
            return *this;
        _visible = v;
        _opts.visible = v;
        YGNodeStyleSetDisplay(_yoga_node, v ? YGDisplayFlex : YGDisplayNone);
        if(!v && _focused)
            blur();
        mark_dirty();
        request_render();
        return *this;
    }

    Renderable& Renderable::id(std::string id) {
        _id = std::move(id);
        _opts.id = _id;
        return *this;
    }

    Renderable& Renderable::focusable(bool v) {
        _focusable = v;
        _opts.focusable = v;
        return *this;
    }

    Renderable& Renderable::buffered(bool v) {
        _opts.buffered = v;
        if(v && _width > 0 && _height > 0) {
            _frame_buffer.emplace(
                BufferOptions{.width = _width, .height = _height}
            );
        } else if(!v) {
            _frame_buffer.reset();
        }
        return *this;
    }

    void Renderable::focus() {
        if(_destroyed || _focused || !_focusable)
            return;
        _ctx->focus_renderable(this);
        _focused = true;

        _key_conn_id = _ctx->on_key([this](KeyEvent& key) -> bool {
            handle_key_press(key);
            if(_opts.on_key_down && !key.propagation_stopped()) {
                _opts.on_key_down(key);
            }
            emit(events::KeyDown{key});
            return !key.propagation_stopped();
        });

        _paste_conn_id = _ctx->on_paste([this](PasteEvent& event) {
            handle_paste(event);
            if(_opts.on_paste)
                _opts.on_paste(event);
        });

        emit(events::Focused{});
        request_render();
    }

    void Renderable::blur() {
        if(!_focused)
            return;
        _focused = false;
        _ctx->cursor(0, 0, false);
        if(_key_conn_id) {
            _ctx->off_key(_key_conn_id);
            _key_conn_id = 0;
        }
        if(_paste_conn_id) {
            _ctx->off_paste(_paste_conn_id);
            _paste_conn_id = 0;
        }
        emit(events::Blurred{});
        request_render();
    }

    Renderable& Renderable::live(bool live) {
        if(live && _live_count == 0) {
            propagate_live_count(1);
            _ctx->request_live();
            _ctx->register_lifecycle(this);
        } else if(!live && _live_count > 0) {
            propagate_live_count(-1);
            _ctx->drop_live();
            _ctx->unregister_lifecycle(this);
        }
        return *this;
    }

    void Renderable::propagate_live_count(int delta) {
        _live_count += delta;
        if(_renderable_parent) {
            _renderable_parent->propagate_live_count(delta);
        }
    }

    int Renderable::add(
        std::shared_ptr<BaseRenderable> child,
        std::optional<int> index
    ) {
        if(!child || child->is_destroyed())
            return -1;

        // If child already has a parent, remove from it first.
        if(child->_parent) {
            child->_parent->remove(child->id());
        }

        auto idx =
            index.value_or(static_cast<int>(_children_layout_order.size()));
        idx =
            std::clamp(idx, 0, static_cast<int>(_children_layout_order.size()));

        _children_layout_order.insert(
            _children_layout_order.begin() + idx,
            child
        );
        if(dynamic_cast<Renderable*>(child.get()))
            _children_z_order.push_back(child.get());
        _z_sort_dirty = true;
        _child_map[child->id()] = child.get();
        child->_parent = this;
        auto* r = dynamic_cast<Renderable*>(child.get());
        if(r) {
            r->_renderable_parent = this;
            if(r->_yoga_node) {
                YGNodeInsertChild(
                    _yoga_node,
                    r->_yoga_node,
                    static_cast<size_t>(idx)
                );
            }
        }

        request_render();
        emit(events::Added{child.get()});

        return idx;
    }

    void Renderable::remove(std::string_view id) {
        auto it = _child_map.find(std::string(id));
        if(it == _child_map.end())
            return;

        BaseRenderable* raw = it->second;
        _child_map.erase(it);

        // Remove from z_order.
        _children_z_order.erase(
            std::remove(
                _children_z_order.begin(),
                _children_z_order.end(),
                raw
            ),
            _children_z_order.end()
        );

        // Remove from layout_order and get shared_ptr.
        std::shared_ptr<BaseRenderable> removed;
        for(auto lit = _children_layout_order.begin();
            lit != _children_layout_order.end();
            ++lit) {
            if(lit->get() == raw) {
                removed = std::move(*lit);
                _children_layout_order.erase(lit);
                break;
            }
        }

        // Yoga removal.
        auto* r = dynamic_cast<Renderable*>(raw);
        if(r && r->_yoga_node) {
            YGNodeRemoveChild(_yoga_node, r->_yoga_node);
            r->_renderable_parent = nullptr;
        }

        raw->_parent = nullptr;
        request_render();
        emit(events::Removed{std::string(id)});
    }

    void Renderable::insert_before(
        std::shared_ptr<BaseRenderable> child,
        BaseRenderable* anchor
    ) {
        if(!anchor) {
            add(std::move(child));
            return;
        }

        int idx = 0;
        for(auto& c : _children_layout_order) {
            if(c.get() == anchor)
                break;
            idx++;
        }
        add(std::move(child), idx);
    }

    std::vector<BaseRenderable*> Renderable::children() const {
        std::vector<BaseRenderable*> result;
        result.reserve(_children_layout_order.size());
        for(const auto& c : _children_layout_order) {
            result.push_back(c.get());
        }
        return result;
    }

    int Renderable::child_count() const noexcept {
        return static_cast<int>(_children_layout_order.size());
    }

    BaseRenderable* Renderable::find(std::string_view id) const {
        auto it = _child_map.find(std::string(id));
        if(it != _child_map.end())
            return it->second;
        // Recursive search.
        for(const auto& c : _children_layout_order) {
            if(auto* found = c->find(id))
                return found;
        }
        return nullptr;
    }

    void Renderable::request_render() {
        if(_ctx)
            _ctx->request_render();
    }

    void Renderable::on_lifecycle_pass(double /*delta_time*/) {
    }

    void Renderable::update_from_layout() {
        if(!_yoga_node || !_visible)
            return;

        int new_x = static_cast<int>(YGNodeLayoutGetLeft(_yoga_node));
        int new_y = static_cast<int>(YGNodeLayoutGetTop(_yoga_node));
        int new_w = static_cast<int>(YGNodeLayoutGetWidth(_yoga_node));
        int new_h = static_cast<int>(YGNodeLayoutGetHeight(_yoga_node));

        bool size_changed = (new_w != _width || new_h != _height);
        _x = new_x;
        _y = new_y;
        _width = new_w;
        _height = new_h;

        if(size_changed) {
            on_resize(new_w, new_h);
            if(_opts.on_size_change)
                _opts.on_size_change();
            emit(events::Resized{new_w, new_h});

            if(_opts.buffered) {
                _frame_buffer.emplace(
                    BufferOptions{.width = new_w, .height = new_h}
                );
            }
        }

        // Recurse into children.
        for(auto& child : _children_layout_order) {
            if(auto* r = dynamic_cast<Renderable*>(child.get()))
                r->update_from_layout();
        }

        mark_clean();
    }

    void Renderable::render(OptimizedBuffer& buf, double delta_time) {
        if(!_visible)
            return;

        // render_before callback.
        if(_opts.render_before)
            _opts.render_before(buf, delta_time);

        // Choose target buffer.
        OptimizedBuffer& target =
            (_opts.buffered && _frame_buffer) ? *_frame_buffer : buf;

        bool pushed_scissor = false;
        bool pushed_opacity = false;

        if(_opts.overflow == Overflow::Hidden ||
           _opts.overflow == Overflow::Scroll) {
            int bx =
                static_cast<int>(YGNodeLayoutGetBorder(_yoga_node, YGEdgeLeft));
            int by =
                static_cast<int>(YGNodeLayoutGetBorder(_yoga_node, YGEdgeTop));
            int br = static_cast<int>(
                YGNodeLayoutGetBorder(_yoga_node, YGEdgeRight)
            );
            int bb = static_cast<int>(
                YGNodeLayoutGetBorder(_yoga_node, YGEdgeBottom)
            );
            target.push_scissor(
                ClipRect{
                    screen_x() + bx,
                    screen_y() + by,
                    _width - bx - br,
                    _height - by - bb
                }
            );
            pushed_scissor = true;
        }

        if(_opts.opacity < 1.0f) {
            target.push_opacity(_opts.opacity);
            pushed_opacity = true;
        }

        // Subclass draw.
        draw(target, delta_time);

        // Register in hit grid before children so that reverse dispatch
        // gives priority to child renderables (children registered later
        // come first in reverse iteration).
        if(_ctx) {
            _ctx->add_to_hit_grid(
                screen_x(),
                screen_y(),
                _width,
                _height,
                _num
            );
        }

        // Z-sorted children.
        ensure_z_sorted();
        for(auto* child : _children_z_order) {
            static_cast<Renderable*>(child)->render(target, delta_time);
        }

        if(pushed_opacity)
            target.pop_opacity();
        if(pushed_scissor)
            target.pop_scissor();

        // Blit frame buffer to parent buffer.
        if(_opts.buffered && _frame_buffer) {
            buf.blit(*_frame_buffer, screen_x(), screen_y());
        }

        // render_after callback.
        if(_opts.render_after)
            _opts.render_after(buf, delta_time);
    }

    void Renderable::process_mouse_event(MouseEvent& event) {
        switch(event.type) {
        case MouseEventType::Down:
            if(_focusable)
                focus();
            if(_opts.on_mouse_down)
                _opts.on_mouse_down(event);
            emit(events::MouseDown{event});
            break;
        case MouseEventType::Up:
            if(_opts.on_mouse_up)
                _opts.on_mouse_up(event);
            emit(events::MouseUp{event});
            break;
        case MouseEventType::Move:
            if(_opts.on_mouse_move)
                _opts.on_mouse_move(event);
            emit(events::MouseMove{event});
            break;
        case MouseEventType::Drag:
            if(_opts.on_mouse_drag)
                _opts.on_mouse_drag(event);
            emit(events::MouseDrag{event});
            break;
        case MouseEventType::Scroll:
            if(_opts.on_mouse_scroll)
                _opts.on_mouse_scroll(event);
            emit(events::MouseScroll{event});
            break;
        }
    }

    void Renderable::mark_dirty() {
        _dirty = true;
    }
    void Renderable::mark_clean() {
        _dirty = false;
    }

    void Renderable::ensure_z_sorted() {
        if(!_z_sort_dirty)
            return;
        _children_z_order.erase(
            std::remove_if(
                _children_z_order.begin(),
                _children_z_order.end(),
                [](BaseRenderable* c) {
                    return dynamic_cast<Renderable*>(c) == nullptr;
                }
            ),
            _children_z_order.end()
        );
        std::stable_sort(
            _children_z_order.begin(),
            _children_z_order.end(),
            [](const BaseRenderable* a, const BaseRenderable* b) {
                return static_cast<const Renderable*>(a)->opts().z_index <
                       static_cast<const Renderable*>(b)->opts().z_index;
            }
        );
        _z_sort_dirty = false;
    }

    void Renderable::destroy() {
        if(_focused)
            blur();
        registry().erase(_num);
        BaseRenderable::destroy();
    }

    void Renderable::destroy_recursive() {
        for(auto& child : _children_layout_order) {
            child->destroy_recursive();
        }
        destroy();
    }

} // namespace stain
