#include "stain/select.hpp"
#include <algorithm>

namespace stain {

    const std::vector<KeyBinding> default_select_bindings = {
        {"up", "move-up"},
        {"down", "move-down"},
        {"k", "move-up"},
        {"j", "move-down"},
        {"return", "select"},
        {"left", "move-left"},
        {"right", "move-right"},
        {"h", "move-left"},
        {"l", "move-right"},
    };

    // Select
    Select::Select(RenderContext* ctx, SelectOptions opts)
        : Renderable(ctx, static_cast<RenderableOptions&>(opts))
        , _select_opts(std::move(opts))
        , _selected_index(_select_opts.selected_index) {
        _focusable = true;
        auto merged =
            merge_bindings(default_select_bindings, _select_opts.key_bindings);
        _binding_map = build_binding_map(merged, _select_opts.key_aliases);
    }

    int Select::selected_index() const noexcept {
        return _selected_index;
    }

    const SelectOption* Select::selected_option() const noexcept {
        if(_selected_index >= 0 &&
           _selected_index < static_cast<int>(_select_opts.options.size()))
            return &_select_opts
                        .options[static_cast<std::size_t>(_selected_index)];
        return nullptr;
    }

    void Select::move_up(int steps) {
        if(_select_opts.options.empty())
            return;
        _selected_index = std::max(0, _selected_index - steps);
        if(_selected_index < _scroll_offset)
            _scroll_offset = _selected_index;
        const auto& opt =
            _select_opts.options[static_cast<std::size_t>(_selected_index)];
        if(_select_opts.on_selection_changed)
            _select_opts.on_selection_changed(_selected_index, opt);
        emit(events::SelectionChanged{_selected_index, opt.label, opt.value});
        request_render();
    }

    void Select::move_down(int steps) {
        if(_select_opts.options.empty())
            return;
        _selected_index = std::min(
            static_cast<int>(_select_opts.options.size()) - 1,
            _selected_index + steps
        );
        int h = layout_h();
        if(_selected_index >= _scroll_offset + h)
            _scroll_offset = _selected_index - h + 1;
        const auto& opt =
            _select_opts.options[static_cast<std::size_t>(_selected_index)];
        if(_select_opts.on_selection_changed)
            _select_opts.on_selection_changed(_selected_index, opt);
        emit(events::SelectionChanged{_selected_index, opt.label, opt.value});
        request_render();
    }

    void Select::select_current() {
        if(auto* opt = selected_option()) {
            if(_select_opts.on_item_selected)
                _select_opts.on_item_selected(_selected_index, *opt);
            emit(events::ItemSelected{_selected_index, opt->label, opt->value});
        }
    }

    Select& Select::options(std::vector<SelectOption> opts) {
        _select_opts.options = std::move(opts);
        _selected_index = std::clamp(
            _selected_index,
            0,
            std::max(0, static_cast<int>(_select_opts.options.size()) - 1)
        );
        _scroll_offset = std::min(
            _scroll_offset,
            std::max(0, static_cast<int>(_select_opts.options.size()) - 1)
        );
        request_render();
        return *this;
    }

    Select& Select::selected_index(int idx) {
        _selected_index = idx;
        request_render();
        return *this;
    }
    Select& Select::style(ItemStyle s) {
        _select_opts.style = s;
        request_render();
        return *this;
    }
    Select& Select::selected_style(ItemStyle s) {
        _select_opts.selected_style = s;
        request_render();
        return *this;
    }
    Select& Select::on_select(std::function<void(int, const SelectOption&)> f) {
        _select_opts.on_selection_changed = std::move(f);
        return *this;
    }
    Select&
    Select::on_activate(std::function<void(int, const SelectOption&)> f) {
        _select_opts.on_item_selected = std::move(f);
        return *this;
    }

