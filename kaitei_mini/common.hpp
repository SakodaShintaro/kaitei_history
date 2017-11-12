#ifndef COMMON_HPP
#define COMMON_HPP

#include<cmath>
#include<cassert>
#include<cstdint>
#ifdef _MSC_VER
#include<intrin.h>
#elif __GNUC__
#include<x86intrin.h>
#include<bmi2intrin.h>
#endif

#ifdef __GNUC__
//cf : http://blog.jiubao.org/2015/01/gcc-bitscanforward-bitscanreverse-msvc.html
unsigned char inline GNUBitScanForward(unsigned long *Index, unsigned long long Mask) {
    if (Mask) {
        *Index = __builtin_ctzll(Mask);
        return 1;
    } else {
        /* 戻り値が0のとき、*Index がどうなるかは未定義。*/
        return 0;
    }
}

unsigned char inline GNUBitScanReverse(unsigned long *Index, unsigned long long Mask) {
    if (Mask) {
        *Index = 63 - __builtin_clzll(Mask);
        return 1;
    } else {
        /* 戻り値が0のとき、*Index がどうなるかは未定義。*/
        return 0;
    }
}
#endif

inline int MSB64(unsigned long long v) {
    assert(v != 0);
    unsigned long index;

#ifdef _MSC_VER
    _BitScanReverse64(&index, v);
#elif __GNUC__
    GNUBitScanReverse(&index, v);
#endif

    return index;
}

inline auto POP_CNT(int bits) {
#if _MSC_VER
    return __popcnt(bits);
#elif __GNUC__
    return __builtin_popcount(bits);
#endif
}
#endif