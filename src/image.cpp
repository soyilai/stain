#include "stain/image.hpp"
#include "stain/terminal.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "stb_image_write.h"
#pragma GCC diagnostic pop

#include <cstring>
#include <sstream>
#include <vector>

namespace stain {

    namespace {

        std::string base64_encode(std::string_view data) {
            static constexpr char tbl[] =
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
                "+/";
            std::string out;
            out.reserve(((data.size() + 2) / 3) * 4);
            std::size_t i = 0;
            while(i + 3 <= data.size()) {
                auto a = static_cast<uint8_t>(data[i]);
                auto b = static_cast<uint8_t>(data[i + 1]);
                auto c = static_cast<uint8_t>(data[i + 2]);
                out.push_back(tbl[a >> 2]);
                out.push_back(tbl[((a << 4) | (b >> 4)) & 0x3F]);
                out.push_back(tbl[((b << 2) | (c >> 6)) & 0x3F]);
                out.push_back(tbl[c & 0x3F]);
                i += 3;
            }
            if(i < data.size()) {
                auto a = static_cast<uint8_t>(data[i]);
                out.push_back(tbl[a >> 2]);
                if(i + 1 < data.size()) {
                    auto b = static_cast<uint8_t>(data[i + 1]);
                    out.push_back(tbl[((a << 4) | (b >> 4)) & 0x3F]);
                    out.push_back(tbl[(b << 2) & 0x3F]);
                    out.push_back('=');
                } else {
                    out.push_back(tbl[(a << 4) & 0x3F]);
                    out.push_back('=');
                    out.push_back('=');
                }
            }
            return out;
        }

        struct WriteContext {
            std::vector<unsigned char> buf;
        };

        void png_write_func(void* context, void* data, int size) {
            auto* ctx = static_cast<WriteContext*>(context);
            auto* bytes = static_cast<unsigned char*>(data);
            ctx->buf.insert(ctx->buf.end(), bytes, bytes + size);
        }

        std::string encode_png_base64(
            int w, int h, const std::vector<RGBA>& pixels
        ) {
            // Convert RGBA float to RGBA bytes
            std::vector<unsigned char> raw(
                static_cast<std::size_t>(w) * h * 4
            );
            for(int i = 0; i < w * h; i++) {
                auto* p = raw.data() + i * 4;
                p[0] = pixels[i].r8();
                p[1] = pixels[i].g8();
                p[2] = pixels[i].b8();
                p[3] = pixels[i].a8();
            }

            WriteContext ctx;
            stbi_write_png_to_func(
                png_write_func, &ctx, w, h, 4, raw.data(), w * 4
            );

            return base64_encode(
                std::string_view(
                    reinterpret_cast<const char*>(ctx.buf.data()),
                    ctx.buf.size()
                )
            );
        }

        std::string encode_rgba_base64(
            int w, int h, const std::vector<RGBA>& pixels
        ) {
            std::string raw(static_cast<std::size_t>(w) * h * 4, '\0');
            for(int i = 0; i < w * h; i++) {
                auto* p = reinterpret_cast<unsigned char*>(raw.data()) + i * 4;
                p[0] = pixels[i].r8();
                p[1] = pixels[i].g8();
                p[2] = pixels[i].b8();
                p[3] = pixels[i].a8();
            }
            return base64_encode(raw);
        }

        static RGBA lerp(RGBA a, RGBA b, float t) {
            return {
                a.r + (b.r - a.r) * t,
                a.g + (b.g - a.g) * t,
                a.b + (b.b - a.b) * t,
                a.a + (b.a - a.a) * t,
            };
        }

        // Image ID counter for Kitty protocol.
        static int g_image_id = 0;

        // Maximum bytes per Kitty chunk (base64 output).
        static constexpr int KITTY_CHUNK_SIZE = 4096;

    } // anonymous namespace

    Image::Image(RenderContext* ctx, RenderableOptions opts)
        : Renderable(ctx, std::move(opts))
        , _kitty_place_id(++g_image_id) {
    }

