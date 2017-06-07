#include"position.hpp"
#include"square.hpp"
#include<map>

void Position::initControl()
{
    for (Square sq : SquareList) {
        control_dir_[BLACK][sq] = 0;
        control_dir_[WHITE][sq] = 0;
        control_num_[BLACK][sq] = 0;
        control_num_[WHITE][sq] = 0;
    }
    for (Square sq : SquareList) {
        Piece subject = board_[sq];
        //直接利きを足す(桂馬含む)
        for (unsigned int i = 0; i < CanMove[subject].size(); i++) {
            control_dir_[pieceToColor(subject)][sq + CanMove[subject][i]] |= DirToOppositeConDir[CanMove[subject][i]];
            control_num_[pieceToColor(subject)][sq + CanMove[subject][i]]++;
        }
        //飛び利きを足す
        for (unsigned int i = 0; i < CanJump[subject].size(); i++) {
            addJumpControl(pieceToColor(subject), sq, DirToOppositeJumpControl[CanJump[subject][i]]);
        }
    }
}

void Position::deleteControl(Square sq)
{
    //sqにある駒を取り除いたような利きに更新する(方向と数の両方)

    //必要かどうかわからないけど一応EMPTYか確認しておく
    if (board_[sq] == EMPTY) return;

    //sqにある駒の利きを消す
    Piece subject = board_[sq];
    //直接利きを消す(桂馬含む)
    for (unsigned int i = 0; i < CanMove[subject].size(); i++) {
        control_dir_[pieceToColor(subject)][sq + CanMove[subject][i]] &= ~DirToOppositeConDir[CanMove[subject][i]];
        control_num_[pieceToColor(subject)][sq + CanMove[subject][i]]--;
    }
    //飛び利きを消す
    for (unsigned int i = 0; i < CanJump[subject].size(); i++) {
        removeJumpControl(pieceToColor(subject), sq, DirToOppositeJumpControl[CanJump[subject][i]]);
    }

    //sqの利きを見て、飛び利きがあったらその先に飛び利きを伸ばしていく
    for (int i = 0; i < 8; i++) {
        extendJumpControl(BLACK, sq, JumpControlList[i]);
        extendJumpControl(WHITE, sq, JumpControlList[i]);
    }

    //sqの利きを見て、影の利きがあったらその先に影の利きを伸ばしていく
    for (int i = 0; i < 8; i++) {
        extendShadowControl(BLACK, sq, ShadowControlList[i]);
        extendShadowControl(WHITE, sq, ShadowControlList[i]);
    }
}

void Position::addControl(Square pos)
{
    //sqに駒がない状態から置いた状態になるように利きを更新する

    //必要かわからないけど一応EMPTYかどうか確認しておく
    if (board_[pos] == EMPTY) return;

    //sqの利きを見て、飛び利きがあったらその先の飛び利きを消していく
    //先に影の利きを消してから飛び利きを消さないと、重ねて飛び駒を打った時影の利きが消えてしまう
    for (int i = 0; i < 8; i++) {
        blockShadowControl(BLACK, pos, ShadowControlList[i]);
        blockShadowControl(WHITE, pos, ShadowControlList[i]);
    }

    for (int i = 0; i < 8; i++) {
        blockJumpControl(BLACK, pos, JumpControlList[i]);
        blockJumpControl(WHITE, pos, JumpControlList[i]);
    }

    //sqにある駒の利きを足す
    Piece subject = board_[pos];
    //直接利きを足す(桂馬含む)
    for (unsigned int i = 0; i < CanMove[subject].size(); i++) {
        control_dir_[pieceToColor(subject)][pos + CanMove[subject][i]] |= DirToOppositeConDir[CanMove[subject][i]];
        control_num_[pieceToColor(subject)][pos + CanMove[subject][i]]++;
    }
    //飛び利きを足す
    for (unsigned int i = 0; i < CanJump[subject].size(); i++) {
        addJumpControl(pieceToColor(subject), pos, DirToOppositeJumpControl[CanJump[subject][i]]);
    }
}

