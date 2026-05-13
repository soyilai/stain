#pragma once

#include "stain/renderable.hpp"

#include <functional>

namespace stain {

    using FrameCallback = std::function<void(OptimizedBuffer&)>;

    // A Renderable that wraps an OptimizedBuffer and blits it to the screen.
    // Use when you need direct, imperative cell control without composing
    // sub-renderables.
    //
    // Usage:
    //   auto fb = framebuffer(*renderer);
    //   fb->live(true);
    //   fb->on_frame([](OptimizedBuffer& buf) {
    //       buf.clear();
    //       buf.draw_text(0, 0, "hello", RGBA::white());
    //   });
    class Framebuffer : public Renderable {
        public:
        explicit Framebuffer(RenderContext* ctx, RenderableOptions opts = {});

        // Access the internal buffer directly.
        [[nodiscard]] OptimizedBuffer& buffer() {
            return _buffer;
        }

        // Register a per-frame callback that mutates the internal buffer.
        // Called at the start of draw(), before blit to screen.
        void on_frame(FrameCallback cb) {
            _frame_cb = std::move(cb);
        }

        void set_cell(int x, int y, Cell cell);
        void clear(RGBA bg = RGBA::transparent());
        int draw_text(
            int x,
            int y,
            std::string_view text,
            RGBA fg,
            RGBA bg = RGBA::transparent(),
            Attr attr = Attr::None
        );
        void fill_rect(int x, int y, int w, int h, RGBA bg);
        void draw_box(
            int x,
            int y,
            int w,
            int h,
            BorderStyle style,
            RGBA border_color,
            RGBA bg = RGBA::transparent(),
            BorderSides sides = BorderSides::all()
        );

        protected:
        void draw(OptimizedBuffer& buf, double delta) override;
        void on_resize(int w, int h) override;

        private:
        OptimizedBuffer _buffer;
        FrameCallback _frame_cb;
    };

    // Factory function for a Framebuffer.
    inline std::shared_ptr<Framebuffer> framebuffer(RenderContext& ctx) {
        return std::make_shared<Framebuffer>(&ctx);
    }

} // namespace stain
