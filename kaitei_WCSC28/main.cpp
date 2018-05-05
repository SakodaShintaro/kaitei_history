#include"usi.hpp"
#include"piece_state.hpp"
#include"usi_options.hpp"
#include"test.hpp"
#include"bitboard.hpp"

std::unique_ptr<EvalParams<int>> eval_params(new EvalParams<int>);

int main()
{
	initPieceToStateIndex();
	initInvPieceState();

	initCanMove();
	initCanJump();

    initConDirToOppositeDir();

    Bitboard::init();

	USI usi;
	usi.loop();
}