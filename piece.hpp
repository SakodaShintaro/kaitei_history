#pragma once

#ifndef PIECE_HPP
#define PIECE_HPP

#include<map>
#include<string>
#include<vector>
#include<iostream>
#include<fstream>
#include<cassert>
#define PROMOTE_BIT 5
#define BLACK_BIT 6
#define WHITE_BIT 7
#define WALL_BIT 8

enum Piece {
    EMPTY = 0,                                          //0000 0000
    PAWN = 1,                                           //0000 0001
    LANCE = 2,                                          //0000 0010
    KNIGHT = 3,                                         //0000 0011
    SILVER = 4,                                         //0000 0100
    GOLD = 5,                                           //0000 0101
    BISHOP = 6,                                         //0000 0110
    ROOK = 7,                                           //0000 0111
    KING = 8,                                           //0000 1000
    PIECE_KIND_MASK = 15,                               //0000 1111
    PROMOTE = 1 << (PROMOTE_BIT - 1),                   //0001 0000  16
    PAWN_PROMOTE = PAWN + PROMOTE,                      //0001 0001  17
    LANCE_PROMOTE = LANCE + PROMOTE,                    //0001 0010  18
    KNIGHT_PROMOTE = KNIGHT + PROMOTE,                  //0001 0011  19
    SILVER_PROMOTE = SILVER + PROMOTE,                  //0001 0100  20
    BISHOP_PROMOTE = BISHOP + PROMOTE,                  //0001 0110  22
    ROOK_PROMOTE = ROOK + PROMOTE,                      //0001 0111  23
    BLACK_FLAG = 1 << (BLACK_BIT - 1),                  //0010 0000  32
    BLACK_PAWN = PAWN + BLACK_FLAG,                     //0010 0001  33
    BLACK_LANCE = LANCE + BLACK_FLAG,                   //0010 0010  34
    BLACK_KNIGHT = KNIGHT + BLACK_FLAG,                 //0010 0011  35
    BLACK_SILVER = SILVER + BLACK_FLAG,                 //0010 0100  36
    BLACK_GOLD = GOLD + BLACK_FLAG,                     //0010 0101  37
    BLACK_BISHOP = BISHOP + BLACK_FLAG,                 //0010 0110  38
    BLACK_ROOK = ROOK + BLACK_FLAG,                     //0010 0111  39
    BLACK_KING = KING + BLACK_FLAG,                     //0010 1000  40
    BLACK_PAWN_PROMOTE = PAWN_PROMOTE + BLACK_FLAG,     //0011 0001  49
    BLACK_LANCE_PROMOTE = LANCE_PROMOTE + BLACK_FLAG,   //0011 0010  50
    BLACK_KNIGHT_PROMOTE = KNIGHT_PROMOTE + BLACK_FLAG, //0011 0011  51
    BLACK_SILVER_PROMOTE = SILVER_PROMOTE + BLACK_FLAG, //0011 0100  52
    BLACK_BISHOP_PROMOTE = BISHOP_PROMOTE + BLACK_FLAG, //0011 0110  54
    BLACK_ROOK_PROMOTE = ROOK_PROMOTE + BLACK_FLAG,     //0011 0111  55
    WHITE_FLAG = 1 << (WHITE_BIT - 1),                  //0100 0000  64
    WHITE_PAWN = PAWN + WHITE_FLAG,                     //0100 0001  65
    WHITE_LANCE = LANCE + WHITE_FLAG,                   //0100 0010  66
    WHITE_KNIGHT = KNIGHT + WHITE_FLAG,                 //0100 0011  67
    WHITE_SILVER = SILVER + WHITE_FLAG,                 //0100 0100  68
    WHITE_GOLD = GOLD + WHITE_FLAG,                     //0100 0101  69
    WHITE_BISHOP = BISHOP + WHITE_FLAG,                 //0100 0110  70
    WHITE_ROOK = ROOK + WHITE_FLAG,                     //0100 0111  71
    WHITE_KING = KING + WHITE_FLAG,                     //0100 1000  72
    WHITE_PAWN_PROMOTE = PAWN_PROMOTE + WHITE_FLAG,     //0101 0001  81
    WHITE_LANCE_PROMOTE = LANCE_PROMOTE + WHITE_FLAG,   //0101 0010  82
    WHITE_KNIGHT_PROMOTE = KNIGHT_PROMOTE + WHITE_FLAG, //0101 0011  83
    WHITE_SILVER_PROMOTE = SILVER_PROMOTE + WHITE_FLAG, //0101 0100  84
    WHITE_BISHOP_PROMOTE = BISHOP_PROMOTE + WHITE_FLAG, //0101 0110  86
    WHITE_ROOK_PROMOTE = ROOK_PROMOTE + WHITE_FLAG,     //0101 0111  87
    PieceNum,
    WALL = 1 << (WALL_BIT),                             //1000 0000
};

