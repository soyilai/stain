#include <stain/box.hpp>
#include <stain/focus.hpp>
#include <stain/framebuffer.hpp>
#include <stain/input.hpp>
#include <stain/keybind.hpp>
#include <stain/renderer.hpp>
#include <stain/scroll.hpp>
#include <stain/select.hpp>
#include <stain/text.hpp>

#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace stain;

// Custom animated counter  /  demonstrates on_lifecycle_pass + live
class AnimatedCounter : public Renderable {
    public:
    AnimatedCounter(RenderContext* ctx, RGBA color)
        : Renderable(ctx, RenderableOptions{})
        , _color(color) {
        height(px(1));
        live(true);
        update_text();
    }

    void on_lifecycle_pass(double) override {
        _counter = (_counter + 1) % 1000;
        update_text();
        request_render();
    }

    protected:
    void draw(OptimizedBuffer& buf, double) override {
        buf.draw_text(screen_x(), screen_y(), _text, _color);
    }

    private:
    void update_text() {
        _text = " lifecycle: " + std::to_string(_counter);
    }
    int _counter{0};
    RGBA _color;
    std::string _text;
};

// Demonstrates render_before / render_after callbacks
class CallbackDemo : public Renderable {
    public:
    CallbackDemo(RenderContext* ctx)
        : Renderable(ctx, RenderableOptions{}) {
        height(px(1));
        _opts.render_before = [this](OptimizedBuffer& buf, double) {
            buf.draw_text(
                screen_x(),
                screen_y(),
                " render_before fires first  ",
                RGBA::from_hex("#888888")
            );
        };
        _opts.render_after = [this](OptimizedBuffer& buf, double) {
            buf.draw_text(
                screen_x() + 28,
                screen_y(),
                "  render_after fires last",
                RGBA::from_hex("#888888")
            );
        };
    }

    protected:
    void draw(OptimizedBuffer& buf, double) override {
        buf.draw_text(
            screen_x() + 14,
            screen_y(),
            " <-- draw() in the middle --> ",
            RGBA::from_hex("#FFFFFF"),
            RGBA::transparent(),
            Attr::Bold
        );
    }
};

