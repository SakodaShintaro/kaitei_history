#pragma once

#ifndef PIECE_HPP
#define PIECE_HPP

#include<unordered_map>
#include<string>
#include<vector>
#include<iostream>
#include<fstream>
#include<cassert>
#include"types.hpp"
#define PROMOTE_BIT 5
#define BLACK_BIT 6
#define WHITE_BIT 7
#define WALL_BIT 8

enum Piece{
    EMPTY  = 0,                                         //0000 0000
    PAWN   = 1,                                         //0000 0001
    LANCE  = 2,                                         //0000 0010
    KNIGHT = 3,                                         //0000 0011
    SILVER = 4,                                         //0000 0100
    GOLD   = 5,                                         //0000 0101
    BISHOP = 6,                                         //0000 0110
    ROOK   = 7,                                         //0000 0111
    KING   = 8,                                         //0000 1000
	PIECE_KIND_MASK = 15,                               //0000 1111
    PROMOTE = 1 << (PROMOTE_BIT - 1),                   //0001 0000  16
    PAWN_PROMOTE   = PAWN + PROMOTE,                    //0001 0001  17
    LANCE_PROMOTE  = LANCE + PROMOTE,                   //0001 0010  18
    KNIGHT_PROMOTE = KNIGHT + PROMOTE,                  //0001 0011  19
    SILVER_PROMOTE = SILVER + PROMOTE,                  //0001 0100  20
    BISHOP_PROMOTE = BISHOP + PROMOTE,                  //0001 0110  22
    ROOK_PROMOTE   = ROOK + PROMOTE,                    //0001 0111  23
    BLACK_FLAG = 1 << (BLACK_BIT - 1),                  //0010 0000  32
    BLACK_PAWN   = PAWN   + BLACK_FLAG,                 //0010 0001  33
    BLACK_LANCE  = LANCE  + BLACK_FLAG,                 //0010 0010  34
    BLACK_KNIGHT = KNIGHT + BLACK_FLAG,                 //0010 0011  35
    BLACK_SILVER = SILVER + BLACK_FLAG,                 //0010 0100  36
    BLACK_GOLD   = GOLD   + BLACK_FLAG,                 //0010 0101  37
    BLACK_BISHOP = BISHOP + BLACK_FLAG,                 //0010 0110  38
    BLACK_ROOK   = ROOK   + BLACK_FLAG,                 //0010 0111  39
    BLACK_KING   = KING   + BLACK_FLAG,                 //0010 1000  40
    BLACK_PAWN_PROMOTE   = PAWN_PROMOTE   + BLACK_FLAG, //0011 0001  49
    BLACK_LANCE_PROMOTE  = LANCE_PROMOTE  + BLACK_FLAG, //0011 0010  50
    BLACK_KNIGHT_PROMOTE = KNIGHT_PROMOTE + BLACK_FLAG, //0011 0011  51
    BLACK_SILVER_PROMOTE = SILVER_PROMOTE + BLACK_FLAG, //0011 0100  52
    BLACK_BISHOP_PROMOTE = BISHOP_PROMOTE + BLACK_FLAG, //0011 0110  54
    BLACK_ROOK_PROMOTE   = ROOK_PROMOTE   + BLACK_FLAG, //0011 0111  55
    WHITE_FLAG = 1 << (WHITE_BIT - 1),                  //0100 0000  64
    WHITE_PAWN   = PAWN   + WHITE_FLAG,                 //0100 0001  65
    WHITE_LANCE  = LANCE  + WHITE_FLAG,                 //0100 0010  66
    WHITE_KNIGHT = KNIGHT + WHITE_FLAG,                 //0100 0011  67
    WHITE_SILVER = SILVER + WHITE_FLAG,                 //0100 0100  68
    WHITE_GOLD   = GOLD   + WHITE_FLAG,                 //0100 0101  69
    WHITE_BISHOP = BISHOP + WHITE_FLAG,                 //0100 0110  70
    WHITE_ROOK   = ROOK   + WHITE_FLAG,                 //0100 0111  71
    WHITE_KING   = KING   + WHITE_FLAG,                 //0100 1000  72
    WHITE_PAWN_PROMOTE   = PAWN_PROMOTE   + WHITE_FLAG, //0101 0001  81
    WHITE_LANCE_PROMOTE  = LANCE_PROMOTE  + WHITE_FLAG, //0101 0010  82
    WHITE_KNIGHT_PROMOTE = KNIGHT_PROMOTE + WHITE_FLAG, //0101 0011  83
    WHITE_SILVER_PROMOTE = SILVER_PROMOTE + WHITE_FLAG, //0101 0100  84
    WHITE_BISHOP_PROMOTE = BISHOP_PROMOTE + WHITE_FLAG, //0101 0110  86
    WHITE_ROOK_PROMOTE   = ROOK_PROMOTE   + WHITE_FLAG, //0101 0111  87
	PieceNum,
    WALL = 1 << (WALL_BIT),                             //1000 0000
};

extern std::unordered_map<int, std::string> PieceToStr;
extern std::unordered_map<int, std::string> PieceToSfenStr;

inline Piece operator++(Piece &p, int) { return p = static_cast<Piece>(p + 1); }

inline int operator<<(Piece p, int shift) { return static_cast<int>(p) << shift; }

static Piece ColorToFlag[ColorNum] = { BLACK_FLAG, WHITE_FLAG };

inline Color operator~(Color c) { return (c == BLACK) ? WHITE : BLACK; }

static std::vector<Piece> PieceList{
	BLACK_PAWN,
	BLACK_LANCE,
	BLACK_KNIGHT,
	BLACK_SILVER,
	BLACK_GOLD,
	BLACK_BISHOP,
	BLACK_ROOK,
	BLACK_KING,
	BLACK_PAWN_PROMOTE,
	BLACK_LANCE_PROMOTE,
	BLACK_KNIGHT_PROMOTE,
	BLACK_SILVER_PROMOTE,
	BLACK_BISHOP_PROMOTE,
	BLACK_ROOK_PROMOTE,
	WHITE_PAWN,
	WHITE_LANCE,
	WHITE_KNIGHT,
	WHITE_SILVER,
	WHITE_GOLD,
	WHITE_BISHOP,
	WHITE_ROOK,
	WHITE_KING,
	WHITE_PAWN_PROMOTE,
	WHITE_LANCE_PROMOTE,
	WHITE_KNIGHT_PROMOTE,
	WHITE_SILVER_PROMOTE,
	WHITE_BISHOP_PROMOTE,
	WHITE_ROOK_PROMOTE,
};

inline Piece kind(Piece p) { return static_cast<Piece>(p & PIECE_KIND_MASK); }
inline Piece promote(Piece p) { return static_cast<Piece>(p | PROMOTE); }
inline Color pieceToColor(Piece p) { if (p & BLACK_FLAG) return BLACK; else if (p & WHITE_FLAG) return WHITE; else return ColorNum; }
inline Piece toBlack(Piece p) { return static_cast<Piece>(p | BLACK_FLAG); }
inline Piece toWhite(Piece p) { return static_cast<Piece>(p | WHITE_FLAG); }

#endif