enum Color {
    BLACK, WHITE, ColorNum,
};

static std::map<Piece, std::string> PieceToStr = {
    { PAWN,   "歩"},
    { LANCE,  "香"},
    { KNIGHT, "桂"},
    { SILVER, "銀"},
    { GOLD,   "金"},
    { BISHOP, "角"},
    { ROOK,   "飛"},
    { BLACK_PAWN,    "先手歩" },
    { BLACK_LANCE,   "先手香" },
    { BLACK_KNIGHT,  "先手桂" },
    { BLACK_SILVER,  "先手銀" },
    { BLACK_GOLD,    "先手金" },
    { BLACK_BISHOP,  "先手角" },
    { BLACK_ROOK,    "先手飛車" },
    { BLACK_KING,    "先手玉" },
    { BLACK_PAWN_PROMOTE,   "先手と" },
    { BLACK_LANCE_PROMOTE,  "先手成香" },
    { BLACK_KNIGHT_PROMOTE, "先手成桂" },
    { BLACK_SILVER_PROMOTE, "先手成銀" },
    { BLACK_BISHOP_PROMOTE, "先手馬" },
    { BLACK_ROOK_PROMOTE,   "先手竜" },
    { WHITE_PAWN,    "後手歩" },
    { WHITE_LANCE,   "後手香" },
    { WHITE_KNIGHT,  "後手桂" },
    { WHITE_SILVER,  "後手銀" },
    { WHITE_GOLD,    "後手金" },
    { WHITE_BISHOP,  "後手角" },
    { WHITE_ROOK,    "後手飛車" },
    { WHITE_KING,    "後手玉" },
    { WHITE_PAWN_PROMOTE,   "後手と" },
    { WHITE_LANCE_PROMOTE,  "後手成香" },
    { WHITE_KNIGHT_PROMOTE, "後手成桂" },
    { WHITE_SILVER_PROMOTE, "後手成銀" },
    { WHITE_BISHOP_PROMOTE, "後手馬" },
    { WHITE_ROOK_PROMOTE,   "後手竜" },
};

static std::map<Piece, std::string> PieceToSfenStr = {
    { EMPTY,  "  "},
    { PAWN,   "P" },
    { LANCE,  "L" },
    { KNIGHT, "N" },
    { SILVER, "S" },
    { GOLD,   "G" },
    { BISHOP, "B" },
    { ROOK,   "R" },
    { BLACK_PAWN,    " P" },
    { BLACK_LANCE,   " L" },
    { BLACK_KNIGHT,  " N" },
    { BLACK_SILVER,  " S" },
    { BLACK_GOLD,    " G" },
    { BLACK_BISHOP,  " B" },
    { BLACK_ROOK,    " R" },
    { BLACK_KING,    " K" },
    { BLACK_PAWN_PROMOTE,   "+P" },
    { BLACK_LANCE_PROMOTE,  "+L" },
    { BLACK_KNIGHT_PROMOTE, "+N" },
    { BLACK_SILVER_PROMOTE, "+S" },
    { BLACK_BISHOP_PROMOTE, "+B" },
    { BLACK_ROOK_PROMOTE,   "+R" },
    { WHITE_PAWN,    " p" },
    { WHITE_LANCE,   " l" },
    { WHITE_KNIGHT,  " n" },
    { WHITE_SILVER,  " s" },
    { WHITE_GOLD,    " g" },
    { WHITE_BISHOP,  " b" },
    { WHITE_ROOK,    " r" },
    { WHITE_KING,    " k" },
    { WHITE_PAWN_PROMOTE,   "+p" },
    { WHITE_LANCE_PROMOTE,  "+l" },
    { WHITE_KNIGHT_PROMOTE, "+n" },
    { WHITE_SILVER_PROMOTE, "+s" },
    { WHITE_BISHOP_PROMOTE, "+b" },
    { WHITE_ROOK_PROMOTE,   "+r" },
};