inline void Position::blockJumpControl(Color BorW, Square sq, ControlDir dir)
{
    //sqの利きを見て飛び利きがあったらその先の飛び利きを消していき、影の利きを立てていく
    if (control_dir_[BorW][sq] & dir) {
        for (Square i = sq + ConDirToOppositeDir[dir]; board_[i] != WALL; i = i + ConDirToOppositeDir[dir]) {
            control_dir_[BorW][i] &= ~dir;
            control_dir_[BorW][i] |= JumpControlToShadowControl[dir];
            control_num_[BorW][i]--;
            if (board_[i] != EMPTY) { //初めて駒に突き当ったらその先の影の利きを消していく
                for (Square j = i + ConDirToOppositeDir[dir]; board_[j] != WALL; j = j + ConDirToOppositeDir[dir]) {
                    control_dir_[BorW][j] &= ~JumpControlToShadowControl[dir];
                    if (board_[j] != EMPTY) break;
                }
                break;
            }
        }
    }
}

inline void Position::blockShadowControl(Color BorW, Square sq, ControlDir dir)
{
    //sqの利きを見て影の飛び利きがあったらその先の影の飛び利きを消していく
    if (control_dir_[BorW][sq] & dir) {
        for (Square i = sq + ConDirToOppositeDir[dir]; board_[i] != WALL; i = i + ConDirToOppositeDir[dir]) {
            control_dir_[BorW][i] &= ~dir;
            if (board_[i] != EMPTY) break;
        }
    }
}

inline void Position::extendJumpControl(Color BorW, Square sq, ControlDir dir)
{
    //sqの利きを見て飛び利きがあったらその先に飛び利きを立てていく
    if (control_dir_[BorW][sq] & dir) {
        for (Square i = sq + ConDirToOppositeDir[dir]; board_[i] != WALL; i = i + ConDirToOppositeDir[dir]) {
            control_dir_[BorW][i] |= dir;
            control_num_[BorW][i]++;
            if (board_[i] != EMPTY) { //初めて駒に突き当ったらその先に影の利きを立てていく	
                for (Square j = i + ConDirToOppositeDir[dir]; board_[j] != WALL; j = j + ConDirToOppositeDir[dir]) {
                    control_dir_[BorW][j] |= JumpControlToShadowControl[dir];
                    if (board_[j] != EMPTY) break;
                }
                break;
            }
        }
    }
}

inline void Position::extendShadowControl(Color BorW, Square sq, ControlDir dir)
{
    //sqの利きを見て影の飛び利きがあったらその先に影の飛び利きを立てていく
    if (control_dir_[BorW][sq] & dir) {
        for (Square i = sq + ConDirToOppositeDir[dir]; board_[i] != WALL; i = i + ConDirToOppositeDir[dir]) {
            control_dir_[BorW][i] |= dir;
            if (board_[i] != EMPTY) break;
        }
    }
}

inline void Position::addJumpControl(Color BorW, Square sq, ControlDir dir) {
    //dirと逆方向にマスを辿っていって飛び利きを立てていく
    for (Square i = sq + ConDirToOppositeDir[dir]; board_[i] != WALL; i = i + ConDirToOppositeDir[dir]) {
        control_dir_[BorW][i] |= dir;
        control_num_[BorW][i]++;
        if (board_[i] != EMPTY) { //初めて駒に突き当たったらそこから先の影の利きを立てていく
            for (Square j = i + ConDirToOppositeDir[dir]; board_[j] != WALL; j = j + ConDirToOppositeDir[dir]) {
                control_dir_[BorW][j] |= JumpControlToShadowControl[dir];
                if (board_[j] != EMPTY) break;
            }
            break;
        }
    }
}

inline void Position::removeJumpControl(Color BorW, Square sq, ControlDir dir) {
    //dirと逆方向にマスを辿っていって飛び利きを消していく
    for (Square i = sq + ConDirToOppositeDir[dir]; board_[i] != WALL; i = i + ConDirToOppositeDir[dir]) {
        control_dir_[BorW][i] &= ~dir;
        control_num_[BorW][i]--;
        if (board_[i] != EMPTY) { //初めて駒に突き当たったらそこから先の影の利きを立てていく
            for (Square j = i + ConDirToOppositeDir[dir]; board_[j] != WALL; j = j + ConDirToOppositeDir[dir]) {
                control_dir_[BorW][j] &= ~JumpControlToShadowControl[dir];
                if (board_[j] != EMPTY) break;
            }
            break;
        }
    }
}