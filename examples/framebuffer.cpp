#include <stain/box.hpp>
#include <stain/framebuffer.hpp>
#include <stain/renderer.hpp>
#include <stain/text.hpp>

#include <cmath>
#include <memory>
#include <string>

using namespace stain;

static constexpr float PI = 3.14159265f;

int main() {
    auto renderer = Renderer::create(
        RendererOptions{}
            .target_fps(60)
            .background_color(RGBA::black())
            .use_mouse(true)
            .use_kitty_keyboard(true)
    );
    auto& root = renderer->root();

    auto outer = box(*renderer);
    outer->flex_direction(FlexDirection::Column);
    outer->flex_grow(1.f);
    outer->padding(Edges::all(px(0)));
    outer->style(
        {.background_color = RGBA{0.02f, 0.02f, 0.06f, 1.0f},
         .border_style = BorderStyle::Rounded,
         .border_color = RGBA::from_hex("#444466")}
    );
    outer->title(" framebuffer demo ");

    auto fb = framebuffer(*renderer);
    fb->live(true);
    fb->flex_grow(1.f);

    int frame = 0;
    fb->on_frame([&frame](OptimizedBuffer& buf) {
        int w = buf.width();
        int h = buf.height();
        if(w <= 0 || h <= 0)
            return;

        // 1. Clear
        buf.clear(RGBA{0.02f, 0.02f, 0.08f, 1.0f});

        // 2. Per-cell plasma
        for(int y = 0; y < h; y++) {
            for(int x = 0; x < w; x++) {
                float fx = float(x) / float(w);
                float fy = float(y) / float(h);
                float t = float(frame) * 0.05f;

                float v =
                    std::sin(fx * 4.0f + t) * std::cos(fy * 3.0f - t * 0.7f) +
                    std::sin((fx + fy) * 5.0f + t * 1.3f) * 0.5f;
                v = v * 0.5f + 0.5f;
                v = std::clamp(v, 0.0f, 1.0f);

                float r = v * 0.2f + std::sin(v * PI * 2.0f + 1.0f) * 0.3f;
                float g = v * 0.6f + std::sin(v * PI * 2.0f + 3.0f) * 0.3f;
                float b = 0.4f + v * 0.6f;

                char32_t ch = U'█';
                if(v < 0.25f)
                    ch = U'░';
                else if(v < 0.5f)
                    ch = U'▒';
                else if(v < 0.8f)
                    ch = U'▓';

                buf.set_cell(
                    x,
                    y,
                    Cell{ch, {r, g, b, 1.0f}, RGBA::transparent()}
                );
            }
        }

        // 3. Box overlay
        int bx = w / 2 - 10;
        int by = h / 2 - 2;
        buf.draw_box(
            bx,
            by,
            20,
            4,
            BorderStyle::Rounded,
            RGBA::from_hex("#FFCC00"),
            RGBA{0.0f, 0.0f, 0.0f, 0.6f}
        );
        buf.draw_text(
            bx + 2,
            by + 1,
            " Framebuffer Demo ",
            RGBA::from_hex("#FFCC00"),
            RGBA::transparent(),
            Attr::Bold
        );
        std::string info = " frame: " + std::to_string(frame) + "  " +
                           std::to_string(w) + "x" + std::to_string(h);
        buf.draw_text(bx + 2, by + 2, info, RGBA::from_hex("#AAAAAA"));

        // 4. Corner decorations
        buf.set_cell(0, 0, Cell{U'╔', RGBA::from_hex("#FF6644")});
        buf.set_cell(w - 1, 0, Cell{U'╗', RGBA::from_hex("#FF6644")});
        buf.set_cell(0, h - 1, Cell{U'╚', RGBA::from_hex("#FF6644")});
        buf.set_cell(w - 1, h - 1, Cell{U'╝', RGBA::from_hex("#FF6644")});
        for(int x = 1; x < w - 1; x++) {
            float bright =
                std::sin(float(x) * 0.3f + float(frame) * 0.1f) * 0.5f + 0.5f;
            auto c = RGBA{bright, bright * 0.7f, 0.2f, 1.0f};
            buf.set_cell(x, 0, Cell{U'═', c});
            buf.set_cell(x, h - 1, Cell{U'═', c});
        }

        frame++;
    });

    outer->add(fb);

    auto status = text(*renderer);
    status->height(px(1));
    status->content(
        TextBuilder{}.fg(RGBA::from_hex("#666666"), " q / Ctrl+C: quit ")
    );
    outer->add(status);

    root.add(outer);

    renderer->on_key([&renderer](KeyEvent& key) -> bool {
        if(key.name == "q" || (key.ctrl && key.name == "c")) {
            renderer->destroy();
            return true;
        }
        return false;
    });

    renderer->run();
    return 0;
}
