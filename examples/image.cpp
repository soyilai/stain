#include <stain/box.hpp>
#include <stain/image.hpp>
#include <stain/renderer.hpp>
#include <stain/terminal.hpp>
#include <stain/text.hpp>

#include <cmath>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

using namespace stain;

// Generate a colourful test pattern when no file is given.
static std::vector<RGBA> make_checkerboard(int w, int h) {
    std::vector<RGBA> pixels(static_cast<std::size_t>(w) * h);
    for(int y = 0; y < h; y++) {
        for(int x = 0; x < w; x++) {
            float fx = float(x) / float(w);
            float fy = float(y) / float(h);
            float r = std::sin(fx * 6.28f) * 0.5f + 0.5f;
            float g = std::sin(fy * 6.28f + 2.09f) * 0.5f + 0.5f;
            float b = std::sin((fx + fy) * 6.28f + 4.18f) * 0.5f + 0.5f;

            // checkerboard overlay
            int cx = x / 16, cy = y / 16;
            float edge = ((cx + cy) & 1) ? 0.85f : 1.0f;
            pixels[static_cast<std::size_t>(y) * w + x] = {
                r * edge, g * edge, b * edge, 1.0f};
        }
    }
    return pixels;
}

int main(int argc, char** argv) {
    auto renderer = Renderer::create(
        RendererOptions{}
            .target_fps(30)
            .background_color(RGBA{0.02f, 0.02f, 0.06f, 1.0f})
            .use_mouse(true)
            .use_kitty_keyboard(true)
            .exit_on_ctrl_c(false)
    );
    auto& root = renderer->root();

    auto& info = renderer->terminal_info();

    auto outer = box(*renderer);
    outer->flex_direction(FlexDirection::Column);
    outer->flex_grow(1.f);
    outer->padding(Edges::all(px(1)));
    outer->style(
        {.background_color = RGBA{0.03f, 0.03f, 0.08f, 1.0f},
         .border_style = BorderStyle::Rounded,
         .border_color = RGBA::from_hex("#444466")}
    );
    outer->title(" image demo ");

    // Status / info bar.
    auto status = text(*renderer);
    status->height(px(1));
    {
        std::string proto;
        if(info.has_kitty_graphics)
            proto = "kitty graphics";
        else if(info.has_iterm2_images)
            proto = "iTerm2 images";
        else
            proto = "half-block fallback";
        status->content(
            TextBuilder{}
                .fg(RGBA::from_hex("#888888"),
                    " terminal: " + std::string(info.has_kitty_graphics ? "Kitty"
                                                    : info.has_iterm2_images
                                                        ? "iTerm2"
                                                        : "other") +
                        "  |  protocol: " + proto +
                        "  |  q/Ctrl+C: quit  |  arrows: switch fit ")
        );
    }
    outer->add(status);

    // The image widget.
    auto img = image(*renderer);
    img->flex_grow(1.f);
    img->margin(Edges::all(px(1)));
    img->live(true);

    // Load from argv[1] or generate a test pattern.
    if(argc > 1) {
        if(!img->load(argv[1])) {
            status->content(
                TextBuilder{}.fg(RGBA::from_hex("#FF4444"),
                                 " failed to load: " + std::string(argv[1]))
            );
            // fall through to test pattern
        } else {
            status->content(
                TextBuilder{}.fg(RGBA::from_hex("#88FF88"),
                                 " loaded: " + std::string(argv[1]) +
                                     "  (" + std::to_string(img->image_width()) +
                                     "x" + std::to_string(img->image_height()) +
                                     ")")
            );
        }
    }

    if(!img->has_image()) {
        auto pixels = make_checkerboard(256, 128);
        img->set_pixels(256, 128, std::move(pixels));
    }

    outer->add(img);

    // Fit mode label.
    auto fit_label = text(*renderer);
    fit_label->height(px(1));
    fit_label->content(
        TextBuilder{}.fg(RGBA::from_hex("#888888"), " fit: Contain ")
    );
    outer->add(fit_label);

    root.add(outer);

    // Key handling: cycle fit modes.
    ImageFit fits[] = {ImageFit::Contain, ImageFit::Fill, ImageFit::Cover};
    const char* fit_names[] = {"Contain", "Fill", "Cover"};
    int fit_idx = 0;

    renderer->on_key([&](KeyEvent& key) -> bool {
        if(key.name == "q" || (key.ctrl && key.name == "c")) {
            renderer->destroy();
            return true; // let signal continue (single handler anyway)
        }
        if(key.name == "left" || key.name == "right") {
            int dir = (key.name == "right") ? 1 : -1;
            fit_idx = (fit_idx + dir + 3) % 3;
            img->fit(fits[fit_idx]);
            fit_label->content(
                TextBuilder{}.fg(
                    RGBA::from_hex("#888888"),
                    std::string(" fit: ") + fit_names[fit_idx] + " ")
            );
            return true; // let signal continue
        }
        return true; // unhandled: let other handlers try
    });

    renderer->run();
    return 0;
}
