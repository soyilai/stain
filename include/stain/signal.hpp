#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>

#include <type_traits>
#include <vector>

namespace stain {

    // A single-event signal. Listeners are std::function<bool(Args...)>.
    // Returning false stops propagation.
    template <typename... Args> class Signal {
        public:
        using Handler = std::function<bool(Args...)>;

        // Connect a handler. Returns a connection ID for disconnect.
        [[nodiscard]] std::size_t connect(Handler h) {
            auto id = _next_id++;
            _handlers.emplace_back(id, std::move(h));
            return id;
        }

        // Connect a void handler. Always returns true (continues propagation).
        [[nodiscard]] std::size_t connect(std::function<void(Args...)> h) {
            return connect(Handler{[h = std::move(h)](Args... args) -> bool {
                h(std::forward<Args>(args)...);
                return true;
            }});
        }

        // Disconnect a handler by its connection ID.
        void disconnect(std::size_t id) {
            _handlers.erase(
                std::remove_if(
                    _handlers.begin(),
                    _handlers.end(),
                    [id](const auto& p) { return p.first == id; }
                ),
                _handlers.end()
            );
        }

        // Emit the signal. Calls each handler in order; stops if one returns
        // false.
        void emit(Args... args) const {
            auto handlers = _handlers;
            for(const auto& [id, handler] : handlers) {
                if(!handler(args...)) {
                    break;
                }
            }
        }

        // Remove all handlers.
        void clear() {
            _handlers.clear();
        }

        [[nodiscard]] bool empty() const noexcept {
            return _handlers.empty();
        }

        private:
        std::vector<std::pair<std::size_t, Handler>> _handlers;
        std::size_t _next_id{1};
    };

    namespace detail {

        template <typename Tag> struct SignalHolder {
            Signal<Tag> signal;
        };

    } // namespace detail

    // Typed event bus, owns one Signal per event tag.
    //
    // Usage:
    //   struct Focused{};
    //   struct Blurred{};
    //   struct Resized{ int w, h; };
    //   Emitter<Focused, Blurred, Resized> emitter;
    //   auto id = emitter.on<Resized>([](Resized e){ ... });
    //   emitter.emit(Resized{80, 24});
    //   emitter.off<Resized>(id);
    template <typename... EventTags>
    class Emitter : private detail::SignalHolder<EventTags>... {
        public:
        // Subscribe to an event tag. Returns a connection ID.
        template <typename Tag>
        [[nodiscard]] std::size_t on(std::function<void(Tag)> handler) {
            static_assert(
                (std::is_same_v<Tag, EventTags> || ...),
                "Tag is not a registered event type for this Emitter"
            );
            return get_signal<Tag>().connect(std::move(handler));
        }

        // Unsubscribe by connection ID.
        template <typename Tag> void off(std::size_t id) {
            static_assert(
                (std::is_same_v<Tag, EventTags> || ...),
                "Tag is not a registered event type for this Emitter"
            );
            get_signal<Tag>().disconnect(id);
        }

        // Emit an event.
        template <typename Tag> void emit(Tag event) {
            static_assert(
                (std::is_same_v<Tag, EventTags> || ...),
                "Tag is not a registered event type for this Emitter"
            );
            get_signal<Tag>().emit(std::move(event));
        }

        // Clear all handlers for a specific event.
        template <typename Tag> void clear() {
            static_assert(
                (std::is_same_v<Tag, EventTags> || ...),
                "Tag is not a registered event type for this Emitter"
            );
            get_signal<Tag>().clear();
        }

        // Clear all handlers for all events.
        void clear_all() {
            (get_signal<EventTags>().clear(), ...);
        }

        private:
        template <typename Tag> Signal<Tag>& get_signal() {
            return static_cast<detail::SignalHolder<Tag>&>(*this).signal;
        }

        template <typename Tag> const Signal<Tag>& get_signal() const {
            return static_cast<const detail::SignalHolder<Tag>&>(*this).signal;
        }
    };

} // namespace stain