inline static Piece operator++(Piece &p, int) { return p = static_cast<Piece>(p + 1); }

inline static int operator<<(Piece p, int shift) { return static_cast<int>(p) << shift; }

static Piece ColorToFlag[ColorNum] = { BLACK_FLAG, WHITE_FLAG };

inline static Color operator~(Color c) { return (c == BLACK) ? WHITE : BLACK; }

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

enum PieceState {
    black_hand_pawn = 0,
    white_hand_pawn = black_hand_pawn + 18,
    black_hand_lance = white_hand_pawn + 18,
    white_hand_lance = black_hand_lance + 4,
    black_hand_knight = white_hand_lance + 4,
    white_hand_knight = black_hand_knight + 4,
    black_hand_silver = white_hand_knight + 4,
    white_hand_silver = black_hand_silver + 4,
    black_hand_gold = white_hand_silver + 4,
    white_hand_gold = black_hand_gold + 4,
    black_hand_bishop = white_hand_gold + 4,
    white_hand_bishop = black_hand_bishop + 2,
    black_hand_rook = white_hand_bishop + 2,
    white_hand_rook = black_hand_rook + 2,
    hand_end = white_hand_rook + 2,

    black_pawn = hand_end,
    white_pawn = black_pawn + 81,
    black_lance = white_pawn + 81,
    white_lance = black_lance + 81,
    black_knight = white_lance + 81,
    white_knight = black_knight + 81,
    black_silver = white_knight + 81,
    white_silver = black_silver + 81,
    black_gold = white_silver + 81,
    white_gold = black_gold + 81,
    black_bishop = white_gold + 81,
    white_bishop = black_bishop + 81,
    black_rook = white_bishop + 81,
    white_rook = black_rook + 81,
    black_horse = white_rook + 81,
    white_horse = black_horse + 81,
    black_dragon = white_horse + 81,
    white_dragon = black_dragon + 81,
    square_end = white_dragon + 81,
    PieceStateNum = square_end,
};

//static std::map<Piece, PieceState> PieceToStateIndex = {
//	{ PAWN,   black_hand_pawn },
//	{ LANCE,  black_hand_lance },
//	{ KNIGHT, black_hand_knight },
//	{ SILVER, black_hand_silver },
//	{ GOLD,   black_hand_gold },
//	{ BISHOP, black_hand_bishop },
//	{ ROOK,   black_hand_rook },
//	{ BLACK_PAWN,    black_pawn },
//	{ BLACK_LANCE,   black_lance },
//	{ BLACK_KNIGHT,  black_knight },
//	{ BLACK_SILVER,  black_silver },
//	{ BLACK_GOLD,    black_gold },
//	{ BLACK_BISHOP,  black_bishop },
//	{ BLACK_ROOK,    black_rook },
//	{ BLACK_PAWN_PROMOTE,   black_gold },
//	{ BLACK_LANCE_PROMOTE,  black_gold },
//	{ BLACK_KNIGHT_PROMOTE, black_gold },
//	{ BLACK_SILVER_PROMOTE, black_gold },
//	{ BLACK_BISHOP_PROMOTE, black_gold },
//	{ BLACK_ROOK_PROMOTE,   black_gold },
//	{ WHITE_PAWN,    white_pawn },
//	{ WHITE_LANCE,   white_lance },
//	{ WHITE_KNIGHT,  white_knight },
//	{ WHITE_SILVER,  white_silver },
//	{ WHITE_GOLD,    white_gold },
//	{ WHITE_BISHOP,  white_bishop },
//	{ WHITE_ROOK,    white_rook },
//	{ WHITE_PAWN_PROMOTE,   white_gold },
//	{ WHITE_LANCE_PROMOTE,  white_gold },
//	{ WHITE_KNIGHT_PROMOTE, white_gold },
//	{ WHITE_SILVER_PROMOTE, white_gold },
//	{ WHITE_BISHOP_PROMOTE, white_gold },
//	{ WHITE_ROOK_PROMOTE,   white_gold },
//};

