#include "stain/signal.hpp"
#include <gtest/gtest.h>

using namespace stain;

TEST(SignalTest, connect_and_emit) {
    Signal<int> sig;
    int value = 0;
    sig.connect(std::function<bool(int)>([&](int v) {
        value = v;
        return true;
    }));
    sig.emit(42);
    EXPECT_EQ(value, 42);
}

TEST(SignalTest, disconnect) {
    Signal<int> sig;
    int count = 0;
    auto id = sig.connect(std::function<bool(int)>([&](int) {
        count++;
        return true;
    }));
    sig.emit(1);
    sig.disconnect(id);
    sig.emit(2);
    EXPECT_EQ(count, 1);
}

TEST(SignalTest, stops_on_false) {
    Signal<int> sig;
    int count = 0;
    sig.connect(std::function<bool(int)>([&](int) {
        count++;
        return true;
    }));
    sig.connect(std::function<bool(int)>([&](int) {
        count++;
        return false;
    }));
    sig.connect(std::function<bool(int)>([&](int) {
        count++;
        return true;
    }));
    sig.emit(1);
    EXPECT_EQ(count, 2);
}

TEST(SignalTest, void_handler_always_continues) {
    Signal<int> sig;
    int count = 0;
    sig.connect(std::function<void(int)>([&](int) { count++; }));
    sig.connect(std::function<bool(int)>([&](int) {
        count++;
        return true;
    }));
    sig.emit(42);
    EXPECT_EQ(count, 2);
}

TEST(SignalTest, clear) {
    Signal<int> sig;
    int count = 0;
    sig.connect(std::function<bool(int)>([&](int) {
        count++;
        return true;
    }));
    sig.clear();
    sig.emit(42);
    EXPECT_EQ(count, 0);
}

TEST(SignalTest, empty) {
    Signal<int> sig;
    EXPECT_TRUE(sig.empty());
    sig.connect(std::function<bool(int)>([](int) { return true; }));
    EXPECT_FALSE(sig.empty());
    sig.clear();
    EXPECT_TRUE(sig.empty());
}

TEST(SignalTest, multiple_args) {
    Signal<int, std::string> sig;
    std::string result;
    sig.connect(
        std::function<bool(int, std::string)>([&](int n, std::string s) {
            result = s + std::to_string(n);
            return true;
        })
    );
    sig.emit(42, "answer:");
    EXPECT_EQ(result, "answer:42");
}

TEST(EmitterTest, on_and_emit) {
    struct MyEvent {
        int x;
    };
    struct OtherEvent {
        std::string s;
    };
    Emitter<MyEvent, OtherEvent> emitter;

    int value = 0;
    emitter.on<MyEvent>([&](MyEvent e) { value = e.x; });

    std::string str;
    emitter.on<OtherEvent>([&](OtherEvent e) { str = e.s; });

    emitter.emit(MyEvent{99});
    EXPECT_EQ(value, 99);
    EXPECT_TRUE(str.empty());

    emitter.emit(OtherEvent{"hello"});
    EXPECT_EQ(str, "hello");
}

TEST(EmitterTest, off) {
    struct E {};
    Emitter<E> emitter;
    int count = 0;
    auto id = emitter.on<E>([&](E) { count++; });
    emitter.emit(E{});
    emitter.off<E>(id);
    emitter.emit(E{});
    EXPECT_EQ(count, 1);
}

TEST(EmitterTest, clear_all) {
    struct A {};
    struct B {};
    Emitter<A, B> emitter;
    int count = 0;
    emitter.on<A>([&](A) { count++; });
    emitter.on<B>([&](B) { count++; });
    emitter.clear_all();
    emitter.emit(A{});
    emitter.emit(B{});
    EXPECT_EQ(count, 0);
}

TEST(EmitterTest, clear_specific) {
    struct A {};
    struct B {};
    Emitter<A, B> emitter;
    int count = 0;
    emitter.on<A>([&](A) { count++; });
    emitter.on<B>([&](B) { count++; });
    emitter.clear<A>();
    emitter.emit(A{});
    emitter.emit(B{});
    EXPECT_EQ(count, 1);
}
