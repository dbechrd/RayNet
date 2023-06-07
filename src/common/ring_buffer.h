#pragma once

template <class T, size_t S>
struct RingBuffer {
    size_t nextIdx{};
    T data[S];

    constexpr size_t size() const {
        return S;
    }

    T &operator[](size_t index) {
        return data[(nextIdx + index) % S];
    }

    T &oldest() {
        return data[nextIdx];
    }

    T &newest() {
        return data[nextIdx ? (nextIdx - 1) : S - 1];
    }

    void push(T &value) {
        oldest() = value;
        nextIdx = (nextIdx + 1) % S;
    }
};
