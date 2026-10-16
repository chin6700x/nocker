/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 *
 * ⚡ nocker: Multi-Language Parity Benchmark - C++20 Engine
 */

#include <iostream>
#include <vector>
#include <numeric>
#include <concepts>

template <typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template <Numeric T>
class Accumulator {
public:
    constexpr explicit Accumulator(T initial = 0) : total_(initial) {}

    constexpr void add(T val) noexcept {
        total_ += val;
    }

    [[nodiscard]] constexpr T value() const noexcept {
        return total_;
    }

private:
    T total_{};
};

int main() {
    Accumulator<int64_t> acc(100);
    std::vector<int64_t> numbers = { 10, 20, 30, 40, 50 };

    for (const auto n : numbers) {
        acc.add(n);
    }

    std::cout << "⚡ [CPP-Runner] Accumulated Value = " << acc.value() << std::endl;
    return 0;
}
