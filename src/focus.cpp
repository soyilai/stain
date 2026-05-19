#include "stain/focus.hpp"
#include "stain/renderer.hpp"

namespace stain {

    FocusCycler::~FocusCycler() {
        disable_auto_cycle();
    }

    void FocusCycler::add(Renderable* r) {
        _focusable.push_back(r);
    }

    void FocusCycler::remove(Renderable* r) {
        auto it = std::find(_focusable.begin(), _focusable.end(), r);
        if(it != _focusable.end())
            _focusable.erase(it);
    }

    void FocusCycler::remove_dangling() {
        _focusable.erase(
            std::remove_if(
                _focusable.begin(),
                _focusable.end(),
                [](Renderable* r) {
                    return !r || r->is_destroyed();
                }
            ),
            _focusable.end()
        );
    }

    bool FocusCycler::handle_key(KeyEvent& key) {
        if(key.name == "tab") {
            remove_dangling();
            cycle(key.shift ? -1 : 1);
            key.stop_propagation();
            return true;
        }
        return false;
    }

    void FocusCycler::focus_first() {
        remove_dangling();
        if(!_focusable.empty())
            _focusable[0]->focus();
    }

    void FocusCycler::next() {
        cycle(1);
    }

    void FocusCycler::prev() {
        cycle(-1);
    }

    void FocusCycler::enable_auto_cycle(Renderer& renderer) {
        disable_auto_cycle();
        _renderer = &renderer;
        _key_conn_id = renderer.on_key([this](KeyEvent& key) -> bool {
            return handle_key(key);
        });
    }

    void FocusCycler::disable_auto_cycle() {
        if(_renderer && _key_conn_id) {
            _renderer->off_key(_key_conn_id);
        }
        _key_conn_id = 0;
        _renderer = nullptr;
    }

    void FocusCycler::cycle(int dir) {
        if(_focusable.empty())
            return;
        int idx = -1;
        for(int i = 0; i < static_cast<int>(_focusable.size()); i++) {
            if(_focusable[static_cast<std::size_t>(i)]->focused()) {
                idx = i;
                break;
            }
        }
        int n = static_cast<int>(_focusable.size());
        int next = (idx < 0) ? 0 : (idx + dir + n) % n;
        _focusable[static_cast<std::size_t>(next)]->focus();
    }

} // namespace stain
