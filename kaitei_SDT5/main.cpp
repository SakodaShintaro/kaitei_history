#include"usi.hpp"
#include"piece_state.hpp"
#include"usi_options.hpp"
#include"bitboard.hpp"
#include"eval_params.hpp"

EvalParams<int> eval_params;

int main()
{
	initPieceToStateIndex();
	initInvPieceState();

	initCanMove();
	initCanJump();

    initDirToControlDir();
    initDirToOppositeJumpControl();
    initDirToOppositeConDir();

    Bitboard::init();

	USI usi;
	usi.loop();
}