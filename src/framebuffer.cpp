#include "stain/framebuffer.hpp"

namespace stain {

    Framebuffer::Framebuffer(RenderContext* ctx, RenderableOptions opts)
        : Renderable(ctx, std::move(opts))
        , _buffer(BufferOptions{0, 0}) {
    }

    void Framebuffer::set_cell(int x, int y, Cell cell) {
        _buffer.set_cell(x, y, cell);
    }

    void Framebuffer::clear(RGBA bg) {
        _buffer.clear(bg);
    }

    int Framebuffer::draw_text(
        int x,
        int y,
        std::string_view text,
        RGBA fg,
        RGBA bg,
        Attr attr
    ) {
        return _buffer.draw_text(x, y, text, fg, bg, attr);
    }

    void Framebuffer::fill_rect(int x, int y, int w, int h, RGBA bg) {
        _buffer.fill_rect(x, y, w, h, bg);
    }

    void Framebuffer::draw_box(
        int x,
        int y,
        int w,
        int h,
        BorderStyle style,
        RGBA border_color,
        RGBA bg,
        BorderSides sides
    ) {
        _buffer.draw_box(x, y, w, h, style, border_color, bg, sides);
    }

    void Framebuffer::draw(OptimizedBuffer& buf, double /*delta*/) {
        if(_buffer.width() <= 0 || _buffer.height() <= 0)
            return;
        if(_frame_cb)
            _frame_cb(_buffer);
        buf.blit(_buffer, screen_x(), screen_y());
    }

    void Framebuffer::on_resize(int w, int h) {
        if(w > 0 && h > 0 && (w != _buffer.width() || h != _buffer.height())) {
            _buffer.resize(w, h);
            _buffer.clear();
        }
    }

} // namespace stain
