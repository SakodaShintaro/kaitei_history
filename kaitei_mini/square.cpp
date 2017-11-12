#include"square.hpp"
#include<array>

ControlDir dirToOppositeJumpControl[LDD + DirOffset];
ControlDir dirToOppositeConDir[LDD + DirOffset];
ControlDir dirToControlDir[LDD + DirOffset];
std::vector<Dir> CanMove[WHITE_ROOK_PROMOTE + 1];
std::vector<Dir> CanJump[WHITE_ROOK_PROMOTE + 1];

const std::array<Square, 81> SquareList = {
    SQ11, SQ12, SQ13, SQ14, SQ15, SQ16, SQ17, SQ18, SQ19,
    SQ21, SQ22, SQ23, SQ24, SQ25, SQ26, SQ27, SQ28, SQ29,
    SQ31, SQ32, SQ33, SQ34, SQ35, SQ36, SQ37, SQ38, SQ39,
    SQ41, SQ42, SQ43, SQ44, SQ45, SQ46, SQ47, SQ48, SQ49,
    SQ51, SQ52, SQ53, SQ54, SQ55, SQ56, SQ57, SQ58, SQ59,
    SQ61, SQ62, SQ63, SQ64, SQ65, SQ66, SQ67, SQ68, SQ69,
    SQ71, SQ72, SQ73, SQ74, SQ75, SQ76, SQ77, SQ78, SQ79,
    SQ81, SQ82, SQ83, SQ84, SQ85, SQ86, SQ87, SQ88, SQ89,
    SQ91, SQ92, SQ93, SQ94, SQ95, SQ96, SQ97, SQ98, SQ99
};