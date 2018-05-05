#pragma once

#ifndef TYPES_HPP
#define TYPES_HPP

#include<iostream>

enum Color {
    BLACK, WHITE, ColorNum,
};

enum Bound {
    EXACT_BOUND, UPPER_BOUND, LOWER_BOUND
};

enum Depth {
    PLY = 128,
    DEPTH_MAX = 64 * PLY,
    MATE_DEPTH_MAX = 128,
};

enum Score {
    MAX_SCORE = 1000000,
    SCORE_ZERO = 0,
    DRAW_SCORE = 0,
    MIN_SCORE = -MAX_SCORE,
    MATE_SCORE_LOWER_BOUND = MAX_SCORE - static_cast<int>(MATE_DEPTH_MAX),
    MATE_SCORE_UPPER_BOUND = MIN_SCORE + static_cast<int>(MATE_DEPTH_MAX),
};

inline bool isMatedScore(const Score score) {
    return score <= MATE_SCORE_UPPER_BOUND || MATE_SCORE_LOWER_BOUND <= score;
}

//演算子をオーバーロードしておかないと不便
//Score
constexpr Score operator-(Score lhs) { return Score(-int(lhs)); }
constexpr Score operator+(Score lhs, Score rhs) { return Score(int(lhs) + int(rhs)); }
constexpr Score operator-(Score lhs, Score rhs) { return Score(int(lhs) - int(rhs)); }
constexpr Score operator+(Score lhs, int rhs) { return Score(int(lhs) + rhs); }
constexpr Score operator-(Score lhs, int rhs) { return Score(int(lhs) - rhs); }
constexpr Score operator+(int lhs, Score rhs) { return Score(lhs + int(rhs)); }
constexpr Score operator-(int lhs, Score rhs) { return Score(lhs - int(rhs)); }
inline Score& operator+=(Score& lhs, Score rhs) { return lhs = lhs + rhs; }
inline Score& operator-=(Score& lhs, Score rhs) { lhs = lhs - rhs;  return lhs; }
inline Score& operator+=(Score& lhs, int rhs) { lhs = lhs + rhs;  return lhs; }
inline Score& operator-=(Score& lhs, int rhs) { lhs = lhs - rhs;  return lhs; }
inline Score& operator++(Score& lhs) { lhs = lhs + 1;  return lhs; }
inline Score& operator--(Score& lhs) { lhs = lhs - 1;  return lhs; }
inline Score operator++(Score& lhs, int) { Score t = lhs; lhs += 1;  return t; }
inline Score operator--(Score& lhs, int) { Score t = lhs; lhs -= 1;  return t; }

constexpr Score operator*(Score lhs, int rhs) { return Score(int(lhs) * rhs); }
constexpr Score operator*(int lhs, Score rhs) { return Score(lhs * int(rhs)); }
constexpr Score operator/(Score lhs, int rhs) { return Score(int(lhs) / rhs); }
inline Score& operator*=(Score& lhs, int rhs) { lhs = lhs * rhs;  return lhs; }
inline Score& operator/=(Score& lhs, int rhs) { lhs = lhs / rhs;  return lhs; }
std::ostream& operator<<(std::ostream& os, const Score s);

//Depth
constexpr Depth operator-(Depth lhs) { return Depth(-int(lhs)); }
constexpr Depth operator+(Depth lhs, Depth rhs) { return Depth(int(lhs) + int(rhs)); }
constexpr Depth operator-(Depth lhs, Depth rhs) { return Depth(int(lhs) - int(rhs)); }
constexpr Depth operator+(Depth lhs, int rhs) { return Depth(int(lhs) + rhs); }
constexpr Depth operator-(Depth lhs, int rhs) { return Depth(int(lhs) - rhs); }
constexpr Depth operator+(int lhs, Depth rhs) { return Depth(lhs + int(rhs)); }
constexpr Depth operator-(int lhs, Depth rhs) { return Depth(lhs - int(rhs)); }
inline Depth& operator+=(Depth& lhs, Depth rhs) { return lhs = lhs + rhs; }
inline Depth& operator-=(Depth& lhs, Depth rhs) { lhs = lhs - rhs;  return lhs; }
inline Depth& operator+=(Depth& lhs, int rhs) { lhs = lhs + rhs;  return lhs; }
inline Depth& operator-=(Depth& lhs, int rhs) { lhs = lhs - rhs;  return lhs; }
inline Depth& operator++(Depth& lhs) { lhs = lhs + 1;  return lhs; }
inline Depth& operator--(Depth& lhs) { lhs = lhs - 1;  return lhs; }
inline Depth operator++(Depth& lhs, int) { Depth t = lhs; lhs += 1;  return t; }
inline Depth operator--(Depth& lhs, int) { Depth t = lhs; lhs -= 1;  return t; }

constexpr Depth operator*(Depth lhs, int rhs) { return Depth(int(lhs) * rhs); }
constexpr Depth operator*(int lhs, Depth rhs) { return Depth(lhs * int(rhs)); }
constexpr Depth operator/(Depth lhs, int rhs) { return Depth(int(lhs) / rhs); }
inline Depth& operator*=(Depth& lhs, int rhs) { lhs = lhs * rhs;  return lhs; }
inline Depth& operator/=(Depth& lhs, int rhs) { lhs = lhs / rhs;  return lhs; }
std::ostream& operator<<(std::ostream& os, const Depth d);
std::istream& operator>>(std::istream& is, Depth& d);

#endif // !TYPES_HPP