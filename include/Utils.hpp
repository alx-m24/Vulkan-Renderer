#pragma once

#include <ranges>

template<typename A, typename B>
static bool isAllPresent(const std::vector<A>& a, const std::vector<B>& b) {
    return std::ranges::all_of(a,
            [&b] (const A& _a) {
                return std::ranges::any_of(b,
                        [&_a] (const B& _b) {
                            return _a == _b;
                        });
                });
}

#ifndef NDEBUG
#define DEBUG_PRINT(x) do { std::cout << (x) << std::endl; } while(0)
#else
#define DEBUG_PRINT(x) do { /*DO NOTHING*/ } while(0)
#endif
