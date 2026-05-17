#include <stain/box.hpp>
#include <stain/framebuffer.hpp>
#include <stain/renderer.hpp>
#include <stain/text.hpp>
#include <stain/timeline.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <vector>

using namespace stain;

static constexpr float PI = 3.14159265f;

static const char32_t BALL[3][3] = {
    {U' ', U'▓', U' '},
    {U'▓', U'█', U'▓'},
    {U' ', U'▓', U' '},
};

class BouncingBall : public Renderable {
    public:
    BouncingBall(RenderContext* ctx, RGBA color)
        : Renderable(ctx, RenderableOptions{})
        , _color(color) {
        width(px(3));
        height(px(3));
        live(true);
    }

    void on_lifecycle_pass(double dt) override {
        int pw = _renderable_parent ? _renderable_parent->layout_w() : 40;
        int ph = _renderable_parent ? _renderable_parent->layout_h() : 12;
        int area_w = std::max(pw - 2, 1);
        int area_h = std::max(ph - 2, 1);

        // Bounce bounds account for the 3x3 ball size.
        float max_x = float(area_w - 3);
        float max_y = float(area_h - 3);

        _px += _vx * dt;
        _py += _vy * dt;

        if(_px < 0) {
            _px = 0;
            _vx = -_vx;
        }
        if(_px > max_x) {
            _px = max_x;
            _vx = -_vx;
        }
        if(_py < 0) {
            _py = 0;
            _vy = -_vy;
        }
        if(_py > max_y) {
            _py = max_y;
            _vy = -_vy;
        }

        _x = int(std::round(_px));
        _y = int(std::round(_py));

        translate(_x, _y);
    }

    protected:
    void draw(OptimizedBuffer& buf, double) override {
        int sx = screen_x();
        int sy = screen_y();
        for(int r = 0; r < 3; r++)
            for(int c = 0; c < 3; c++)
                if(BALL[r][c] != U' ')
                    buf.set_cell(sx + c, sy + r, Cell{BALL[r][c], _color});
    }

    private:
    RGBA _color;
    int _x{0}, _y{0};
    float _px{5}, _py{2};
    float _vx{8}, _vy{5};
};

class ColorFill : public Renderable {
    public:
    ColorFill(RenderContext* ctx, RGBA panel_bg)
        : Renderable(ctx, RenderableOptions{})
        , _panel_bg(panel_bg) {
        flex_grow(1.f);
        live(true);

        auto tl = std::make_shared<Timeline>();
        tl->duration(12000).loop(true);
        tl->animate({{"hue", 0.0f}}, {{"hue", {360.0f, easing::linear}}}, 12000)
            .on_update([this](Animation& a) {
                _hue = a.get("hue");
                update_color();
                request_render();
            });
        _ctx->add_timeline(tl);
    }

    protected:
    void draw(OptimizedBuffer& buf, double) override {
        buf.fill_rect(screen_x(), screen_y(), layout_w(), layout_h(), _color);
    }

    private:
    void update_color() {
        RGBA color = stain::from_hue(_hue);
        float t = std::sin(float(_t) * 1.2f) * 0.4f + 0.6f;
        _color = {
            color.r * t + _panel_bg.r * (1.0f - t),
            color.g * t + _panel_bg.g * (1.0f - t),
            color.b * t + _panel_bg.b * (1.0f - t),
            1.0f,
        };
        _t += 0.016f;
    }

    RGBA _panel_bg;
    float _hue{0};
    float _t{0};
    RGBA _color{RGBA::transparent()};
};

struct Particle {
    float x, y;
    float vx, vy;
    float life;
    RGBA color;
};

class ParticleSystem {
    public:
    ParticleSystem(int max_particles, float speed)
        : _max(max_particles)
        , _speed(speed) {
        _particles.reserve(max_particles);
    }

    void update(float dt, int w, int h) {
        _emit_cooldown -= dt;
        if(_emit_cooldown <= 0.0f && int(_particles.size()) < _max) {
            float angle = (std::rand() % 6283) / 1000.0f;
            _particles.push_back({
                float(w) / 2.0f,
                float(h) / 2.0f,
                std::cos(angle) * _speed,
                std::sin(angle) * _speed * 0.6f,
                1.0f,
                RGBA{
                    0.5f + std::sin(angle) * 0.5f,
                    0.3f + std::cos(angle * 1.3f) * 0.3f,
                    0.8f + std::sin(angle * 0.7f) * 0.2f,
                    1.0f,
                },
            });
            _emit_cooldown = 0.02f;
        }

        for(auto& p : _particles) {
            p.x += p.vx * dt * 20.0f;
            p.y += p.vy * dt * 20.0f;
            p.vx *= 0.98f;
            p.vy *= 0.98f;
            p.life -= dt * 0.8f;
        }

        std::erase_if(_particles, [&](const Particle& p) {
            return p.life <= 0.0f || p.x < 0 || p.x >= w || p.y < 0 || p.y >= h;
        });
    }

