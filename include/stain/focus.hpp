#pragma once

#include "stain/event.hpp"
#include "stain/renderable.hpp"

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
        // Register a focusable renderable.
        void add(Renderable* r);

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

        private:
        void cycle(int dir);
        std::vector<Renderable*> _focusable;
    };

} // namespace stain
