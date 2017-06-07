#ifndef SQUARE_HPP
#define SQUARE_HPP

#include"piece.hpp"
#include<map>
#include<cassert>
#include<vector>

enum Square {
    WALL00, WALL01, WALL02, WALL03, WALL04, WALL05, WALL06, WALL07, WALL08, WALL09, WALL0A,
    WALL10, SQ11, SQ12, SQ13, SQ14, SQ15, SQ16, SQ17, SQ18, SQ19, WALL1A,
    WALL20, SQ21, SQ22, SQ23, SQ24, SQ25, SQ26, SQ27, SQ28, SQ29, WALL2A,
    WALL30, SQ31, SQ32, SQ33, SQ34, SQ35, SQ36, SQ37, SQ38, SQ39, WALL3A,
    WALL40, SQ41, SQ42, SQ43, SQ44, SQ45, SQ46, SQ47, SQ48, SQ49, WALL4A,
    WALL50, SQ51, SQ52, SQ53, SQ54, SQ55, SQ56, SQ57, SQ58, SQ59, WALL5A,
    WALL60, SQ61, SQ62, SQ63, SQ64, SQ65, SQ66, SQ67, SQ68, SQ69, WALL6A,
    WALL70, SQ71, SQ72, SQ73, SQ74, SQ75, SQ76, SQ77, SQ78, SQ79, WALL7A,
    WALL80, SQ81, SQ82, SQ83, SQ84, SQ85, SQ86, SQ87, SQ88, SQ89, WALL8A,
    WALL90, SQ91, SQ92, SQ93, SQ94, SQ95, SQ96, SQ97, SQ98, SQ99, WALL9A,
    WALLA0, WALLA1, WALLA2, WALLA3, WALLA4, WALLA5, WALLA6, WALLA7, WALLA8, WALLA9, WALLAA,
    SquareNum,
};

enum File {
    File0, File1, File2, File3, File4, File5, File6, File7, File8, File9, FileA, FileNum,
};

enum Rank {
    Rank0, Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8, Rank9, RankA, RankNum,
};

enum DiagR {
    DiagR0, DiagR1, DiagR2, DiagR3, DiagR4, DiagR5, DiagR6, DiagR7, DiagR8, DiagR9, DiagRA, DiagRB, DiagRC, DiagRD, DiagRE, DiagRF, DiagRG, DiagRH, DiagRNum,
};

enum DiagL {
    DiagL0, DiagL1, DiagL2, DiagL3, DiagL4, DiagL5, DiagL6, DiagL7, DiagL8, DiagL9, DiagLA, DiagLB, DiagLC, DiagLD, DiagLE, DiagLF, DiagLG, DiagLH, DiagLNum,
};

enum Dir {
    H = 0,
    U = -1,  //上
    D = 1,  //下
    R = -11, //右
    L = 11, //左
    RU = R + U, //右上
    RD = R + D, //右下
    LD = L + D, //左下
    LU = L + U, //左上
    RUU = RU + U, //右上上
    RDD = RD + D, //右下下
    LDD = LD + D, //左下下
    LUU = LU + U, //左上上
};

enum ControlDir {
    //利きの方向を表す定数
    //真ん中のマスに対してどこから利きが来ているのかをn番目のビットを立てて表す
    //LからSは1枚駒の裏から通ってくる飛び駒の利き(影の利き)
    /*
       S  L  M
        JK9AB
         812
       RI7 3CN
         654
        HGFED
       Q  P  O
    */
    //             SRQPONMLKJIHGFEDCBA987654321
    Con_U = 0b0000000000000000000000000001,
    Con_RU = 0b0000000000000000000000000010,
    Con_R = 0b0000000000000000000000000100,
    Con_RD = 0b0000000000000000000000001000,
    Con_D = 0b0000000000000000000000010000,
    Con_LD = 0b0000000000000000000000100000,
    Con_L = 0b0000000000000000000001000000,
    Con_LU = 0b0000000000000000000010000000,
    Con_UU = 0b0000000000000000000100000000,
    Con_RUU = 0b0000000000000000001000000000,
    Con_RURU = 0b0000000000000000010000000000,
    Con_RR = 0b0000000000000000100000000000,
    Con_RDRD = 0b0000000000000001000000000000,
    Con_RDD = 0b0000000000000010000000000000,
    Con_DD = 0b0000000000000100000000000000,
    Con_LDD = 0b0000000000001000000000000000,
    Con_LDLD = 0b0000000000010000000000000000,
    Con_LL = 0b0000000000100000000000000000,
    Con_LULU = 0b0000000001000000000000000000,
    Con_LUU = 0b0000000010000000000000000000,
    Con_UUU = 0b0000000100000000000000000000,
    Con_RURURU = 0b0000001000000000000000000000,
    Con_RRR = 0b0000010000000000000000000000,
    Con_RDRDRD = 0b0000100000000000000000000000,
    Con_DDD = 0b0001000000000000000000000000,
    Con_LDLDLD = 0b0010000000000000000000000000,
    Con_LLL = 0b0100000000000000000000000000,
    Con_LULULU = 0b1000000000000000000000000000,