int main() {
    auto renderer = Renderer::create(
        RendererOptions{}
            .target_fps(30)
            .background_color(RGBA::black())
            .use_mouse(true)
            .use_kitty_keyboard(true)
    );
    auto& root = renderer->root();

    //  0. OUTER SHELL
    auto outer = box(*renderer);
    outer->flex_direction(FlexDirection::Column);
    outer->flex_grow(1.f);
    outer->padding(Edges::all(px(1)));
    outer->style(
        {.background_color = RGBA{0.05f, 0.05f, 0.1f, 1.0f},
         .border_style = BorderStyle::Rounded,
         .border_color = RGBA::from_hex("#444444")}
    );
    outer->focus_style({.border_color = RGBA::from_hex("#00AAFF")});
    outer->title(" stain complete test ");
    outer->title(
        " Tab: cycle focus  |  Ctrl+C: quit  |  mouse: click/scroll ",
        BoxOptions::TitlePos::Bottom
    );

    FocusCycler cycler;

    // Section header helper
    auto section = [&](const char* title) {
        auto t = text(*renderer);
        t->height(px(1));
        t->margin(Edges{.bottom = px(1)});
        t->content(TextBuilder{}.fg(RGBA::from_hex("#FFAA00"), title));
        outer->add(t);
    };

    //  1. BORDER STYLES  /  four boxes in a row
    section("## Box Borders");

    {
        auto row = box(*renderer);
        row->flex_direction(FlexDirection::Row);
        row->height(px(2));
        row->column_gap(px(1));
        row->margin(Edges{.bottom = px(1)});

        auto add_border =
            [&](BorderStyle s, const char* title, const char* bg) {
                auto bx = box(*renderer);
                bx->flex_grow(1.f);
                bx->style(
                    {.background_color = RGBA::from_hex(bg),
                     .border_style = s,
                     .border_color = RGBA::from_hex("#888888")}
                );
                bx->title(title);
                row->add(bx);
            };

        add_border(BorderStyle::Single, " Single ", "#1a1a2e");
        add_border(BorderStyle::Double, " Double ", "#16213e");
        add_border(BorderStyle::Rounded, " Rounded", "#0f3460");
        add_border(BorderStyle::Heavy, " Heavy  ", "#533483");
        outer->add(row);
    }

    //  2. STYLED TEXT  /  TextBuilder with bold, italic, fg, link
    section("## Styled Text (TextBuilder)");

    {
        auto t = text(*renderer);
        t->height(px(1));
        t->bg(RGBA{0.08f, 0.08f, 0.12f, 1.0f});
        t->margin(Edges{.bottom = px(1)});
        t->content(
            TextBuilder{}
                .bold("Bold, ")
                .italic("italic, ")
                .fg(RGBA::from_hex("#FF5555"), "colored, ")
                .append(
                    "bold+italic ",
                    RGBA::white(),
                    RGBA::transparent(),
                    Attr::Bold | Attr::Italic
                )
                .link("https://github.com", "clickable link")
        );

        // Subscribe to Focused/Blurred events
        t->on<events::Focused>([t = t.get()](events::Focused) {
            t->content({TextChunk{
                " Text focused event fired!",
                RGBA::from_hex("#00FF88"),
                {},
                Attr::Bold
            }});
        });
        t->on<events::Blurred>([t = t.get()](events::Blurred) {
            t->content({TextChunk{
                " Text blurred event fired!",
                RGBA::from_hex("#FF8844")
            }});
        });

        outer->add(t);
    }

    //  3. INPUT & TEXTAREA  /  single and multi-line editing
    section("## Input & Textarea");

    // Status line showing input callbacks
    auto input_status = text(*renderer);
    input_status->height(px(1));
    input_status->content(
        {TextChunk{" waiting for input...", RGBA::from_hex("#555555")}}
    );
    outer->add(input_status);

    // Single-line Input
    Renderable* first_focusable = nullptr;
    {
        auto row = box(*renderer);
        row->flex_direction(FlexDirection::Row);
        row->height(px(1));

        auto label = text(*renderer);
        label->width(px(8));
        label->content({TextChunk{" Name: ", RGBA::from_hex("#888888")}});

        auto field = input(*renderer);
        field->focusable(true);
        field->flex_grow(1.f);
        field->placeholder("Type your name and press Enter...");
        field->style({.text = RGBA::white()});
        field->focus_style(
            {.text = RGBA::from_hex("#00FF88"),
             .bg = RGBA{0.05f, 0.1f, 0.05f, 1.0f}}
        );
        field->on_enter([input_status](std::string_view v) {
            input_status->content({TextChunk{
                std::string(" Enter pressed! Value: \"") + std::string(v) +
                    "\"",
                RGBA::from_hex("#88FF88")
            }});
        });
        field->on_change([input_status](std::string_view v) {
            input_status->content({TextChunk{
                std::string(" on_change: \"") + std::string(v) + "\"",
                RGBA::from_hex("#88AAFF")
            }});
        });
        cycler.add(field.get());
        first_focusable = field.get();

        row->add(label);
        row->add(field);
        outer->add(row);
    }

    // Multi-line Textarea
    {
        auto ta = textarea(*renderer);
        ta->focusable(true);
        ta->height(px(2));
        ta->placeholder("Multi-line textarea... (Ctrl+Enter to submit)");
        ta->style({.bg = RGBA{0.06f, 0.06f, 0.08f, 1.0f}});
        ta->focus_style(
            {.text = RGBA::from_hex("#88FF88"),
             .bg = RGBA{0.1f, 0.15f, 0.1f, 1.0f}}
        );
        ta->key_bindings({{"ctrl+return", "submit"}});
        ta->on_submit([input_status](std::string_view v) {
            input_status->content({TextChunk{
                std::string(" Textarea submitted: \"") + std::string(v) + "\"",
                RGBA::from_hex("#FFAA00")
            }});
        });
        ta->text("Hello\nWorld!\nMulti-line editing works.");
        ta->margin(Edges{.bottom = px(1)});
        cycler.add(ta.get());
        outer->add(ta);
    }

    //  4. SELECT & TABSELECT
    section("## Select & TabSelect");

    auto sel_status = text(*renderer);
    sel_status->height(px(1));
    sel_status->content(
        {TextChunk{" select an item...", RGBA::from_hex("#555555")}}
    );
    outer->add(sel_status);

    {
        auto row = box(*renderer);
        row->flex_direction(FlexDirection::Row);
        row->height(px(2));
        row->column_gap(px(1));
        row->margin(Edges{.bottom = px(1)});

        // Select  /  vertical list
        {
            auto sel = select(*renderer);
            sel->focusable(true);
            sel->flex_grow(1.f);
            sel->options({
                {"Red", "red"},
                {"Green", "green"},
                {"Blue", "blue"},
                {"Yellow", "yellow"},
                {"Cyan", "cyan"},
                {"Magenta", "magenta"},
                {"Orange", "orange"},
            });
            sel->selected_style(
                {.text = RGBA::from_hex("#00AAFF"),
                 .bg = RGBA::from_hex("#003366")}
            );
            sel->on_select([sel_status](int idx, const SelectOption& opt) {
                sel_status->content({TextChunk{
                    std::string(" Select changed: #") + std::to_string(idx) +
                        " = " + std::string(opt.label),
                    RGBA::from_hex("#88CCFF")
                }});
            });
            sel->on_activate([sel_status](int idx, const SelectOption& opt) {
                sel_status->content({TextChunk{
                    std::string(" Select ENTER: ") + std::string(opt.label) +
                        " (value=" + std::string(opt.value) + ")",
                    RGBA::from_hex("#FFCC00")
                }});
            });
            cycler.add(sel.get());
            row->add(sel);
        }

        // TabSelect  /  horizontal tabs
        {
            auto tab = tabselect(*renderer);
            tab->focusable(true);
            tab->flex_grow(1.f);
            tab->options({
                {"Files", "files", 10},
                {"Edit", "edit", 10},
                {"View", "view", 10},
                {"Help", "help", 12},
                {"Tools", "tools", 12},
            });
            tab->selected_style(
                {.text = RGBA::from_hex("#00CCFF"),
                 .bg = RGBA::from_hex("#004488")}
            );
            tab->on_select([sel_status](int idx, const TabOption& opt) {
                sel_status->content({TextChunk{
                    std::string(" Tab changed to: ") + std::string(opt.label),
                    RGBA::from_hex("#88CCFF")
                }});
            });
            tab->on_activate([sel_status](int idx, const TabOption& opt) {
                sel_status->content({TextChunk{
                    std::string(" Tab selected: ") + std::string(opt.label),
                    RGBA::from_hex("#FFCC00")
                }});
            });
            cycler.add(tab.get());
            row->add(tab);
        }

        outer->add(row);
    }

    //  5. SCROLLBOX  /  50 items with vertical scrolling
    section("## ScrollBox (scrollable content)");

    {
        auto scroll = scrollbox(*renderer);
        scroll->height(px(4));
        scroll->margin(Edges{.bottom = px(1)});
        scroll->scroll_y(true);

        for(int i = 0; i < 50; i++) {
            std::ostringstream ss;
            ss << " Item #" << i << " --- "
               << "Lorem ipsum dolor sit amet, consectetur adipiscing elit.";
            auto t = text(*renderer);
            t->height(px(1));
            t->content({TextChunk{
                ss.str(),
                (i % 2 == 0) ? RGBA::from_hex("#AAAAAA")
                             : RGBA::from_hex("#CCCCCC")
            }});
            scroll->add(t);
        }
        cycler.add(scroll.get());
        outer->add(scroll);
    }

    //  6. Z-INDEX & OPACITY  /  overlapping layered widgets
    section("## Z-Index & Opacity");

    {
        auto z_box = box(*renderer);
        z_box->height(px(2));
        z_box->margin(Edges{.bottom = px(1)});

        // Background box (z:0)
        {
            BoxOptions b;
            b.position = PositionType::Absolute;
            b.pos_left = px(0);
            b.pos_top = px(0);
            b.width = px(36);
            b.height = px(2);
            b.z_index = 0;
            b.style.background_color = RGBA::from_hex("#1a1a2e");
            b.style.border_style = BorderStyle::Single;
            b.style.border_color = RGBA::from_hex("#666666");
            b.title = " z-index:0 ";
            z_box->add(std::make_shared<Box>(renderer.get(), std::move(b)));
        }

        // Overlapping box (z:1, opacity:70%)
        {
            BoxOptions b;
            b.position = PositionType::Absolute;
            b.pos_left = px(12);
            b.pos_top = px(0);
            b.width = px(24);
            b.height = px(2);
            b.z_index = 1;
            b.opacity = 0.70f;
            b.style.background_color = RGBA::from_hex("#533483");
            b.style.border_style = BorderStyle::Double;
            b.style.border_color = RGBA::from_hex("#AA66CC");
            b.title = " z:1  opacity:70% ";
            z_box->add(std::make_shared<Box>(renderer.get(), std::move(b)));
        }

        // Text on top (z:2)
        {
            TextOptions t;
            t.position = PositionType::Absolute;
            t.pos_left = px(14);
            t.pos_top = px(0);
            t.z_index = 2;
            t.content = {TextChunk{
                " On top! (z:2) ",
                RGBA::from_hex("#FFFF88"),
                RGBA::transparent(),
                Attr::Bold
            }};
            z_box->add(std::make_shared<Text>(renderer.get(), std::move(t)));
        }

        outer->add(z_box);
    }

    //  7. EVENTS, CALLBACKS & LIFECYCLE
    section("## Events, Callbacks & Lifecycle");

    {
        auto ev_box = box(*renderer);
        ev_box->flex_direction(FlexDirection::Column);
        ev_box->height(px(4));
        ev_box->padding(Edges::all(px(1)));
        ev_box->flex_shrink(0.f);
        ev_box->style(
            {.background_color = RGBA{0.06f, 0.06f, 0.1f, 1.0f},
             .border_style = BorderStyle::Single,
             .border_color = RGBA::from_hex("#555555")}
        );
        ev_box->margin(Edges{.bottom = px(1)});

        // CallbackDemo uses render_before / render_after callbacks
        ev_box->add(std::make_shared<CallbackDemo>(renderer.get()));

        // Animated counter with lifecycle pass
        ev_box->add(
            std::make_shared<AnimatedCounter>(
                renderer.get(),
                RGBA::from_hex("#66FF66")
            )
        );

        outer->add(ev_box);
    }

    //  8. PER-SIDE BORDERS
    section("## Per-Side Borders");

    {
        auto row = box(*renderer);
        row->flex_direction(FlexDirection::Row);
        row->height(px(2));
        row->column_gap(px(1));
        row->margin(Edges{.bottom = px(1)});

        // Top + bottom borders only
        {
            auto bx = box(*renderer);
            bx->flex_grow(1.f);
            bx->style(
                {.background_color = RGBA{0.04f, 0.08f, 0.04f, 1.0f},
                 .border_style = BorderStyle::Double,
                 .border_sides = {.right = false, .left = false},
                 .border_color = RGBA::from_hex("#88FF88")}
            );
            bx->title(" top+bottom ");
            auto t = text(*renderer);
            t->content({TextChunk{
                " Only top & bottom borders visible.",
                RGBA::from_hex("#AAAAAA")
            }});
            bx->add(t);
            row->add(bx);
        }

        // Left + right borders only
        {
            auto bx = box(*renderer);
            bx->flex_grow(1.f);
            bx->style(
                {.background_color = RGBA{0.08f, 0.06f, 0.02f, 1.0f},
                 .border_style = BorderStyle::Single,
                 .border_sides = {.top = false, .bottom = false},
                 .border_color = RGBA::from_hex("#FFAA55")}
            );
            bx->title(" left+right ");
            auto t = text(*renderer);
            t->content({TextChunk{
                " Only left & right borders visible.",
                RGBA::from_hex("#AAAAAA")
            }});
            bx->add(t);
            row->add(bx);
        }

        outer->add(row);
    }

    //  9. TEXT WRAP MODES
    section("## Word Wrap Modes: Word / Char / None");

    {
        auto row = box(*renderer);
        row->flex_direction(FlexDirection::Row);
        row->height(px(2));
        row->column_gap(px(1));
        row->margin(Edges{.bottom = px(1)});

        std::string lorem = "The quick brown fox jumps over the lazy dog. ";
        auto add_wrap = [&](WrapMode mode, const char* title) {
            auto t = text(*renderer);
            t->flex_grow(1.f);
            t->content({TextChunk{
                std::string(title) + ": " + lorem,
                RGBA::from_hex("#CCCCCC")
            }});
            t->wrap(mode);
            row->add(t);
        };

        add_wrap(WrapMode::Word, "Word");
        add_wrap(WrapMode::Char, "Char");
        add_wrap(WrapMode::None, "None");
        outer->add(row);
    }

    //  HELP / STATUS FOOTER
    {
        auto h = text(*renderer);
        h->height(px(1));
        h->content(
            TextBuilder{}.fg(
                RGBA::from_hex("#666666"),
                " Tab: cycle focus  |  Cursors/arrows: navigate  |  Enter: "
                "select/confirm  |  Ctrl+C: quit"
            )
        );
        outer->add(h);
    }

    //  ASSEMBLE & RUN
    root.add(outer);

    // Register global key handler for Tab focus cycling
    renderer->on_key([&cycler](KeyEvent& key) -> bool {
        return !cycler.handle_key(key);
    });

    // Set initial focus
    if(first_focusable)
        first_focusable->focus();

    renderer->run();
    return 0;
}