    void Select::draw(OptimizedBuffer& buf, double /*delta*/) {
        int sx = screen_x(), sy = screen_y(), w = layout_w(), h = layout_h();
        if(w <= 0 || h <= 0)
            return;
        if(!_select_opts.style.bg.is_transparent())
            buf.fill_rect(sx, sy, w, h, _select_opts.style.bg);
        int visible = std::min(
            static_cast<int>(_select_opts.options.size()) - _scroll_offset,
            h
        );
        for(int i = 0; i < visible; i++) {
            int idx = _scroll_offset + i;
            bool selected = (idx == _selected_index);
            RGBA fg = selected ? _select_opts.selected_style.text
                               : _select_opts.style.text;
            RGBA bg = selected ? _select_opts.selected_style.bg
                               : _select_opts.style.bg;
            if(selected)
                buf.fill_rect(sx, sy + i, w, 1, bg);
            std::string label =
                (selected ? " > " : "   ") +
                _select_opts.options[static_cast<std::size_t>(idx)].label;
            buf.draw_text(sx, sy + i, label, fg, bg);
        }
    }

    void Select::on_resize(int /*w*/, int /*h*/) {
        _scroll_offset = std::max(
            0,
            std::min(
                _scroll_offset,
                std::max(
                    0,
                    static_cast<int>(_select_opts.options.size()) - layout_h()
                )
            )
        );
    }

    bool Select::handle_key_press(KeyEvent& key) {
        auto combo = key_binding_key(key);
        auto it = _binding_map.find(combo);
        if(it != _binding_map.end()) {
            if(it->second == "move-up")
                move_up();
            else if(it->second == "move-down")
                move_down();
            else if(it->second == "select")
                select_current();
            return true;
        }
        return false;
    }

    void Select::process_mouse_event(MouseEvent& event) {
        if(event.type == MouseEventType::Down) {
            if(_focusable)
                focus();
            int sy = screen_y(), h = layout_h();
            int row = event.y - sy;
            if(row >= 0 && row < h) {
                int idx = _scroll_offset + row;
                if(idx >= 0 &&
                   idx < static_cast<int>(_select_opts.options.size())) {
                    if(idx != _selected_index) {
                        _selected_index = idx;
                        if(_select_opts.on_selection_changed)
                            _select_opts.on_selection_changed(
                                idx,
                                _select_opts
                                    .options[static_cast<std::size_t>(idx)]
                            );
                    }
                    if(_select_opts.on_item_selected)
                        _select_opts.on_item_selected(
                            idx,
                            _select_opts.options[static_cast<std::size_t>(idx)]
                        );
                    request_render();
                    event.stop_propagation();
                    return;
                }
            }
        } else if(event.type == MouseEventType::Scroll) {
            if(event.scroll_delta > 0)
                move_down(event.scroll_delta);
            else
                move_up(-event.scroll_delta);
            event.stop_propagation();
            return;
        }
        Renderable::process_mouse_event(event);
    }

    // TabSelect
    TabSelect::TabSelect(RenderContext* ctx, TabSelectOptions opts)
        : Renderable(ctx, static_cast<RenderableOptions&>(opts))
        , _tab_opts(std::move(opts))
        , _selected_index(_tab_opts.selected_index) {
        _focusable = true;
        auto merged =
            merge_bindings(default_select_bindings, _tab_opts.key_bindings);
        _binding_map = build_binding_map(merged, _tab_opts.key_aliases);
    }

    int TabSelect::selected_index() const noexcept {
        return _selected_index;
    }

    void TabSelect::move_left() {
        if(_tab_opts.options.empty())
            return;
        _selected_index = std::max(0, _selected_index - 1);
        const auto& opt =
            _tab_opts.options[static_cast<std::size_t>(_selected_index)];
        if(_tab_opts.on_selection_changed)
            _tab_opts.on_selection_changed(_selected_index, opt);
        emit(events::SelectionChanged{_selected_index, opt.label, opt.value});
        request_render();
    }

    void TabSelect::move_right() {
        if(_tab_opts.options.empty())
            return;
        _selected_index = std::min(
            static_cast<int>(_tab_opts.options.size()) - 1,
            _selected_index + 1
        );
        const auto& opt =
            _tab_opts.options[static_cast<std::size_t>(_selected_index)];
        if(_tab_opts.on_selection_changed)
            _tab_opts.on_selection_changed(_selected_index, opt);
        emit(events::SelectionChanged{_selected_index, opt.label, opt.value});
        request_render();
    }