    bool Image::load(std::string_view path) {
        // Read file bytes (for iTerm2 protocol which needs original format).
        FILE* f = std::fopen(path.data(), "rb");
        if(!f)
            return false;
        std::fseek(f, 0, SEEK_END);
        long file_size = std::ftell(f);
        std::fseek(f, 0, SEEK_SET);
        std::vector<unsigned char> file_data(
            static_cast<std::size_t>(file_size)
        );
        if(file_size > 0) {
            std::fread(file_data.data(), 1, static_cast<std::size_t>(file_size),
                       f);
        }
        std::fclose(f);

        int w, h, channels;
        unsigned char* data =
            stbi_load(path.data(), &w, &h, &channels, STBI_rgb_alpha);
        if(!data)
            return false;

        std::vector<RGBA> pixels(static_cast<std::size_t>(w) * h);
        for(int i = 0; i < w * h; i++) {
            auto* p = data + i * 4;
            pixels[i] = RGBA{
                p[0] / 255.0f,
                p[1] / 255.0f,
                p[2] / 255.0f,
                p[3] / 255.0f,
            };
        }

        stbi_image_free(data);

        _file_data = std::move(file_data);
        set_pixels(w, h, std::move(pixels));
        return true;
    }

    bool Image::load(const unsigned char* data, std::size_t len) {
        std::vector<unsigned char> file_data(data, data + len);

        int w, h, channels;
        unsigned char* decoded = stbi_load_from_memory(
            data, static_cast<int>(len), &w, &h, &channels, STBI_rgb_alpha
        );
        if(!decoded)
            return false;

        std::vector<RGBA> pixels(static_cast<std::size_t>(w) * h);
        for(int i = 0; i < w * h; i++) {
            auto* p = decoded + i * 4;
            pixels[i] = RGBA{
                p[0] / 255.0f,
                p[1] / 255.0f,
                p[2] / 255.0f,
                p[3] / 255.0f,
            };
        }

        stbi_image_free(decoded);

        _file_data = std::move(file_data);
        set_pixels(w, h, std::move(pixels));
        return true;
    }

    void Image::set_pixels(int width, int height, std::vector<RGBA> pixels) {
        _img_w = width;
        _img_h = height;
        _pixels = std::move(pixels);
        _kitty_transmitted = false;

        // Pre-encode for protocols.
        {
            _protocol_b64 = encode_rgba_base64(_img_w, _img_h, _pixels);
        }

        // Pre-encode PNG for iTerm2.
        {
            _png_b64 = encode_png_base64(_img_w, _img_h, _pixels);
        }

        request_render();
    }

    Image& Image::filter(ImageScale f) {
        _filter = f;
        request_render();
        return *this;
    }

    Image& Image::fit(ImageFit f) {
        _fit = f;
        request_render();
        return *this;
    }

    Image& Image::protocol(ImageProtocol p) {
        _protocol = p;
        _kitty_transmitted = false;
        _kitty_img_id = 0;
        request_render();
        return *this;
    }

    void Image::clear() {
        _file_data.clear();
        _protocol_b64.clear();
        _png_b64.clear();
        _img_w = 0;
        _img_h = 0;
        _kitty_transmitted = false;
        _kitty_img_id = 0;
        request_render();
    }

    ImageProtocol Image::effective_protocol() const {
        if(_protocol != ImageProtocol::Auto)
            return _protocol;

        const auto& info = _ctx->terminal_info();
        if(info.has_kitty_graphics)
            return ImageProtocol::Kitty;
        if(info.has_iterm2_images)
            return ImageProtocol::ITerm2;
        return ImageProtocol::HalfBlock;
    }

    RGBA Image::pixel_at(int x, int y) const {
        if(x < 0 || x >= _img_w || y < 0 || y >= _img_h)
            return RGBA::transparent();
        return _pixels[static_cast<std::size_t>(y) * _img_w + x];
    }

    RGBA Image::sample(float fx, float fy) const {
        if(_filter == ImageScale::Nearest) {
            return pixel_at(
                static_cast<int>(fx + 0.5f),
                static_cast<int>(fy + 0.5f)
            );
        }

        int ix = static_cast<int>(fx);
        int iy = static_cast<int>(fy);
        float dx = fx - static_cast<float>(ix);
        float dy = fy - static_cast<float>(iy);

        auto clamp = [](int v, int lo, int hi) {
            return v < lo ? lo : (v > hi ? hi : v);
        };
        int mx = _img_w - 1;
        int my = _img_h - 1;

        RGBA c00 = pixel_at(clamp(ix, 0, mx), clamp(iy, 0, my));
        RGBA c10 = pixel_at(clamp(ix + 1, 0, mx), clamp(iy, 0, my));
        RGBA c01 = pixel_at(clamp(ix, 0, mx), clamp(iy + 1, 0, my));
        RGBA c11 = pixel_at(clamp(ix + 1, 0, mx), clamp(iy + 1, 0, my));

        return lerp(lerp(c00, c10, dx), lerp(c01, c11, dx), dy);
    }

