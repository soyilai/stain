#pragma once

#include "stain/renderable.hpp"

#include <string>
#include <string_view>

namespace stain {

enum class AsciiFontType { Tiny, Block };

class AsciiFont : public Renderable {
    public:
    explicit AsciiFont(RenderContext* ctx, RenderableOptions opts = {});

    AsciiFont& text(std::string_view t);
    AsciiFont& font(AsciiFontType f);
    AsciiFont& fg(RGBA color);
    AsciiFont& bg(RGBA color);

    [[nodiscard]] const std::string& text() const noexcept {
        return _text;
    }
    [[nodiscard]] AsciiFontType font_type() const noexcept {
        return _font;
    }

    protected:
    void draw(OptimizedBuffer& buf, double delta) override;

    private:
    std::string _text;
    AsciiFontType _font{AsciiFontType::Tiny};
    RGBA _fg{RGBA::white()};
    RGBA _bg{RGBA::transparent()};
};

inline std::shared_ptr<AsciiFont> ascii_font(RenderContext& ctx) {
    return std::make_shared<AsciiFont>(&ctx);
}

} // namespace stain
