#include "stain/keybind.hpp"

namespace stain {

    std::vector<KeyBinding> merge_bindings(
        std::span<const KeyBinding> defaults,
        std::span<const KeyBinding> overrides
    ) {
        // Start with defaults, then override matching keys.
        std::unordered_map<std::string, std::string> merged;
        std::vector<std::string> order;

        for(const auto& b : defaults) {
            if(merged.find(b.key) == merged.end()) {
                order.push_back(b.key);
            }
            merged[b.key] = b.action;
        }

        for(const auto& b : overrides) {
            if(merged.find(b.key) == merged.end()) {
                order.push_back(b.key);
            }
            merged[b.key] = b.action;
        }

        std::vector<KeyBinding> result;
        result.reserve(order.size());
        for(const auto& key : order) {
            result.push_back({key, merged[key]});
        }
        return result;
    }

    std::unordered_map<std::string, std::string> build_binding_map(
        std::span<const KeyBinding> bindings,
        const KeyAliasMap& aliases
    ) {
        std::unordered_map<std::string, std::string> map;
        for(const auto& b : bindings) {
            std::string key = b.key;
            // Apply aliases to the key.
            auto it = aliases.find(key);
            if(it != aliases.end()) {
                key = it->second;
            }
            map[key] = b.action;
        }
        return map;
    }

    std::string key_binding_key(const KeyEvent& event) {
        // Build canonical key string: ctrl+meta+shift+super+name
        std::string result;

        if(event.ctrl) {
            result += "ctrl+";
        }
        if(event.meta) {
            result += "meta+";
        }
        if(event.shift) {
            result += "shift+";
        }
        if(event.super_) {
            result += "super+";
        }

        result += event.name;
        return result;
    }

} // namespace stain
