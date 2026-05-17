#pragma once

#include "stain/buffer.hpp"
#include "stain/color.hpp"
#include "stain/context.hpp"
#include "stain/event.hpp"
#include "stain/renderable.hpp"

#include <functional>
#include <memory>

namespace stain {

    // Whether to use the alternate screen buffer.
    enum class ScreenMode { AlternateScreen, MainScreen };

    // Options for creating a Renderer.
    // Usage:
    //   auto renderer = Renderer::create({
    //       ._target_fps = 60,
    //       ._background_color = RGBA::black()
    //   });
    struct RendererOptions {
        int _width{0}; // 0 = detect from terminal
        int _height{0};
        ScreenMode _screen_mode{ScreenMode::AlternateScreen};
        bool _exit_on_ctrl_c{true};
        int _target_fps{30};
        bool _use_mouse{true};
        bool _use_kitty_keyboard{true};
        RGBA _background_color{RGBA::transparent()};
        bool _testing{false}; // Suppress terminal setup for tests

        RendererOptions& target_fps(int v) {
            _target_fps = v;
            return *this;
        }
        RendererOptions& background_color(RGBA v) {
            _background_color = v;
            return *this;
        }
        RendererOptions& use_mouse(bool v) {
            _use_mouse = v;
            return *this;
        }
        RendererOptions& use_kitty_keyboard(bool v) {
            _use_kitty_keyboard = v;
            return *this;
        }
        RendererOptions& screen_mode(ScreenMode v) {
            _screen_mode = v;
            return *this;
        }
        RendererOptions& exit_on_ctrl_c(bool v) {
            _exit_on_ctrl_c = v;
            return *this;
        }
        RendererOptions& width(int v) {
            _width = v;
            return *this;
        }
        RendererOptions& height(int v) {
            _height = v;
            return *this;
        }
        RendererOptions& testing(bool v) {
            _testing = v;
            return *this;
        }
    };

    class Renderer final : public RenderContext {
        public:
        [[nodiscard]] static std::unique_ptr<Renderer>
        create(RendererOptions opts = {});
        ~Renderer() override;

        int width() const noexcept override;
        int height() const noexcept override;
        void request_render() override;
        void request_live() override;
        void drop_live() override;
        void focus_renderable(Renderable* r) override;
        Renderable* focused() const noexcept override;
        void cursor(int col, int row, bool visible) override;
        void cursor_style(CursorOptions opts) override;
        void mouse_pointer(MousePointer shape) override;
        void add_to_hit_grid(int x, int y, int w, int h, int num) override;
        void push_hit_scissor(ClipRect rect) override;
        void pop_hit_scissor() override;
        void clear_hit_grid() override;
        void register_lifecycle(Renderable* r) override;
        void unregister_lifecycle(Renderable* r) override;
        void add_timeline(std::shared_ptr<Timeline> tl) override;
        void remove_timeline(Timeline* tl) override;
        std::optional<Selection> get_selection() const override;
        void clear_selection() override;
        void start_selection(Renderable* r, int x, int y) override;
        void update_selection(Renderable* r, int x, int y) override;
        void clipboard_copy(std::string_view text) override;
        std::size_t on_key(std::function<bool(KeyEvent&)> handler) override;
        void off_key(std::size_t id) override;
        std::size_t on_paste(std::function<void(PasteEvent&)> handler) override;
        void off_paste(std::size_t id) override;

        // Access the root renderable.
        [[nodiscard]] Renderable& root();

        // Run the render loop. Blocks until destroy() is called.
        void run();

        // Stop the render loop.
        void destroy();

        private:
        explicit Renderer(RendererOptions opts);

        void activate_frame();
        void run_lifecycle_passes(double delta);
        void update_layout();
        void render_frame(double delta);
        void diff_and_flush();

        void handle_stdin(const char* data, std::size_t len);
        void parse_key_sequence(const char* data, std::size_t len);
        void parse_mouse_sequence(const char* data, std::size_t len);

        struct Impl;
        std::unique_ptr<Impl> _impl;
    };

} // namespace stain