    void draw(OptimizedBuffer& buf) {
        for(const auto& p : _particles) {
            int px = int(p.x);
            int py = int(p.y);
            if(px < 0 || px >= buf.width() || py < 0 || py >= buf.height())
                continue;
            float b = std::clamp(p.life, 0.0f, 1.0f);
            char32_t ch = b > 0.7f   ? U'█'
                          : b > 0.4f ? U'▓'
                          : b > 0.2f ? U'▒'
                                     : U'░';
            buf.set_cell(
                px,
                py,
                Cell{ch, {p.color.r * b, p.color.g * b, p.color.b * b, b}}
            );
        }
    }

    private:
    int _max;
    float _speed;
    float _emit_cooldown{0};
    std::vector<Particle> _particles;
};

static void draw_sine_glow(OptimizedBuffer& buf, float t) {
    int w = buf.width();
    int h = buf.height();
    for(int y = 0; y < h; y++) {
        for(int x = 0; x < w; x++) {
            float fx = float(x) / float(w);
            float fy = float(y) / float(h);
            float v = std::sin(fx * 6.0f + t * 2.0f) * 0.4f +
                      std::cos(fy * 5.0f + t * 1.3f) * 0.4f +
                      std::sin((fx + fy) * 4.0f + t * 0.9f) * 0.2f;
            v = v * 0.5f + 0.5f;
            v = std::clamp(v, 0.0f, 1.0f) * 0.12f;
            if(v < 0.01f)
                continue;
            Cell existing = buf.get_cell(x, y);
            RGBA glow{0.1f, 0.05f, 0.2f, v};
            buf.set_cell(
                x,
                y,
                Cell{existing.ch, existing.fg, glow.blend_over(existing.bg)}
            );
        }
    }
}

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
    outer->padding(Edges::all(px(1)));
    outer->style({
        .background_color = RGBA{0.02f, 0.02f, 0.06f, 1.0f},
        .border_style = BorderStyle::Rounded,
        .border_color = RGBA::from_hex("#444466"),
    });
    outer->title(" animation test ");

    {
        auto t = text(*renderer);
        t->height(px(1));
        t->margin(Edges{.bottom = px(1)});
        t->content(
            TextBuilder{}
                .fg(RGBA::from_hex("#FFDD44"), "  ● bounce  ")
                .fg(RGBA::from_hex("#88FF88"), "  ◆ particles  ")
                .fg(RGBA::from_hex("#FF8844"), "  ◇ color fill  ")
                .fg(RGBA::from_hex("#666666"),
                    "  |  translate + opacity + on_frame")
        );
        outer->add(t);
    }

    {
        auto row = box(*renderer);
        row->flex_direction(FlexDirection::Row);
        row->flex_grow(1.f);
        row->column_gap(px(1));

        // Bounce panel
        auto ball_panel = box(*renderer);
        ball_panel->flex_grow(1.f);
        ball_panel->style({
            .background_color = RGBA{0.03f, 0.03f, 0.08f, 1.0f},
            .border_style = BorderStyle::Single,
            .border_color = RGBA::from_hex("#335566"),
        });
        ball_panel->title(" bounce (translate) ");
        ball_panel->add(
            std::make_shared<BouncingBall>(
                renderer.get(),
                RGBA::from_hex("#FFDD44")
            )
        );
        row->add(ball_panel);

        auto particle_panel = box(*renderer);
        particle_panel->flex_grow(1.f);
        particle_panel->style({
            .background_color = RGBA{0.03f, 0.03f, 0.08f, 1.0f},
            .border_style = BorderStyle::Single,
            .border_color = RGBA::from_hex("#446644"),
        });
        particle_panel->title(" particles (framebuffer) ");

        auto fb = framebuffer(*renderer);
        fb->live(true);
        fb->flex_grow(1.f);

        auto particles = std::make_shared<ParticleSystem>(80, 1.5f);
        fb->on_frame([particles](OptimizedBuffer& buf) {
            int w = buf.width();
            int h = buf.height();
            if(w <= 0 || h <= 0)
                return;
            static float t = 0;
            float dt = 1.0f / 60.0f;
            t += dt;
            buf.clear(RGBA::transparent());
            particles->update(dt, w, h);
            particles->draw(buf);
            draw_sine_glow(buf, t);
        });

        particle_panel->add(fb);
        row->add(particle_panel);

        auto fill_panel = box(*renderer);
        fill_panel->flex_grow(1.f);
        fill_panel->style({
            .background_color = RGBA{0.03f, 0.03f, 0.08f, 1.0f},
            .border_style = BorderStyle::Single,
            .border_color = RGBA::from_hex("#553344"),
        });
        fill_panel->title(" color fill (hue + fade) ");

        fill_panel->add(
            std::make_shared<ColorFill>(
                renderer.get(),
                RGBA{0.03f, 0.03f, 0.08f, 1.0f}
            )
        );
        row->add(fill_panel);

        outer->add(row);
    }

    {
        auto f = text(*renderer);
        f->height(px(1));
        f->margin(Edges{.top = px(1)});
        f->content(
            TextBuilder{}.fg(
                RGBA::from_hex("#666666"),
                " q / Ctrl+C: quit  |  "
                "Timeline + on_lifecycle_pass(dt) + translate() + opacity() + on_frame()"
            )
        );
        outer->add(f);
    }

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
