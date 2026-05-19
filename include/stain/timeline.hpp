#pragma once

#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace stain {

namespace easing {

using EasingFn = float (*)(float);

inline float linear(float t) {
    return t;
}

inline float ease_in_quad(float t) {
    return t * t;
}
inline float ease_out_quad(float t) {
    return t * (2.0f - t);
}
inline float ease_in_out_quad(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

inline float ease_in_cubic(float t) {
    return t * t * t;
}
inline float ease_out_cubic(float t) {
    float v = 1.0f - t;
    return 1.0f - v * v * v;
}
inline float ease_in_out_cubic(float t) {
    return t < 0.5f ? 4.0f * t * t * t
                    : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

inline float ease_in_quart(float t) {
    return t * t * t * t;
}
inline float ease_out_quart(float t) {
    float v = 1.0f - t;
    return 1.0f - v * v * v * v;
}
inline float ease_in_out_quart(float t) {
    return t < 0.5f ? 8.0f * t * t * t * t
                    : 1.0f - std::pow(-2.0f * t + 2.0f, 4.0f) / 2.0f;
}

inline float ease_in_expo(float t) {
    return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * t - 10.0f);
}
inline float ease_out_expo(float t) {
    return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
}
inline float ease_in_out_expo(float t) {
    return t == 0.0f ? 0.0f
         : t == 1.0f ? 1.0f
         : t < 0.5f  ? std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f
                     : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;
}

inline float ease_in_back(float t) {
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    return c3 * t * t * t - c1 * t * t;
}
inline float ease_out_back(float t) {
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    float v = t - 1.0f;
    return 1.0f + c3 * v * v * v + c1 * v * v;
}
inline float ease_in_out_back(float t) {
    constexpr float c1 = 1.70158f;
    constexpr float c2 = c1 * 1.525f;
    return t < 0.5f
        ? (std::pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f
        : (std::pow(2.0f * t - 2.0f, 2.0f) *
                       ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) +
                   2.0f) /
                   2.0f;
}

inline float ease_in_elastic(float t) {
    if(t == 0.0f || t == 1.0f)
        return t;
    constexpr float c4 = 2.0f * 3.14159265f / 3.0f;
    return -std::pow(2.0f, 10.0f * t - 10.0f) *
           std::sin((t * 10.0f - 10.75f) * c4);
}
inline float ease_out_elastic(float t) {
    if(t == 0.0f || t == 1.0f)
        return t;
    constexpr float c4 = 2.0f * 3.14159265f / 3.0f;
    return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) +
           1.0f;
}
inline float ease_in_out_elastic(float t) {
    if(t == 0.0f || t == 1.0f)
        return t;
    constexpr float c5 = 2.0f * 3.14159265f / 4.5f;
    return t < 0.5f
        ? -(std::pow(2.0f, 20.0f * t - 10.0f) *
            std::sin((20.0f * t - 11.125f) * c5)) /
               2.0f
        : (std::pow(2.0f, -20.0f * t + 10.0f) *
                   std::sin((20.0f * t - 11.125f) * c5)) /
                   2.0f +
               1.0f;
}

inline float ease_out_bounce(float t) {
    constexpr float n1 = 7.5625f;
    constexpr float d1 = 2.75f;
    if(t < 1.0f / d1) {
        return n1 * t * t;
    } else if(t < 2.0f / d1) {
        t -= 1.5f / d1;
        return n1 * t * t + 0.75f;
    } else if(t < 2.5f / d1) {
        t -= 2.25f / d1;
        return n1 * t * t + 0.9375f;
    } else {
        t -= 2.625f / d1;
        return n1 * t * t + 0.984375f;
    }
}
inline float ease_in_bounce(float t) {
    return 1.0f - ease_out_bounce(1.0f - t);
}
inline float ease_in_out_bounce(float t) {
    return t < 0.5f
        ? (1.0f - ease_out_bounce(1.0f - 2.0f * t)) / 2.0f
        : (1.0f + ease_out_bounce(2.0f * t - 1.0f)) / 2.0f;
}

} // namespace easing

struct AnimProperty {
    float to{};
    std::function<float(float)> ease{easing::linear};
};

class Animation {
    public:
    [[nodiscard]] float get(std::string_view key) const;
    [[nodiscard]] const std::unordered_map<std::string, float>&
    targets() const {
        return _current;
    }
    [[nodiscard]] float progress() const {
        return _progress;
    }
    [[nodiscard]] bool done() const {
        return _done;
    }

    Animation& on_update(std::function<void(Animation&)> cb);
    Animation& on_complete(std::function<void()> cb);

    private:
    friend class Timeline;

    std::unordered_map<std::string, float> _initial;
    std::unordered_map<std::string, AnimProperty> _properties;
    std::unordered_map<std::string, float> _current;
    std::unordered_map<std::string, float> _delta;

    float _delay_s{0};
    float _duration_s{0.3f};
    float _elapsed{0};
    float _progress{0};
    bool _done{false};
    bool _completed_fired{false};

    std::function<void(Animation&)> _on_update;
    std::function<void()> _on_complete;

    void tick(double dt_s);
    void reset();
};

class Timeline : public std::enable_shared_from_this<Timeline> {
    public:
    Timeline() = default;

    Timeline& duration(float ms);
    Timeline& loop(bool v);
    Timeline& on_complete(std::function<void()> cb);
    Timeline& on_pause(std::function<void()> cb);

    Animation& animate(
        std::initializer_list<std::pair<std::string, float>> initial,
        std::initializer_list<std::pair<std::string, AnimProperty>> properties,
        float duration_ms = 300,
        float delay_ms = 0
    );

    void play();
    void pause();
    void restart();

    [[nodiscard]] float progress() const {
        return _progress;
    }
    [[nodiscard]] float duration() const {
        return _duration_ms;
    }
    [[nodiscard]] bool paused() const {
        return _paused;
    }
    [[nodiscard]] bool done() const {
        return _done;
    }

    [[nodiscard]] float effective_duration() const;

    void update(double dt_s);

    private:
    float _duration_ms{0};
    float _elapsed{0};
    float _progress{0};
    float _auto_duration{0};
    bool _loop{false};
    bool _paused{false};
    bool _done{false};

    std::vector<std::unique_ptr<Animation>> _animations;
    std::function<void()> _on_complete;
    std::function<void()> _on_pause;
};

} // namespace stain
