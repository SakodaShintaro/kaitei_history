#include"usi.hpp"
#include"usi_options.hpp"

int main()
{
	initCanMove();
	initCanJump();

    initDirToControlDir();
    initDirToOppositeJumpControl();
    initDirToOppositeConDir();

	USI usi;
	usi.loop();
}