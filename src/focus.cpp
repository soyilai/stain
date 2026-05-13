#include "stain/focus.hpp"
#include "stain/renderer.hpp"

namespace stain {

    void FocusCycler::add(Renderable* r) {
        _focusable.push_back(r);
    }

    bool FocusCycler::handle_key(KeyEvent& key) {
        if(key.name == "tab") {
            cycle(key.shift ? -1 : 1);
            key.stop_propagation();
            return true;
        }
        return false;
    }

    void FocusCycler::focus_first() {
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
        renderer.on_key([this](KeyEvent& key) -> bool {
            return handle_key(key);
        });
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
