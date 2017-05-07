#ifndef COMMON_HPP
#define COMMON_HPP

// 立っているビットの数を返す
static int highestOneBit(int i)
{
	i |= (i >> 1);
	i |= (i >> 2);
	i |= (i >> 4);
	i |= (i >> 8);
	i |= (i >> 16);
	return i - (i >> 1);
}
#endif