static PieceState PieceToStateIndex[PieceStateNum];

static void initPieceToStateIndex() {
    PieceToStateIndex[PAWN] = black_hand_pawn;
    PieceToStateIndex[LANCE] = black_hand_lance;
    PieceToStateIndex[KNIGHT] = black_hand_knight;
    PieceToStateIndex[SILVER] = black_hand_silver;
    PieceToStateIndex[GOLD] = black_hand_gold;
    PieceToStateIndex[BISHOP] = black_hand_bishop;
    PieceToStateIndex[ROOK] = black_hand_rook;
    PieceToStateIndex[BLACK_PAWN] = black_pawn;
    PieceToStateIndex[BLACK_LANCE] = black_lance;
    PieceToStateIndex[BLACK_KNIGHT] = black_knight;
    PieceToStateIndex[BLACK_SILVER] = black_silver;
    PieceToStateIndex[BLACK_GOLD] = black_gold;
    PieceToStateIndex[BLACK_BISHOP] = black_bishop;
    PieceToStateIndex[BLACK_ROOK] = black_rook;
    PieceToStateIndex[BLACK_PAWN_PROMOTE] = black_gold;
    PieceToStateIndex[BLACK_LANCE_PROMOTE] = black_gold;
    PieceToStateIndex[BLACK_KNIGHT_PROMOTE] = black_gold;
    PieceToStateIndex[BLACK_SILVER_PROMOTE] = black_gold;
    PieceToStateIndex[BLACK_BISHOP_PROMOTE] = black_gold;
    PieceToStateIndex[BLACK_ROOK_PROMOTE] = black_gold;
    PieceToStateIndex[WHITE_PAWN] = white_pawn;
    PieceToStateIndex[WHITE_LANCE] = white_lance;
    PieceToStateIndex[WHITE_KNIGHT] = white_knight;
    PieceToStateIndex[WHITE_SILVER] = white_silver;
    PieceToStateIndex[WHITE_GOLD] = white_gold;
    PieceToStateIndex[WHITE_BISHOP] = white_bishop;
    PieceToStateIndex[WHITE_ROOK] = white_rook;
    PieceToStateIndex[WHITE_PAWN_PROMOTE] = white_gold;
    PieceToStateIndex[WHITE_LANCE_PROMOTE] = white_gold;
    PieceToStateIndex[WHITE_KNIGHT_PROMOTE] = white_gold;
    PieceToStateIndex[WHITE_SILVER_PROMOTE] = white_gold;
    PieceToStateIndex[WHITE_BISHOP_PROMOTE] = white_gold;
    PieceToStateIndex[WHITE_ROOK_PROMOTE] = white_gold;
}