    DIR_MASK = 0b0000000010001010001011111111,
    JUMP_MASK = 0b0000000001110101110100000000,
    SHADOW_MASK = 0b1111111100000000000000000000,
};

static std::map<ControlDir, ControlDir> OppositeConDir = {
    {Con_U, Con_D},
    {Con_RU, Con_LD},
    {Con_R, Con_L},
    {Con_RD, Con_LU},
    {Con_D, Con_U},
    {Con_LD, Con_RU},
    {Con_L, Con_R},
    {Con_LU, Con_RD},
    {Con_UU, Con_DD},
    {Con_RUU, Con_LDD},
    {Con_RURU, Con_LDLD},
    {Con_RR, Con_LL},
    {Con_RDRD, Con_LULU},
    {Con_RDD, Con_LUU},
    {Con_DD, Con_UU},
    {Con_LDD, Con_RUU},
    {Con_LDLD, Con_RURU},
    {Con_LL, Con_RR},
    {Con_LULU, Con_RDRD},
    {Con_LUU, Con_RDD},
    {Con_UUU, Con_DDD},
    {Con_RURURU, Con_LDLDLD},
    {Con_RRR, Con_LLL},
    {Con_RDRDRD, Con_LULULU},
    {Con_DDD, Con_UUU},
    {Con_LDLDLD, Con_RURURU},
    {Con_LLL, Con_RRR},
    {Con_LULULU, Con_RDRDRD},
};

const Rank SquareToRank[SquareNum] = {
    Rank0, Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8, Rank9, RankA,
    Rank0, Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8, Rank9, RankA,
    Rank0, Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8, Rank9, RankA,
    Rank0, Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8, Rank9, RankA,
    Rank0, Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8, Rank9, RankA,
    Rank0, Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8, Rank9, RankA,
    Rank0, Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8, Rank9, RankA,
    Rank0, Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8, Rank9, RankA,
    Rank0, Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8, Rank9, RankA,
    Rank0, Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8, Rank9, RankA,
    Rank0, Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8, Rank9, RankA,
};

const File SquareToFile[SquareNum] = {
    File0, File0, File0, File0, File0, File0, File0, File0, File0, File0, File0,
    File1, File1, File1, File1, File1, File1, File1, File1, File1, File1, File1,
    File2, File2, File2, File2, File2, File2, File2, File2, File2, File2, File2,
    File3, File3, File3, File3, File3, File3, File3, File3, File3, File3, File3,
    File4, File4, File4, File4, File4, File4, File4, File4, File4, File4, File4,
    File5, File5, File5, File5, File5, File5, File5, File5, File5, File5, File5,
    File6, File6, File6, File6, File6, File6, File6, File6, File6, File6, File6,
    File7, File7, File7, File7, File7, File7, File7, File7, File7, File7, File7,
    File8, File8, File8, File8, File8, File8, File8, File8, File8, File8, File8,
    File9, File9, File9, File9, File9, File9, File9, File9, File9, File9, File9,
    FileA, FileA, FileA, FileA, FileA, FileA, FileA, FileA, FileA, FileA, FileA,
};

