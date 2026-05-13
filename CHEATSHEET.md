# stain cheatsheet

## create renderer

```cpp
auto r = Renderer::create(RendererOptions{}.target_fps(30));
// or no FPS cap:
auto r = Renderer::create({});
r->run();
```

## widget tree

```cpp
auto& root = r->root();

// add boxes
auto& box = root.add<Box>();
auto& child = box.add<Box>();

// position & size
child.dimensions({
    .width  = {.type = DimensionType::Percent, .value = 50},
    .height = {.type = DimensionType::Px,      .value = 100},
});
// DimensionType: Px, Percent, Auto

// margin, padding, gap
child.margin({.left = 2, .right = 2, .top = 1, .bottom = 1});
child.padding({.left = 1, .top = 1});
child.gap(2);                      // both row & column
child.gap({.row = 4, .column = 2});

// border
box.border(BorderStyle::Rounded);  // None, Single, Double, Rounded, Heavy
box.border_side(Side::Left, BorderStyle::None);
// sides: Left, Right, Top, Bottom

// title
box.title("hello");
box.title_position(TitlePosition::Bottom);  // Top default

// colors
box.background(Color::hex("#1a1b26"));        // or .rgb(r, g, b)
box.focus_background(Color::named("blue"));    // when focused
```

## text

```cpp
// plain
auto& t = box.add<Text>("hello world");

// with style
auto& t = box.add<Text>(StyledText{}
    .append("bold ", Attr::Bold)
    .append("italic ", Attr::Italic)
    .append("underline", Attr::Underline));

// TextBuilder
auto& t = box.add<Text>(TextBuilder{}
    .span("red on blue", Color::hex("#ff0000"), Color::hex("#0000ff"))
    .span("dim with strike", Attr::Dim | Attr::Strike));

// wrapping
t.wrap(TextWrap::Word);  // Word, Char, None (default)

// hyperlink
t.href("https://example.com");

// click callback
t.on_click([] { /* xdg-open or whatever */ });
```

## input & textarea

```cpp
// single line
auto& inp = box.add<Input>();
inp.placeholder("type here");
inp.on_input([](std::string_view s) { /* each char */ });
inp.on_change([](std::string_view s) { /* value changed */ });
inp.on_enter([](std::string_view s) { /* enter pressed */ });

// multi line
auto& ta = box.add<Textarea>();
ta.on_change([](std::string_view s) { /* full text changed */ });
```

## select & tabselect

```cpp
// vertical
auto& sel = box.add<Select>();
sel.add("item 1");
sel.add("item 2");
sel.selected(0);
sel.on_select([](int index, std::string_view label) {
    // called on enter or click
});

// horizontal tabs
auto& tabs = box.add<TabSelect>();
tabs.add("tab a");
tabs.add("tab b");
tabs.selected(1);
tabs.on_select(...);  // same signature
```

## scrollbox

```cpp
auto& sb = box.add<ScrollBox>();
sb.add<Text>("scrolling content");
// keyboard: up/down, pageup/down, home/end
// scrollbars auto-show when content overflows
```

## framebuffer

```cpp
auto& fb = box.add<Framebuffer>();
fb.on_frame([](Framebuffer::Frame& frame, double dt, bool first) {
    for (int y = 0; y < frame.height; y++)
        for (int x = 0; x < frame.width; x++)
            frame.at(x, y) = {.c = '@', .fg = color_from_xy(x, y)};
});
```

## animation

```cpp
// per-frame update (no extra widget needed)
root.on_lifecycle_pass([](double dt, bool first, bool last) {
    my_thing.tick(dt);
});

// request continuous rendering
r->live(true);
```

## positioning

```cpp
box.position_type(PositionType::Absolute);
box.inset({.left = 10, .top = 5});

box.z_index(10);

box.opacity(0.5f);  // 0.0 to 1.0

box.overflow(Overflow::Scroll);  // Visible, Hidden, Scroll

// translate offsets render position without affecting layout
box.translate({.x = 2, .y = 1});
```

## focus

```cpp
// tab cycling
FocusCycler cycler;
cycler.add(box);
cycler.add(another);

r->on_key([&](const KeyEvent& e) -> bool {
    if (e.matches(Key::Tab))       return cycler.focus_next();
    if (e.matches(Key::ShiftTab))  return cycler.focus_prev();
    return false;  // let event propagate
});
```

## events

```cpp
// on a renderable
box.on_key([](const KeyEvent& e) -> bool {
    if (e.matches('q')) r->destroy();
    return e.matches(Key::Enter);  // true = consumed, stop propagation
});

box.on_mouse([](const MouseEvent& e) -> bool { ... });
box.on_focus([](bool focused) { ... });

// global on the renderer
r->on_key([](const KeyEvent& e) -> bool {
    if (e.is_ctrl() && e.codepoint == 'q') {
        r->destroy();
        return true;
    }
    return false;
});
```

## signals

```cpp
Emitter<KeyDown, MouseDown, Resized> emitter;

auto conn = emitter.on<KeyDown>([](const KeyEvent& e) {
    // handle
});

emitter.emit<KeyDown>(some_event);
emitter.erase(conn);
```

## custom key bindings

```cpp
auto bindings = merge_bindings(
    default_bindings(),
    {
        {Key::CtrlC, "quit"},
        {Key::CtrlS, "save"},
    }
);

auto map = build_binding_map(bindings);
```

## terminal

```cpp
// RAII guard — sets raw mode, alt screen, mouse, bracketed paste
TerminalGuard guard;

// query terminal capabilities
auto info = TerminalInfo::query();
bool rgb = info.true_color;
int cols = info.cols;
```

## colors

```cpp
auto c = Color::hex("#ff8800");
auto c = Color::rgb(255, 136, 0);
auto c = Color::named("tomato");           // CSS named colors
auto c = Color::from_hue(0.5f);           // full saturation, value=1
auto c = Color::from_hsl(0.5f, 0.8f, 0.6f);
auto c = Color::blend(fore, back, 0.3f);  // source-over alpha blend
auto c = Color::transparent();
```

## render attributes

```cpp
Attr::None
Attr::Bold
Attr::Dim
Attr::Italic
Attr::Underline
Attr::Blink
Attr::Reverse
Attr::Strike

// combine with bitwise OR:
Attr::Bold | Attr::Italic
```

## cell (raw access)

```cpp
Cell{
    .c = U'X',           // char32_t codepoint
    .fg = Color::white(),
    .bg = Color::black(),
    .attr = Attr::Bold,
    .link = "https://...",
};
```
