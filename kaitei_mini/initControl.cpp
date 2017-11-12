#include"position.hpp"
#include"square.hpp"
#include<unordered_map>

void Position::initControl() {
	for (auto sq : SquareList) {
		control_dir_[BLACK][sq] = 0;
		control_dir_[WHITE][sq] = 0;
		control_num_[BLACK][sq] = 0;
		control_num_[WHITE][sq] = 0;
	}
	for (auto sq : SquareList) {
		Piece subject = board_[sq];
		//直接利きを足す(桂馬含む)
        for (auto dir : CanMove[subject]) {
            control_dir_[pieceToColor(subject)][sq + dir] |= DirToOppositeConDir(dir);
            control_num_[pieceToColor(subject)][sq + dir]++;
        }
		//飛び利きを足す
        for (auto dir : CanJump[subject]) {
            addJumpControl(pieceToColor(subject), sq, DirToOppositeJumpControl(dir));
        }
	}

    //評価値
    control_num_score_ = Score(0);
    for (Square sq : SquareList) {
        control_num_score_ += control_num_[BLACK][sq];
        control_num_score_ -= control_num_[WHITE][sq];
    }
}

void Position::deleteControl(const Square sq) {
	//sqにある駒を取り除いたような利きに更新する(方向と数の両方)

	//必要かどうかわからないけど一応EMPTYか確認しておく
    assert(board_[sq] != EMPTY);

	//sqにある駒の利きを消す
	Piece subject = board_[sq];
	//直接利きを消す(桂馬含む)
    for (auto dir : CanMove[subject]) {
        control_dir_[pieceToColor(subject)][sq + dir] &= ~DirToOppositeConDir(dir);
        control_num_[pieceToColor(subject)][sq + dir]--;
        if (board_[sq + dir] != WALL)
            control_num_score_ -= (pieceToColor(subject) == BLACK ? 1 : -1);
    }
	//飛び利きを消す
    for (auto toward : CanJump[subject]) {
        Color BorW = pieceToColor(subject);
        auto flag = DirToOppositeJumpControl(toward);
        //towardの方向にマスを辿っていって飛び利きを消していく
        for (Square to_jump = sq + toward; board_[to_jump] != WALL; to_jump = to_jump + toward) {
            control_dir_[BorW][to_jump] &= ~flag;
            control_num_[BorW][to_jump]--;
            control_num_score_ -= (BorW == BLACK ? 1 : -1);
            if (board_[to_jump] != EMPTY) { //初めて駒に突き当たったらそこから先の影の利きを立てていく
                for (Square to_shadow = to_jump + toward; board_[to_shadow] != WALL; to_shadow = to_shadow + toward) {
                    control_dir_[BorW][to_shadow] &= ~JumpControlToShadowControl(flag);
                    if (board_[to_shadow] != EMPTY) break;
                }
                break;
            }
        }
    }

	//sqの利きを見て飛び利きと影の利きを伸ばしていく
    for (int i = 0; i < 8; ++i) { //周囲8方向について順番に処理
        auto jump_dir = JumpControlList[i];
        auto shadow_dir = ShadowControlList[i];
        const Dir toward = ConDirToOppositeDir(jump_dir);
        for (int c = BLACK; c <= WHITE; ++c) {
            //飛び利き
            if (control_dir_[c][sq] & jump_dir) {
                //sqの利きを見て飛び利きがあったらその先に飛び利きを立てていく
                for (Square to_jump = sq + toward; board_[to_jump] != WALL; to_jump = to_jump + toward) {
                    control_dir_[c][to_jump] |= jump_dir;
                    control_num_[c][to_jump]++;
                    control_num_score_ += (c == BLACK ? 1 : -1);
                    if (board_[to_jump] != EMPTY) { //初めて駒に突き当ったらその先に影の利きを立てていく	
                        for (Square to_shadow = to_jump + toward; board_[to_shadow] != WALL; to_shadow = to_shadow + toward) {
                            control_dir_[c][to_shadow] |= JumpControlToShadowControl(jump_dir);
                            if (board_[to_shadow] != EMPTY) break;
                        }
                        break;
                    }
                }
            }

            //影の利き
            if (control_dir_[c][sq] & shadow_dir) {
                //見てるマスから1マスずつ辿って行く
                for (Square to = sq + toward; board_[to] != WALL; to = to + toward) {
                    control_dir_[c][to] |= shadow_dir;
                    if (board_[to] != EMPTY) break;
                }
            }
        }
    }
}

