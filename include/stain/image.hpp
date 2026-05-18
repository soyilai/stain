#pragma once

#include "stain/renderable.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace stain {

// Image scaling filter.
enum class ImageScale { Nearest, Bilinear };

// How the image fits within the widget bounds.
enum class ImageFit { Contain, Fill, Cover };

// Terminal image protocol to use for rendering.
enum class ImageProtocol {
    Auto,      // auto-detect best available
    Kitty,     // Kitty graphics protocol
    ITerm2,    // iTerm2 inline images
    HalfBlock, // Unicode half-block fallback
};

// Renders a bitmap image. Uses terminal graphics protocols (Kitty, iTerm2)
// when available, falling back to Unicode half-block characters (U+2580)
// for 2x vertical resolution.
//
// Usage:
//   auto img = image(*renderer);
//   img->load("photo.png");
//   img->width(px(40));
//   img->height(px(20));
class Image : public Renderable {
    public:
    explicit Image(RenderContext* ctx, RenderableOptions opts = {});

    // Load from file path. Returns true on success.
    bool load(std::string_view path);

    // Load from memory buffer. Returns true on success.
    bool load(const unsigned char* data, std::size_t len);

    // Set pixel data directly. Takes ownership.
    void set_pixels(int width, int height, std::vector<RGBA> pixels);

    [[nodiscard]] int image_width() const noexcept {
        return _img_w;
    }
    [[nodiscard]] int image_height() const noexcept {
        return _img_h;
    }
    [[nodiscard]] bool has_image() const noexcept {
        return !_pixels.empty();
    }

    Image& filter(ImageScale f);
    Image& fit(ImageFit f);
    Image& protocol(ImageProtocol p);
    void clear();

    protected:
    void draw(OptimizedBuffer& buf, double delta) override;

    private:
    std::vector<RGBA> _pixels;
    std::vector<unsigned char> _file_data;
    std::string _protocol_b64;   // base64-encoded RGBA for Kitty
    std::string _png_b64;        // base64-encoded PNG for iTerm2
    int _img_w{0};
    int _img_h{0};
    ImageScale _filter{ImageScale::Nearest};
    ImageFit _fit{ImageFit::Contain};
    ImageProtocol _protocol{ImageProtocol::Auto};
    int _kitty_img_id{0};
    int _kitty_place_id{0};
    bool _kitty_transmitted{false};

    [[nodiscard]] ImageProtocol effective_protocol() const;

    [[nodiscard]] RGBA pixel_at(int x, int y) const;
    [[nodiscard]] RGBA sample(float fx, float fy) const;
};

// Factory function.
inline std::shared_ptr<Image> image(RenderContext& ctx) {
    return std::make_shared<Image>(&ctx);
}

} // namespace stain
