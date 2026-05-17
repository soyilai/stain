#include "stain/timeline.hpp"

#include <algorithm>
#include <cassert>

namespace stain {

// Animation

float Animation::get(std::string_view key) const {
    auto it = _current.find(std::string(key));
    return it != _current.end() ? it->second : 0.0f;
}

Animation& Animation::on_update(std::function<void(Animation&)> cb) {
    _on_update = std::move(cb);
    return *this;
}

Animation& Animation::on_complete(std::function<void()> cb) {
    _on_complete = std::move(cb);
    return *this;
}

void Animation::tick(double dt_s) {
    if(_done)
        return;

    float dt = static_cast<float>(dt_s);
    _elapsed += dt;

    if(_elapsed < _delay_s) {
        _progress = 0.0f;
        _current = _initial;
        if(_on_update)
            _on_update(*this);
        return;
    }

    float local_t = _elapsed - _delay_s;
    _progress = std::min(local_t / _duration_s, 1.0f);

    for(auto& [key, prop] : _properties) {
        float t = prop.ease(_progress);
        _current[key] = _initial.at(key) + _delta.at(key) * t;
    }

    if(_on_update)
        _on_update(*this);

    if(_progress >= 1.0f && !_completed_fired) {
        _done = true;
        _completed_fired = true;
        if(_on_complete)
            _on_complete();
    }
}

void Animation::reset() {
    _elapsed = 0;
    _progress = 0;
    _done = false;
    _completed_fired = false;
    _current = _initial;
}

// Timeline

Timeline& Timeline::duration(float ms) {
    _duration_ms = ms;
    return *this;
}

Timeline& Timeline::loop(bool v) {
    _loop = v;
    return *this;
}

Timeline& Timeline::on_complete(std::function<void()> cb) {
    _on_complete = std::move(cb);
    return *this;
}

Timeline& Timeline::on_pause(std::function<void()> cb) {
    _on_pause = std::move(cb);
    return *this;
}

Animation& Timeline::animate(
    std::initializer_list<std::pair<std::string, float>> initial,
    std::initializer_list<std::pair<std::string, AnimProperty>> properties,
    float duration_ms,
    float delay_ms
) {
    auto anim = std::make_unique<Animation>();

    for(auto& [key, val] : initial) {
        anim->_initial[key] = val;
        anim->_current[key] = val;
    }

    for(auto& [key, prop] : properties) {
        anim->_properties[key] = prop;
        float init_val = anim->_initial.count(key) ? anim->_initial.at(key) : 0.0f;
        anim->_delta[key] = prop.to - init_val;
    }

    anim->_duration_s = duration_ms / 1000.0f;
    anim->_delay_s = delay_ms / 1000.0f;
    if(anim->_duration_s < 0.001f)
        anim->_duration_s = 0.001f;

    float end_time = duration_ms + delay_ms;
    if(_duration_ms == 0.0f) {
        _auto_duration = std::max(_auto_duration, end_time);
    }

    _animations.push_back(std::move(anim));
    return *_animations.back();
}

void Timeline::play() {
    if(!_paused)
        return;
    _paused = false;
}

void Timeline::pause() {
    if(_paused)
        return;
    _paused = true;
    if(_on_pause)
        _on_pause();
}

void Timeline::restart() {
    _elapsed = 0;
    _progress = 0;
    _done = false;
    _paused = false;
    for(auto& anim : _animations) {
        anim->reset();
    }
}

void Timeline::update(double dt_s) {
    if(_paused || _done)
        return;

    float dt = static_cast<float>(dt_s);
    _elapsed += dt;
    float dur = effective_duration();

    if(_elapsed >= dur && dur > 0.0f) {
        if(_loop) {
            _elapsed = 0.0f;
            for(auto& anim : _animations) {
                anim->reset();
            }
            if(_on_complete)
                _on_complete();
        } else {
            _done = true;
        }
    }

    _progress = dur > 0.0f ? std::min(_elapsed / dur, 1.0f) : 0.0f;

    for(auto& anim : _animations) {
        anim->tick(dt_s);
    }
}

float Timeline::effective_duration() const {
    return _duration_ms > 0.0f ? _duration_ms : _auto_duration;
}

} // namespace stain