void Position::addControl(const Square sq) {
	//sqに駒がない状態から置いた状態になるように利きを更新する

	//必要かわからないけど一応EMPTYかどうか確認しておく
    assert(board_[sq] != EMPTY);

	//sqの利きを見て、飛び利きがあったらその先の飛び利きを消していく
	//先に影の利きを消してから飛び利きを消さないと、重ねて飛び駒を打った時影の利きが消えてしまう
    //↑何を言っているのか読み取れない.本当か？
    for (int i = 0; i < 8; ++i) {
        auto shadow_dir = ShadowControlList[i];
        auto jump_dir = JumpControlList[i];
        for (int c = BLACK; c <= WHITE; ++c) {
            //マスをたどっていく方向(利きが来ている方向と逆)
            const Dir toward = ConDirToOppositeDir(jump_dir);

            //影の利き
            if (control_dir_[c][sq] & shadow_dir) {
                //見てるマスから1マスずつ辿って行く
                for (Square to = sq + toward; board_[to] != WALL; to = to + toward) {
                    control_dir_[c][to] &= ~shadow_dir;
                    if (board_[to] != EMPTY) break;
                }
            }

            //飛び利き
            //sqの利きを見て飛び利きがあったらその先の飛び利きを消していき、影の利きを立てていく
            if (control_dir_[c][sq] & jump_dir) {
                for (Square to_jump = sq + toward; board_[to_jump] != WALL; to_jump = to_jump + toward) {
                    control_dir_[c][to_jump] &= ~jump_dir;
                    control_dir_[c][to_jump] |= JumpControlToShadowControl(jump_dir);
                    control_num_[c][to_jump]--;
                    control_num_score_ -= (c == BLACK ? 1 : -1);
                    if (board_[to_jump] != EMPTY) { //初めて駒に突き当ったらその先の影の利きを消していく
                        for (Square to_shadow = to_jump + toward; board_[to_shadow] != WALL; to_shadow = to_shadow + toward) {
                            control_dir_[c][to_shadow] &= ~JumpControlToShadowControl(jump_dir);
                            if (board_[to_shadow] != EMPTY) break;
                        }
                        break;
                    }
                }
            }
        }
    }

	//sqにある駒の利きを足す
	Piece subject = board_[sq];
	//直接利きを足す(桂馬含む)
    for (auto dir : CanMove[subject]) {
        control_dir_[pieceToColor(subject)][sq + dir] |= DirToOppositeConDir(dir);
        control_num_[pieceToColor(subject)][sq + dir]++;
        if (board_[sq + dir] != WALL)
            control_num_score_ += (pieceToColor(subject) == BLACK ? 1 : -1);
    }
	//飛び利きを足す
    for (auto toward : CanJump[subject]) {
        //addJumpControl(pieceToColor(subject), sq, DirToOppositeJumpControl(dir));
        //dirと逆方向にマスを辿っていって飛び利きを立てていく
        auto flag = DirToOppositeJumpControl(toward);
        Color BorW = pieceToColor(subject);
        for (Square to_jump = sq + toward; board_[to_jump] != WALL; to_jump = to_jump + toward) {
            control_dir_[BorW][to_jump] |= flag;
            control_num_[BorW][to_jump]++;
            control_num_score_ += (BorW == BLACK ? 1 : -1);
            if (board_[to_jump] != EMPTY) { //初めて駒に突き当たったらそこから先の影の利きを立てていく
                for (Square to_shadow = to_jump + toward; board_[to_shadow] != WALL; to_shadow = to_shadow + toward) {
                    control_dir_[BorW][to_shadow] |= JumpControlToShadowControl(flag);
                    if (board_[to_shadow] != EMPTY) break;
                }
                break;
            }
        }
    }
}

inline void Position::addJumpControl(const Color BorW, const Square sq, const ControlDir dir) {
	//dirと逆方向にマスを辿っていって飛び利きを立てていく
    const Dir toward = ConDirToOppositeDir(dir);
	for (Square to_jump = sq + toward; board_[to_jump] != WALL; to_jump = to_jump + toward) {
		control_dir_[BorW][to_jump] |= dir;
		control_num_[BorW][to_jump]++;
        control_num_score_ += (BorW == BLACK ? 1 : -1);
        if (board_[to_jump] != EMPTY) { //初めて駒に突き当たったらそこから先の影の利きを立てていく
			for (Square to_shadow = to_jump + toward; board_[to_shadow] != WALL; to_shadow = to_shadow + toward) {
				control_dir_[BorW][to_shadow] |= JumpControlToShadowControl(dir);
				if (board_[to_shadow] != EMPTY) break;
			}
			break;
		}
	}
}