//斜め方向右上がり
const DiagR SquareToDiagR[SquareNum] = {
    DiagR0, DiagR0, DiagR0, DiagR0, DiagR0, DiagR0, DiagR0, DiagR0, DiagR0, DiagR0, DiagR0,
    DiagR0, DiagR1, DiagR2, DiagR3, DiagR4, DiagR5, DiagR6, DiagR7, DiagR8, DiagR9, DiagR0,
    DiagR0, DiagR2, DiagR3, DiagR4, DiagR5, DiagR6, DiagR7, DiagR8, DiagR9, DiagRA, DiagR0,
    DiagR0, DiagR3, DiagR4, DiagR5, DiagR6, DiagR7, DiagR8, DiagR9, DiagRA, DiagRB, DiagR0,
    DiagR0, DiagR4, DiagR5, DiagR6, DiagR7, DiagR8, DiagR9, DiagRA, DiagRB, DiagRC, DiagR0,
    DiagR0, DiagR5, DiagR6, DiagR7, DiagR8, DiagR9, DiagRA, DiagRB, DiagRC, DiagRD, DiagR0,
    DiagR0, DiagR6, DiagR7, DiagR8, DiagR9, DiagRA, DiagRB, DiagRC, DiagRD, DiagRE, DiagR0,
    DiagR0, DiagR7, DiagR8, DiagR9, DiagRA, DiagRB, DiagRC, DiagRD, DiagRE, DiagRF, DiagR0,
    DiagR0, DiagR8, DiagR9, DiagRA, DiagRB, DiagRC, DiagRD, DiagRE, DiagRF, DiagRG, DiagR0,
    DiagR0, DiagR9, DiagRA, DiagRB, DiagRC, DiagRD, DiagRE, DiagRF, DiagRG, DiagRH, DiagR0,
    DiagR0, DiagR0, DiagR0, DiagR0, DiagR0, DiagR0, DiagR0, DiagR0, DiagR0, DiagR0, DiagR0,
};

//斜め方向左上がり
const DiagL SquareToDiagL[SquareNum] = {
    DiagL0, DiagL0, DiagL0, DiagL0, DiagL0, DiagL0, DiagL0, DiagL0, DiagL0, DiagL0, DiagL0,
    DiagL0, DiagL9, DiagL8, DiagL7, DiagL6, DiagL5, DiagL4, DiagL3, DiagL2, DiagL1, DiagL0,
    DiagL0, DiagLA, DiagL9, DiagL8, DiagL7, DiagL6, DiagL5, DiagL4, DiagL3, DiagL2, DiagL0,
    DiagL0, DiagLB, DiagLA, DiagL9, DiagL8, DiagL7, DiagL6, DiagL5, DiagL4, DiagL3, DiagL0,
    DiagL0, DiagLC, DiagLB, DiagLA, DiagL9, DiagL8, DiagL7, DiagL6, DiagL5, DiagL4, DiagL0,
    DiagL0, DiagLD, DiagLC, DiagLB, DiagLA, DiagL9, DiagL8, DiagL7, DiagL6, DiagL5, DiagL0,
    DiagL0, DiagLE, DiagLD, DiagLC, DiagLB, DiagLA, DiagL9, DiagL8, DiagL7, DiagL6, DiagL0,
    DiagL0, DiagLF, DiagLE, DiagLD, DiagLC, DiagLB, DiagLA, DiagL9, DiagL8, DiagL7, DiagL0,
    DiagL0, DiagLG, DiagLF, DiagLE, DiagLD, DiagLC, DiagLB, DiagLA, DiagL9, DiagL8, DiagL0,
    DiagL0, DiagLH, DiagLG, DiagLF, DiagLE, DiagLD, DiagLC, DiagLB, DiagLA, DiagL9, DiagL0,
    DiagL0, DiagL0, DiagL0, DiagL0, DiagL0, DiagL0, DiagL0, DiagL0, DiagL0, DiagL0, DiagL0,
};

const Square FRToSquare[FileNum][RankNum] = {
    {WALL00, WALL01, WALL02, WALL03, WALL04, WALL05, WALL06, WALL07, WALL08, WALL09, WALL0A},
    {WALL10, SQ11,   SQ12,   SQ13,   SQ14,   SQ15,   SQ16,   SQ17,   SQ18,   SQ19,   WALL1A},
    {WALL20, SQ21,   SQ22,   SQ23,   SQ24,   SQ25,   SQ26,   SQ27,   SQ28,   SQ29,   WALL2A},
    {WALL30, SQ31,   SQ32,   SQ33,   SQ34,   SQ35,   SQ36,   SQ37,   SQ38,   SQ39,   WALL3A},
    {WALL40, SQ41,   SQ42,   SQ43,   SQ44,   SQ45,   SQ46,   SQ47,   SQ48,   SQ49,   WALL4A},
    {WALL50, SQ51,   SQ52,   SQ53,   SQ54,   SQ55,   SQ56,   SQ57,   SQ58,   SQ59,   WALL5A},
    {WALL60, SQ61,   SQ62,   SQ63,   SQ64,   SQ65,   SQ66,   SQ67,   SQ68,   SQ69,   WALL6A},
    {WALL70, SQ71,   SQ72,   SQ73,   SQ74,   SQ75,   SQ76,   SQ77,   SQ78,   SQ79,   WALL7A},
    {WALL80, SQ81,   SQ82,   SQ83,   SQ84,   SQ85,   SQ86,   SQ87,   SQ88,   SQ89,   WALL8A},
    {WALL90, SQ91,   SQ92,   SQ93,   SQ94,   SQ95,   SQ96,   SQ97,   SQ98,   SQ99,   WALL9A},
    {WALLA0, WALLA1, WALLA2, WALLA3, WALLA4, WALLA5, WALLA6, WALLA7, WALLA8, WALLA9, WALLAA},
};