    void Image::draw(OptimizedBuffer& buf, double /*delta*/) {
        int w = layout_w();
        int h = layout_h();
        if(w <= 0 || h <= 0 || _pixels.empty())
            return;

        int sx = screen_x();
        int sy = screen_y();

        auto proto = effective_protocol();
        if(proto != ImageProtocol::HalfBlock) {
            // For protocol rendering, leave cells transparent so the
            // terminal graphics can render on top without interference.
            // The protocol sequence is queued via write_after_flush.

            // Clear the cell area so nothing visible sits under the overlay.
            for(int cy = 0; cy < h; cy++)
                for(int cx = 0; cx < w; cx++)
                    buf.set_cell(
                        sx + cx, sy + cy, Cell{U' ', RGBA::transparent(), RGBA::transparent()}
                    );

            if(proto == ImageProtocol::Kitty) {
                // Transmit image data (one-time). Use a=t (lowercase) to
                // transmit only — a=T would also place, creating a stray
                // placement at the draw-time cursor position.
                if(!_kitty_transmitted) {
                    _kitty_img_id = ++g_image_id;

                    std::string& b64 = _protocol_b64;
                    std::size_t pos = 0;
                    bool first = true;

                    while(pos < b64.size()) {
                        std::size_t chunk_size = std::min(
                            static_cast<std::size_t>(KITTY_CHUNK_SIZE),
                            b64.size() - pos
                        );
                        bool last = (pos + chunk_size >= b64.size());

                        std::string chunk = "\033_G";
                        if(first) {
                            chunk +=
                                "a=t,f=32,s=" + std::to_string(_img_w) +
                                ",v=" + std::to_string(_img_h) +
                                ",i=" + std::to_string(_kitty_img_id);
                            if(!last)
                                chunk += ",m=1";
                            first = false;
                        } else {
                            chunk += "m=";
                            chunk += last ? "0" : "1";
                        }
                        chunk += ";";
                        chunk.append(b64, pos, chunk_size);
                        chunk += "\033\\";
                        _ctx->write_raw(chunk);

                        pos += chunk_size;
                    }

                    _kitty_transmitted = true;
                }

                // Compute display pixel size from widget cell dimensions.
                // Without w/h the image renders at its native pixel size.
                const auto& tinfo = _ctx->terminal_info();
                int cell_px_w = 10, cell_px_h = 20;
                if(tinfo.pixel_width > 0 && tinfo.cols > 0)
                    cell_px_w = tinfo.pixel_width / tinfo.cols;
                if(tinfo.pixel_height > 0 && tinfo.rows > 0)
                    cell_px_h = tinfo.pixel_height / tinfo.rows;

                int area_px_w = w * cell_px_w;
                int area_px_h = h * cell_px_h;
                int disp_w = area_px_w;
                int disp_h = area_px_h;

                if(_fit == ImageFit::Contain) {
                    float img_aspect =
                        static_cast<float>(_img_w) / static_cast<float>(_img_h);
                    float area_aspect =
                        static_cast<float>(disp_w) / static_cast<float>(disp_h);
                    if(img_aspect > area_aspect) {
                        disp_h = std::max(1, static_cast<int>(disp_w / img_aspect));
                    } else {
                        disp_w = std::max(1, static_cast<int>(disp_h * img_aspect));
                    }
                } else if(_fit == ImageFit::Cover) {
                    float img_aspect =
                        static_cast<float>(_img_w) / static_cast<float>(_img_h);
                    float area_aspect =
                        static_cast<float>(disp_w) / static_cast<float>(disp_h);
                    if(img_aspect > area_aspect) {
                        disp_w = std::max(1, static_cast<int>(disp_h * img_aspect));
                    } else {
                        disp_h = std::max(1, static_cast<int>(disp_w / img_aspect));
                    }
                }

                std::string place_seq =
                    "\033_Ga=p,i=" + std::to_string(_kitty_img_id) +
                    ",p=" + std::to_string(_kitty_place_id) +
                    ",c=0,r=0,w=" + std::to_string(disp_w) +
                    ",h=" + std::to_string(disp_h) + ";\033\\";
                _ctx->write_after_flush(sx, sy, place_seq);
            } else if(proto == ImageProtocol::ITerm2) {
                // iTerm2 inline image — include full image data each time.
                std::string iterm2_seq =
                    "\033]1337;File=inline=1;size=" +
                    std::to_string(_png_b64.size()) + ";width=" +
                    std::to_string(w) + ";height=" + std::to_string(h) + ":" +
                    _png_b64 + "\a";
                _ctx->write_after_flush(sx, sy, iterm2_seq);
            }
            return;
        }

        // --- Half-block fallback rendering ---
        int cell_h_pixels = h * 2;

        int render_w = w;
        int render_h_pixels = cell_h_pixels;
        int offset_x = 0;
        int offset_y_pixels = 0;

        if(_fit != ImageFit::Fill) {
            float img_aspect =
                static_cast<float>(_img_w) / static_cast<float>(_img_h);
            float cell_aspect =
                static_cast<float>(w) / static_cast<float>(cell_h_pixels);

            if(img_aspect > cell_aspect) {
                render_w = w;
                render_h_pixels = static_cast<int>(w / img_aspect);
            } else {
                render_h_pixels = cell_h_pixels;
                render_w = static_cast<int>(cell_h_pixels * img_aspect);
            }

            if(render_w > w)
                render_w = w;
            if(render_h_pixels > cell_h_pixels)
                render_h_pixels = cell_h_pixels;

            if(_fit == ImageFit::Contain) {
                float scale = std::min(
                    static_cast<float>(w) / _img_w,
                    static_cast<float>(cell_h_pixels) / _img_h
                );
                if(scale < 1.0f) {
                    render_w = static_cast<int>(_img_w * scale);
                    render_h_pixels = static_cast<int>(_img_h * scale);
                } else {
                    render_w = _img_w;
                    render_h_pixels = _img_h;
                }
            }

            offset_x = (w - render_w) / 2;
            offset_y_pixels = (cell_h_pixels - render_h_pixels) / 2;
        }

        for(int cy = 0; cy < h; cy++) {
            for(int cx = 0; cx < w; cx++) {
                int cell_px = cx;
                int cell_py_top = cy * 2;
                int cell_py_bot = cy * 2 + 1;

                int img_px = 0, img_py_top = 0, img_py_bot = 0;
                bool valid_top = true, valid_bot = true;

                if(_fit == ImageFit::Fill) {
                    img_px = cell_px * _img_w / w;
                    img_py_top = cell_py_top * _img_h / cell_h_pixels;
                    img_py_bot = cell_py_bot * _img_h / cell_h_pixels;
                } else {
                    int rx = cell_px - offset_x;
                    int ry_top = cell_py_top - offset_y_pixels;
                    int ry_bot = cell_py_bot - offset_y_pixels;

                    valid_top = rx >= 0 && rx < render_w && ry_top >= 0 &&
                                ry_top < render_h_pixels;
                    valid_bot = rx >= 0 && rx < render_w && ry_bot >= 0 &&
                                ry_bot < render_h_pixels;

                    if(valid_top) {
                        img_px = rx * _img_w / render_w;
                        img_py_top = ry_top * _img_h / render_h_pixels;
                    }
                    if(valid_bot) {
                        img_px = rx * _img_w / render_w;
                        img_py_bot = ry_bot * _img_h / render_h_pixels;
                    }
                }

                RGBA top = valid_top ? pixel_at(img_px, img_py_top)
                                     : RGBA::transparent();
                RGBA bot = valid_bot ? pixel_at(img_px, img_py_bot)
                                     : RGBA::transparent();

                if(top.is_transparent() && bot.is_transparent())
                    continue;

                if(top.approx_equal(bot)) {
                    buf.set_cell(
                        sx + cx, sy + cy, Cell{U' ', top, top}
                    );
                } else {
                    buf.set_cell(
                        sx + cx, sy + cy, Cell{U'\u2580', top, bot}
                    );
                }
            }
        }
    }

} // namespace stain
