#pragma once

#include "stain/event.hpp"

#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace stain {

    // A key combo to action string mapping.
    struct KeyBinding {
        std::string key;    // e.g. "ctrl+a", "shift+up", "return"
        std::string action; // e.g. "line-home", "move-up", "submit"
    };

    // Remap key names before lookup. e.g. { "enter" -> "return" }
    using KeyAliasMap = std::unordered_map<std::string, std::string>;

    // Merge user bindings on top of defaults. User bindings for the same key
    // override defaults.
    [[nodiscard]] std::vector<KeyBinding> merge_bindings(
        std::span<const KeyBinding> defaults,
        std::span<const KeyBinding> overrides
    );

    // Build a lookup map from merged bindings. Keys are canonical combos,
    // values are action strings.
    [[nodiscard]] std::unordered_map<std::string, std::string>
    build_binding_map(
        std::span<const KeyBinding> bindings,
        const KeyAliasMap& aliases = {}
    );

    // Produce the canonical lookup key from a KeyEvent.
    // e.g. "ctrl+shift+a", "meta+up", "return"
    [[nodiscard]] std::string key_binding_key(const KeyEvent& event);

} // namespace stain