static inline bool isOnBoard(Square pos)
{
    return (Rank1 <= SquareToRank[pos] && SquareToRank[pos] <= Rank9 && File1 <= SquareToFile[pos] && SquareToFile[pos] <= File9);
}

static Dir DirList[8] = {
    //前から時計回りに
    U, RU, R, RD, D, LD, L, LU
};

static ControlDir DirectControlList[8] = {
    //前から時計回りに
    Con_U, Con_RU, Con_R, Con_RD, Con_D, Con_LD, Con_L, Con_LU
};

static ControlDir JumpControlList[8] = {
    //前から時計回りに
    Con_UU,	Con_RURU, Con_RR, Con_RDRD,	Con_DD,	Con_LDLD, Con_LL, Con_LULU,
};

static ControlDir ShadowControlList[8] = {
    //前から時計回りに
    Con_UUU, Con_RURURU, Con_RRR, Con_RDRDRD, Con_DDD, Con_LDLDLD, Con_LLL, Con_LULULU,
};

static std::map<Dir, Dir> OppositeDir = {
    { U, D },
    { RU, LD },
    { R, L },
    { RD, LU },
    { D, U },
    { LD, RU },
    { L, R },
    { LU, RD },
};

static std::map<Dir, ControlDir> DirToControlDir = {
    { U, Con_U },
    { RU, Con_RU },
    { R, Con_R },
    { RD, Con_RD },
    { D, Con_D },
    { LD, Con_LD },
    { L, Con_L },
    { LU, Con_LU },
    { RUU, Con_RUU },
    { RDD, Con_RDD },
    { LDD, Con_LDD },
    { LUU, Con_LUU },
};

static std::map<ControlDir, Dir> ConDirToOppositeDir = {
    { Con_UU, D },
    { Con_RURU, LD },
    { Con_RR, L },
    { Con_RDRD, LU },
    { Con_DD, U },
    { Con_LDLD, RU },
    { Con_LL, R },
    { Con_LULU, RD },

    { Con_UUU, D },
    { Con_RURURU, LD },
    { Con_RRR, L },
    { Con_RDRDRD, LU },
    { Con_DDD, U },
    { Con_LDLDLD, RU },
    { Con_LLL, R },
    { Con_LULULU, RD },
};

static std::map<Dir, ControlDir> DirToOppositeJumpControl = {
    { D, Con_UU},
    { LD, Con_RURU },
    { L, Con_RR },
    { LU, Con_RDRD },
    { U, Con_DD },
    { RU, Con_LDLD },
    { R, Con_LL },
    { RD, Con_LULU },
};

static std::map<Dir, ControlDir> DirToOppositeConDir = {
    { U, Con_D },
    { RU, Con_LD },
    { R, Con_L },
    { RD, Con_LU },
    { D, Con_U },
    { LD, Con_RU },
    { L, Con_R },
    { LU, Con_RD },
    { LDD, Con_RUU },
    { LUU, Con_RDD },
    { RUU, Con_LDD },
    { RDD, Con_LUU },
};

static std::map<ControlDir, ControlDir> JumpControlToShadowControl = {
    { Con_UU, Con_UUU },
    { Con_RURU, Con_RURURU },
    { Con_RR, Con_RRR },
    { Con_RDRD, Con_RDRDRD },
    { Con_DD, Con_DDD },
    { Con_LDLD, Con_LDLDLD },
    { Con_LL, Con_LLL },
    { Con_LULU, Con_LULULU },
};