    void TabSelect::select_current() {
        if(_selected_index >= 0 &&
           _selected_index < static_cast<int>(_tab_opts.options.size())) {
            const auto& opt =
                _tab_opts.options[static_cast<std::size_t>(_selected_index)];
            if(_tab_opts.on_item_selected)
                _tab_opts.on_item_selected(_selected_index, opt);
            emit(events::ItemSelected{_selected_index, opt.label, opt.value});
        }
    }

    TabSelect& TabSelect::options(std::vector<TabOption> opts) {
        _tab_opts.options = std::move(opts);
        _selected_index = std::clamp(
            _selected_index,
            0,
            std::max(0, static_cast<int>(_tab_opts.options.size()) - 1)
        );
        request_render();
        return *this;
    }
    TabSelect& TabSelect::selected_index(int idx) {
        _selected_index = idx;
        request_render();
        return *this;
    }
    TabSelect& TabSelect::style(ItemStyle s) {
        _tab_opts.style = s;
        request_render();
        return *this;
    }
    TabSelect& TabSelect::selected_style(ItemStyle s) {
        _tab_opts.selected_style = s;
        request_render();
        return *this;
    }
    TabSelect&
    TabSelect::on_select(std::function<void(int, const TabOption&)> f) {
        _tab_opts.on_selection_changed = std::move(f);
        return *this;
    }
    TabSelect&
    TabSelect::on_activate(std::function<void(int, const TabOption&)> f) {
        _tab_opts.on_item_selected = std::move(f);
        return *this;
    }

    void TabSelect::draw(OptimizedBuffer& buf, double /*delta*/) {
        int sx = screen_x(), sy = screen_y(), w = layout_w();
        if(w <= 0)
            return;
        int cx = sx;
        for(int i = 0; i < static_cast<int>(_tab_opts.options.size()); i++) {
            bool selected = (i == _selected_index);
            RGBA fg =
                selected ? _tab_opts.selected_style.text : _tab_opts.style.text;
            RGBA bg =
                selected ? _tab_opts.selected_style.bg : RGBA::transparent();
            std::string label =
                " " + _tab_opts.options[static_cast<std::size_t>(i)].label +
                " ";
            int tw = _tab_opts.options[static_cast<std::size_t>(i)].tab_width;
            if(tw <= 0)
                tw = static_cast<int>(label.size());
            if(selected)
                buf.fill_rect(cx, sy, tw, 1, bg);
            buf.draw_text(cx, sy, label, fg, bg);
            cx += tw;
        }
    }

    bool TabSelect::handle_key_press(KeyEvent& key) {
        auto combo = key_binding_key(key);
        auto it = _binding_map.find(combo);
        if(it != _binding_map.end()) {
            if(it->second == "move-left")
                move_left();
            else if(it->second == "move-right")
                move_right();
            else if(it->second == "select")
                select_current();
            return true;
        }
        return false;
    }

    void TabSelect::process_mouse_event(MouseEvent& event) {
        if(event.type == MouseEventType::Down) {
            if(_focusable)
                focus();
            int sx = screen_x(), sy = screen_y();
            int cx = sx;
            for(int i = 0; i < static_cast<int>(_tab_opts.options.size());
                i++) {
                int tw =
                    _tab_opts.options[static_cast<std::size_t>(i)].tab_width;
                if(tw <= 0) {
                    std::string label =
                        " " +
                        _tab_opts.options[static_cast<std::size_t>(i)].label +
                        " ";
                    tw = static_cast<int>(label.size());
                }
                if(event.y == sy && event.x >= cx && event.x < cx + tw) {
                    if(i != _selected_index) {
                        _selected_index = i;
                        if(_tab_opts.on_selection_changed)
                            _tab_opts.on_selection_changed(
                                i,
                                _tab_opts.options[static_cast<std::size_t>(i)]
                            );
                    }
                    if(_tab_opts.on_item_selected)
                        _tab_opts.on_item_selected(
                            _selected_index,
                            _tab_opts.options
                                [static_cast<std::size_t>(_selected_index)]
                        );
                    request_render();
                    event.stop_propagation();
                    return;
                }
                cx += tw;
            }
        }
        Renderable::process_mouse_event(event);
    }

} // namespace stain
