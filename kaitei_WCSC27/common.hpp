#ifndef COMMON_HPP
#define COMMON_HPP

#ifdef _MSC_VER
#include<intrin.h>
#endif

// 立っているビットの数を返す
static int highestOneBit(int i) {
    i |= (i >> 1);
    i |= (i >> 2);
    i |= (i >> 4);
    i |= (i >> 8);
    i |= (i >> 16);
    return i - (i >> 1);
}

inline auto POP_CNT(int bits) {
#ifdef _MSC_VER
    return __popcnt(bits);
#elif __GNUC__
    return __builtin_popcount(bits);
#endif
}

#endif