inline Dir directionAtoB(Square A, Square B)
{
    //8方向のうちどれかか、あるいはどれでもないかだけ判定できればいい
    //Aの位置を0とすると周囲8マスは
    //10 -1 -12
    //11  0 -11
    //12  1 -10
    //だから差の正負と段、筋、斜めの一致具合で方向がわかるはず
    if (A == B) return H;
    else if (B - A > 0) {
        if (SquareToRank[A] == SquareToRank[B]) return L;
        if (SquareToFile[A] == SquareToFile[B]) return D;
        if (SquareToDiagR[A] == SquareToDiagR[B]) return LU;
        if (SquareToDiagL[A] == SquareToDiagL[B]) return LD;
    } else {
        if (SquareToRank[A] == SquareToRank[B]) return R;
        if (SquareToFile[A] == SquareToFile[B]) return U;
        if (SquareToDiagR[A] == SquareToDiagR[B]) return RD;
        if (SquareToDiagL[A] == SquareToDiagL[B]) return RU;
    }
    return H;
}

static std::map<Piece, std::vector<Dir> > CanMove = {
    { EMPTY, {} },
    { BLACK_PAWN,    { U } },
    { BLACK_LANCE,   {} },
    { BLACK_KNIGHT,  { RUU, LUU } },
    { BLACK_SILVER,  { U, RU, RD, LD, LU } },
    { BLACK_GOLD,    { U, RU, R, D, L, LU } },
    { BLACK_BISHOP,  {} },
    { BLACK_ROOK,    {} },
    { BLACK_KING,    { U, RU, R, RD, D, LD, L, LU } },
    { BLACK_PAWN_PROMOTE,   { U, RU, R, D, L, LU } },
    { BLACK_LANCE_PROMOTE,  { U, RU, R, D, L, LU } },
    { BLACK_KNIGHT_PROMOTE, { U, RU, R, D, L, LU } },
    { BLACK_SILVER_PROMOTE, { U, RU, R, D, L, LU } },
    { BLACK_BISHOP_PROMOTE, { U, R, D, L } },
    { BLACK_ROOK_PROMOTE,   { RU, RD, LD, LU } },
    { WHITE_PAWN,    { D } },
    { WHITE_LANCE,   {} },
    { WHITE_KNIGHT,  { RDD, LDD } },
    { WHITE_SILVER,  { RU, RD, D, LD, LU } },
    { WHITE_GOLD,    { U, R, RD, D, LD, L } },
    { WHITE_BISHOP,  {} },
    { WHITE_ROOK,    {} },
    { WHITE_KING,{ U, RU, R, RD, D, LD, L, LU } },
    { WHITE_PAWN_PROMOTE,   { U, R, RD, D, LD, L } },
    { WHITE_LANCE_PROMOTE,  { U, R, RD, D, LD, L } },
    { WHITE_KNIGHT_PROMOTE, { U, R, RD, D, LD, L } },
    { WHITE_SILVER_PROMOTE, { U, R, RD, D, LD, L } },
    { WHITE_BISHOP_PROMOTE, { U, R, D, L } },
    { WHITE_ROOK_PROMOTE,   { RU, RD, LD, LU } },
};

static std::map<Piece, std::vector<Dir> > CanJump = {
    { EMPTY, {} },
    { BLACK_PAWN,{} },
    { BLACK_LANCE,{ U } },
    { BLACK_KNIGHT,{} },
    { BLACK_SILVER,{} },
    { BLACK_GOLD,{} },
    { BLACK_BISHOP,{ RU, RD, LD, LU } },
    { BLACK_ROOK,{ U, R, D, L } },
    { BLACK_PAWN_PROMOTE,{} },
    { BLACK_LANCE_PROMOTE,{} },
    { BLACK_KNIGHT_PROMOTE,{} },
    { BLACK_SILVER_PROMOTE,{} },
    { BLACK_BISHOP_PROMOTE,{ RU, RD, LD, LU } },
    { BLACK_ROOK_PROMOTE,{ U, R, D, L } },
    { WHITE_PAWN,{} },
    { WHITE_LANCE,{ D } },
    { WHITE_KNIGHT,{} },
    { WHITE_SILVER,{} },
    { WHITE_GOLD,{} },
    { WHITE_BISHOP,{ RU, RD, LD, LU } },
    { WHITE_ROOK,{ U, R, D, L } },
    { WHITE_PAWN_PROMOTE,{} },
    { WHITE_LANCE_PROMOTE,{} },
    { WHITE_KNIGHT_PROMOTE,{} },
    { WHITE_SILVER_PROMOTE,{} },
    { WHITE_BISHOP_PROMOTE,{ RU, RD, LD, LU } },
    { WHITE_ROOK_PROMOTE,{ U, R, D, L } },
};

