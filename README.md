# stain

Your modern C++20 terminal UI framework.

[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-3.25+-064F8C?logo=cmake)](https://cmake.org)
[![License](https://img.shields.io/badge/license-LGPL-blue)]()

## Requirements

- C++20 compiler (Clang 14+ or GCC 11+)
- CMake 3.25+
- [yoga](https://github.com/facebook/yoga)
- [tl-expected](https://github.com/TartanLlama/expected)

Tests additionally require [GTest](https://github.com/google/googletest).

## Build

```sh
cmake -B build
cmake --build build
```

The library builds as a static target (`libstain.a`). Headers are in `include/stain/`, sources in `src/`.

Build only the library (skip examples and tests):

```sh
cmake -B build -DSTAIN_BUILD_EXAMPLES=OFF -DSTAIN_BUILD_TESTS=OFF
cmake --build build
```

Install with `cmake --install build`. Installs `libstain.a`, headers, and CMake package config. Once installed, use from another project with:

```cmake
find_package(stain REQUIRED)
target_link_libraries(myapp PRIVATE stain::stain)
```

To install to a custom prefix: `cmake --install build --prefix /custom/path`

## Quickstart

```cpp
#include <stain/renderer.hpp>
#include <stain/box.hpp>
#include <stain/text.hpp>
#include <stain/input.hpp>
#include <stain/select.hpp>

using namespace stain;

int main() {
    auto r = Renderer::create(RendererOptions{}.target_fps(30));
    auto& root = r->root();

    auto& box = root.add<Box>()
        .dimensions({.width = {.value = 600}, .height = {.value = 300}})
        .border(BorderStyle::Rounded)
        .title("stain demo");

    box.add<Text>("hello, terminal");

    auto& inp = box.add<Input>();
    inp.placeholder("type here");
    inp.on_enter([&](std::string_view v) {
        box.title(v);
    });

    r->run();
}
```

Three example programs in `examples/`:

| target        | description                                                 |
| ------------- | ----------------------------------------------------------- |
| `demo`        | All widgets wired together with focus cycling               |
| `framebuffer` | Per-cell plasma effect using `Framebuffer::on_frame`        |
| `animation`   | Bouncing ball, particle system, color fill with hue cycling |

A exahustive cheatsheet is found at `CHEATSHEET.md` at the root of this project. You may also find useful the documentation contained in header files!

More examples and actual documentation coming soon!

## License

LGPL 3.0 or later. See LICENSE.
