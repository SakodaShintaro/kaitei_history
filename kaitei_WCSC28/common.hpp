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

inline auto PEXT64(unsigned long long a, unsigned long long b) {
    return _pext_u64(a, b);
}

inline auto POP_CNT64(uint64_t bits) {
#ifdef _MSC_VER
    return __popcnt64(bits);
#elif __GNUC__
    return __builtin_popcountll (bits);
#endif
}

inline int pop_lsb(uint64_t& b) {
    unsigned long index;

#ifdef _MSC_VER
    _BitScanForward64(&index, b);
#elif __GNUC__
    GNUBitScanForward(&index, b);
#endif

    b = _blsr_u64(b);
    return index;
}

template<class Type>
static double sigmoid(Type x, double gain) {
	return 1 / (1 + exp(-gain * x));
}

template<class Type>
static double standardSigmoid(Type x) {
	return sigmoid(x, 1.0);
}

template<class Type>
static double d_sigmoid(Type x, double gain) {
	return gain * sigmoid(x, gain) * (1 - sigmoid(x, gain));
}

template<class Type>
static double d_standardSigmoid(Type x) {
	return d_sigmoid(x, 1.0);
}

#endif