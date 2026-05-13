#pragma once

#include "stain/renderable.hpp"

#include <vector>

namespace stain {

    // A styled text fragment with optional hyperlink.
    struct TextChunk {
        std::string text;
        RGBA fg{RGBA::white()};
        RGBA bg{RGBA::transparent()};
        Attr attr{Attr::None};
        std::string link_url{};
    };

    // A sequence of styled text chunks.
    using StyledText = std::vector<TextChunk>;

    // Builder for constructing StyledText with chained calls.
    // Usage:
    //   StyledText t = TextBuilder{}
    //       .append("hello ", RGBA::white())
    //       .bold("world")
    //       .link("https://example.com", "link");
    class TextBuilder {
        public:
        // Append text with explicit styling.
        TextBuilder& append(
            std::string_view text,
            RGBA fg = RGBA::white(),
            RGBA bg = RGBA::transparent(),
            Attr attr = Attr::None
        );
        // Append bold text.
        TextBuilder& bold(std::string_view text);
        // Append italic text.
        TextBuilder& italic(std::string_view text);
        // Append text with a specific foreground color.
        TextBuilder& fg(RGBA color, std::string_view text);
        // Append a hyperlink.
        TextBuilder& link(std::string_view url, std::string_view label);
        operator StyledText() const {
            return _chunks;
        }

        private:
        StyledText _chunks;
    };

    // Text wrapping behavior.
    enum class WrapMode { Word, Char, None };

    struct TextOptions : RenderableOptions {
        StyledText content{};
        WrapMode wrap_mode{WrapMode::Word};
        RGBA fg{RGBA::white()};
        RGBA bg{RGBA::transparent()};
        Attr attr{Attr::None};
        bool selectable{false};
    };

    // A renderable that displays styled, wrapping text with optional links.
    class Text : public Renderable {
        public:
        explicit Text(RenderContext* ctx, TextOptions opts = {});

        [[nodiscard]] static std::shared_ptr<Text> create(RenderContext* ctx) {
            return std::make_shared<Text>(ctx);
        }

        // Set content as styled text or plain text.
        Text& content(StyledText text);
        Text& content(std::string_view plain_text);
        [[nodiscard]] const StyledText& content() const noexcept;

        // Style setters for default text appearance.
        Text& fg(RGBA color);
        Text& bg(RGBA color);
        Text& attr(Attr a);
        Text& wrap(WrapMode m);

        protected:
        void draw(OptimizedBuffer& buf, double delta) override;
        void process_mouse_event(MouseEvent& event) override;

        private:
        struct LinkRegion {
            int x, y, w;
            std::string url;
        };
        TextOptions _text_opts;
        StyledText _text;
        std::vector<LinkRegion> _link_regions;
    };

    // Factory function for a Text renderable.
    inline std::shared_ptr<Text> text(RenderContext& ctx) {
        return Text::create(&ctx);
    }

    // A leaf text node that can be embedded in a render tree. Gathers styled
    // text from its children for use by a parent Text renderable.
    class TextNode : public BaseRenderable {
        public:
        explicit TextNode(
            RenderContext* ctx,
            std::string text = {},
            RGBA fg = RGBA::white(),
            Attr attr = Attr::None
        );

        void text(std::string_view text);
        void fg(RGBA color);
        void attr(Attr a);

        [[nodiscard]] StyledText
        gather(RGBA inherited_fg, RGBA inherited_bg, Attr inherited_attr) const;

        int add(
            std::shared_ptr<BaseRenderable> child,
            std::optional<int> index
        ) override;
        void remove(std::string_view) override {
        }
        void insert_before(
            std::shared_ptr<BaseRenderable>,
            BaseRenderable*
        ) override {
        }
        std::vector<BaseRenderable*> children() const override {
            return {};
        }
        int child_count() const noexcept override {
            return 0;
        }
        BaseRenderable* find(std::string_view) const override {
            return nullptr;
        }
        void request_render() override;

        private:
        std::string _text;
        RGBA _fg;
        Attr _attr;
        std::vector<std::shared_ptr<TextNode>> _text_children;
    };

} // namespace stain