inline static Square operator+(Square place, Dir diff) {
    return static_cast<Square>(static_cast<int>(place) + static_cast<int>(diff));
}

inline static int operator<<(Square sq, int shift) {
    return static_cast<int>(static_cast<int>(sq) << shift);
}

static std::vector<Square> SquareList{
    SQ11, SQ12, SQ13, SQ14, SQ15, SQ16, SQ17, SQ18, SQ19,
    SQ21, SQ22, SQ23, SQ24, SQ25, SQ26, SQ27, SQ28, SQ29,
    SQ31, SQ32, SQ33, SQ34, SQ35, SQ36, SQ37, SQ38, SQ39,
    SQ41, SQ42, SQ43, SQ44, SQ45, SQ46, SQ47, SQ48, SQ49,
    SQ51, SQ52, SQ53, SQ54, SQ55, SQ56, SQ57, SQ58, SQ59,
    SQ61, SQ62, SQ63, SQ64, SQ65, SQ66, SQ67, SQ68, SQ69,
    SQ71, SQ72, SQ73, SQ74, SQ75, SQ76, SQ77, SQ78, SQ79,
    SQ81, SQ82, SQ83, SQ84, SQ85, SQ86, SQ87, SQ88, SQ89,
    SQ91, SQ92, SQ93, SQ94, SQ95, SQ96, SQ97, SQ98, SQ99,
};

static int SquareToNum[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8, -1,
    -1,  9, 10, 11, 12, 13, 14, 15, 16, 17, -1,
    -1, 18, 19, 20, 21, 22, 23, 24, 25, 26, -1,
    -1, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1,
    -1, 36, 37, 38, 39, 40, 41, 42, 43, 44, -1,
    -1, 45, 46, 47, 48, 49, 50, 51, 52, 53, -1,
    -1, 54, 55, 56, 57, 58, 59, 60, 61, 62, -1,
    -1, 63, 64, 65, 66, 67, 68, 69, 70, 71, -1,
    -1, 72, 73, 74, 75, 76, 77, 78, 79, 80, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

static Square InvSquare[] = {
    WALL00, WALL01, WALL02, WALL03, WALL04, WALL05, WALL06, WALL07, WALL08, WALL09, WALL0A,
    WALL10, SQ99,   SQ98,   SQ97,   SQ96,   SQ95,   SQ94,   SQ93,   SQ92,   SQ91,   WALL1A,
    WALL20, SQ89,   SQ88,   SQ87,   SQ86,   SQ85,   SQ84,   SQ83,   SQ82,   SQ81,   WALL2A,
    WALL30, SQ79,   SQ78,   SQ77,   SQ76,   SQ75,   SQ74,   SQ73,   SQ72,   SQ71,   WALL3A,
    WALL40, SQ69,   SQ68,   SQ67,   SQ66,   SQ65,   SQ64,   SQ63,   SQ62,   SQ61,   WALL4A,
    WALL50, SQ59,   SQ58,   SQ57,   SQ56,   SQ55,   SQ54,   SQ53,   SQ52,   SQ51,   WALL5A,
    WALL60, SQ49,   SQ48,   SQ47,   SQ46,   SQ45,   SQ44,   SQ43,   SQ42,   SQ41,   WALL6A,
    WALL70, SQ39,   SQ38,   SQ37,   SQ36,   SQ35,   SQ34,   SQ33,   SQ32,   SQ31,   WALL7A,
    WALL80, SQ29,   SQ28,   SQ27,   SQ26,   SQ25,   SQ24,   SQ23,   SQ22,   SQ21,   WALL8A,
    WALL90, SQ19,   SQ18,   SQ17,   SQ16,   SQ15,   SQ14,   SQ13,   SQ12,   SQ11,   WALL9A,
    WALLA0, WALLA1, WALLA2, WALLA3, WALLA4, WALLA5, WALLA6, WALLA7, WALLA8, WALLA9, WALLAA,
};

#endif