static PieceState PieceStateIndex[] = {
    black_hand_pawn,
    white_hand_pawn,
    black_hand_lance,
    white_hand_lance,
    black_hand_knight,
    white_hand_knight,
    black_hand_silver,
    white_hand_silver,
    black_hand_gold,
    white_hand_gold,
    black_hand_bishop,
    white_hand_bishop,
    black_hand_rook,
    white_hand_rook,
    black_pawn,
    white_pawn,
    black_lance,
    white_lance,
    black_knight,
    white_knight,
    black_silver,
    white_silver,
    black_gold,
    white_gold,
    black_bishop,
    white_bishop,
    black_rook,
    white_rook,
    black_horse,
    white_horse,
    black_dragon,
    white_dragon,
    square_end,
    PieceStateNum,
};

static PieceState index(PieceState ps)
{
    for (int i = 0; i < 34 - 1; i++) {
        if (PieceStateIndex[i] <= ps && ps < PieceStateIndex[i + 1]) {
            return PieceStateIndex[i];
        }
    }
    std::cout << "index(PieceState) がおかしい" << std::endl;
    return PieceStateNum;
}

static PieceState invPieceStateMap[PieceStateNum];

static void initInvPieceStateMap()
{
    invPieceStateMap[black_hand_pawn] = white_hand_pawn;
    invPieceStateMap[black_hand_lance] = white_hand_lance;
    invPieceStateMap[black_hand_knight] = white_hand_knight;
    invPieceStateMap[black_hand_silver] = white_hand_silver;
    invPieceStateMap[black_hand_gold] = white_hand_gold;
    invPieceStateMap[black_hand_bishop] = white_hand_bishop;
    invPieceStateMap[black_hand_rook] = white_hand_rook;

    invPieceStateMap[white_hand_pawn] = black_hand_pawn;
    invPieceStateMap[white_hand_lance] = black_hand_lance;
    invPieceStateMap[white_hand_knight] = black_hand_knight;
    invPieceStateMap[white_hand_silver] = black_hand_silver;
    invPieceStateMap[white_hand_gold] = black_hand_gold;
    invPieceStateMap[white_hand_bishop] = black_hand_bishop;
    invPieceStateMap[white_hand_rook] = black_hand_rook;

    invPieceStateMap[black_pawn] = white_pawn;
    invPieceStateMap[black_lance] = white_lance;
    invPieceStateMap[black_knight] = white_knight;
    invPieceStateMap[black_silver] = white_silver;
    invPieceStateMap[black_gold] = white_gold;
    invPieceStateMap[black_bishop] = white_bishop;
    invPieceStateMap[black_rook] = white_rook;
    invPieceStateMap[black_bishop] = white_horse;
    invPieceStateMap[black_rook] = white_dragon;

    invPieceStateMap[white_pawn] = black_pawn;
    invPieceStateMap[white_lance] = black_lance;
    invPieceStateMap[white_knight] = black_knight;
    invPieceStateMap[white_silver] = black_silver;
    invPieceStateMap[white_gold] = black_gold;
    invPieceStateMap[white_bishop] = black_bishop;
    invPieceStateMap[white_rook] = black_rook;
    invPieceStateMap[white_bishop] = black_horse;
    invPieceStateMap[white_rook] = black_dragon;
}

inline static PieceState invPieceState(PieceState ps)
{
    PieceState i = index(ps);
    if (i < hand_end) {
        return static_cast<PieceState>(invPieceStateMap[i] + ps - i);
    } else {
        //マスの番号を反転したもの
        int sq = 80 - (ps - i);
        return static_cast<PieceState>(invPieceStateMap[i] + sq);
    }
}

inline static Piece kind(Piece p) { return static_cast<Piece>(p & PIECE_KIND_MASK); }
inline static Piece promote(Piece p) { return static_cast<Piece>(p | PROMOTE); }
inline static Color pieceToColor(Piece p) { if (p & BLACK_FLAG) return BLACK; else if (p & WHITE_FLAG) return WHITE; else return ColorNum; }
inline static Piece toBlack(Piece p) { return static_cast<Piece>(p | BLACK_FLAG); }
inline static Piece toWhite(Piece p) { return static_cast<Piece>(p | WHITE_FLAG); }

#endif
