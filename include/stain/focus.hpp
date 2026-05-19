#pragma once

#include "stain/event.hpp"
#include "stain/renderable.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace stain {

    class Renderer;

    // Manages Tab/Shift+Tab cycling between focusable renderables.
    // Usage:
    //   FocusCycler cycler;
    //   cycler.add(box1.get());
    //   cycler.add(input1.get());
    //   cycler.enable_auto_cycle(renderer);
    class FocusCycler {
        public:
        FocusCycler() = default;
        ~FocusCycler();

        FocusCycler(const FocusCycler&) = delete;
        FocusCycler& operator=(const FocusCycler&) = delete;
        FocusCycler(FocusCycler&&) = delete;
        FocusCycler& operator=(FocusCycler&&) = delete;

        // Register a focusable renderable.
        void add(Renderable* r);

        // Remove a renderable from tracking.
        void remove(Renderable* r);

        // Handle Tab (forward) and Shift+Tab (backward) keys.
        // Returns true if the key was handled.
        bool handle_key(KeyEvent& key);

        // Focus the first registered renderable.
        void focus_first();

        // Move focus to the next/previous renderable.
        void next();
        void prev();

        // Register as a global key handler on the renderer.
        void enable_auto_cycle(Renderer& renderer);

        // Disconnect the key handler from the renderer.
        void disable_auto_cycle();

        private:
        void cycle(int dir);
        void remove_dangling();
        std::vector<Renderable*> _focusable;
        Renderer* _renderer{nullptr};
        std::size_t _key_conn_id{0};
    };

} // namespace stain
