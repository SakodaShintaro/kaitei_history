#include"search.hpp"
#include"move.hpp"
#include<iostream>
#include<stdio.h>
#include<string>
#include<algorithm>
#include<utility>
#include<functional>

int Searcher::NegaAlphaBeta(Position &pos, int alpha, int beta, int depth, int depth_max)
{
    //fprintf(stderr, "start NegaAlphaBeta(alpha = %d, beta = %d, depth = %d, depth_max = %d)\n", alpha, beta, depth, depth_max);

    //時間を確認
    if (isTimeOver()) return 0;

    //深さ最大まで来ていたら静止探索
    if (depth >= depth_max) return qsearch(pos, -INFINITE, INFINITE, depth);

    //探索局面数を増やす
    node_number_++;

    //指し手を全て生成 
    std::vector<Move> move_list = pos.generateAllMoves();

    //可能な指し手の数が0だったら負けの評価値を返す
    if (move_list.size() == 0) return -INFINITE;

    //局面表を探してヒットしたらmove_listの先頭と入れ替えたい
    if (hash_table_.find(pos.hash_val()) != NULL) {
        Move best_move = hash_table_.find(pos.hash_val())->best_move_;

        //指し手リストを探して一致したら先頭へ入れ替え
        for (unsigned int i = 0; i < move_list.size(); i++) {
            if (move_list[i] == best_move) {
                std::swap(move_list[i], move_list[0]);
                break;
            }
        }
    }

    //max_score を初期化
    int max_score = -INFINITE - 1;

    //全ての指し手について1手読みを深める
    for (unsigned int i = 0; i < move_list.size(); i++) {
        pos.doMove(move_list[i]);
        int v = -NegaAlphaBeta(pos, -beta, -alpha, depth + 1, depth_max);
        pos.undo();

        //時間を確認
        if (isTimeOver()) return 0;

        if (v > max_score) {
            max_score = v;

            best_moves_[depth][depth] = move_list[i];
            for (int j = depth + 1; j < DEPTH_MAX; j++) {
                best_moves_[depth][j] = best_moves_[depth + 1][j];
            }

            //fail-softのためにこんな冗長なのか
            if (max_score > alpha)
            {
                //置換表への保存
                hash_table_.save(pos.hash_val(), move_list[i], depth_max - depth);

                alpha = max_score;
                if (alpha >= beta) return alpha;
            }
        }
    }

    return max_score;
}

void Searcher::think(Position &root, long long limit_msec)
{
    //思考開始時間をセット
    start_ = std::chrono::steady_clock::now();

    //思考する局面の表示
    root.print();
    //root.printAllMoves();

    //制限時間をセット
    limit_msec_ = limit_msec - byoyomi_margin_;

    //合法手生成
    std::vector<Move> root_moves = root.generateAllMoves();

    //合法手が0だったら投了
    if (root_moves.size() == 0) {
        sendResign();
        return;
    }

    //合法手が1つだったらすぐ送る
    if (root_moves.size() == 1) {
        sendBestMove(root_moves[0]);
        std::cout << std::endl;
        return;
    }

    //指定された手数まで完全ランダムに指す
    std::random_device rd;
    if (root.turn_number() + 1 <= random_turn_) {
        int rnd = rd() % root_moves.size();
        std::cout << "random mode : " << rnd << std::endl;
        sendBestMove(root_moves[rnd]);
        std::cout << std::endl;
        return;
    }

    //探索局面数を初期化
    node_number_ = 0;

    //探索
    //反復深化
    for (int depth_max = 1; depth_max <= DEPTH_MAX; depth_max++) {
        int max_score = -INFINITE - 1;
        int alpha = -INFINITE;
        for (unsigned int j = 0; j < root_moves.size(); j++) {
            //時間を確認
            if (isTimeOver()) break;

            //1手進める
            root.doMove(root_moves[j]);
            root_moves[j].score = -NullWindowSearch(root, -alpha, 1, depth_max);
            root.undo();

            //時間を確認
            if (isTimeOver()) break;

            if (root_moves[j].score > max_score) {
                root.doMove(root_moves[j]);
                root_moves[j].score = -NegaAlphaBeta(root, -INFINITE, -alpha, 1, depth_max);
                root.undo();
                max_score = root_moves[j].score;

                //時間を確認
                //if (isTimeOver()) break;

                //置換表への保存
                hash_table_.save(root.hash_val(), root_moves[j], depth_max);

                best_moves_[0][0] = root_moves[j];
                for (int k = 1; k < depth_max; k++) best_moves_[0][k] = best_moves_[1][k];

                //alphaを更新
                alpha = std::max(alpha, max_score);
            }
        }

        //時間を見てオーバーしてたら保存せずループを抜ける
        if (isTimeOver()) break;

        //指し手の並び替え
        std::sort(root_moves.begin(), root_moves.end(), std::greater<Move>());

        //指し手の表示
        //for (unsigned int j = 0; j < root_moves.size(); j++) root_moves[j].printWithScore();

        //GUIへ読みの情報を送る
        sendInfo(root, depth_max, "cp", root_moves[0].score);

        //詰みがあったらすぐ返す
        if (root_moves[0].score == INFINITE || root_moves[0].score == -INFINITE) break;
    }

    //GUIへBestMoveの情報を送る
    sendBestMove(best_moves_[0][0]);
    std::cout << std::endl;
}

void Searcher::think_fixed_depth(Position & root, unsigned int fixed_depth)
{
    //制限時間をセット
    limit_msec_ = 99999999;

    //思考する局面の表示
    root.print();
    root.printAllMoves();

    //合法手生成
    std::vector<Move> root_moves = root.generateAllMoves();

    //合法手が0だったら投了
    if (root_moves.size() == 0) {
        sendResign();
        return;
    }

    //指定された手数まで完全ランダムに指す
    std::random_device rd;
    if (root.turn_number() + 1 <= random_turn_) {
        int rnd = rd() % root_moves.size();
        std::cout << "random mode : " << rnd << std::endl;
        sendBestMove(root_moves[rnd]);
        std::cout << std::endl;
        return;
    }

    //探索局面数を初期化
    node_number_ = 0;

    //指定された深さで探索
    int max_score = -INFINITE - 1;
    int alpha = -INFINITE;
    for (unsigned int j = 0; j < root_moves.size(); j++) {
        //1手進める
        root.doMove(root_moves[j]);
        root_moves[j].score = -NegaAlphaBeta(root, -INFINITE, -alpha, 1, fixed_depth);
        root.undo();
    }

    //指し手の並び替え
    std::sort(root_moves.begin(), root_moves.end(), std::greater<Move>());

    //指し手の表示
    //for (unsigned int j = 0; j < root_moves.size(); j++) root_moves[j].printWithScore();

    //GUIへ読みの情報を送る
    sendInfo(root, fixed_depth, "cp", root_moves[0].score);

    //GUIへBestMoveの情報を送る
    sendBestMove(root_moves[0]);
    std::cout << std::endl;

    //初期化してみる
    for (int i = 0; i < DEPTH_MAX; i++)
        for (int j = 0; j < DEPTH_MAX; j++)
            best_moves_[i][j] = NULL_MOVE;
}

int Searcher::qsearch(Position &pos, int alpha, int beta, int depth)
{
    //fprintf(stderr, "start qsearch(alpha = %d, beta = %d, depth = %d)\n", alpha, beta, depth);

    //時間を確認
    if (isTimeOver()) return 0;

    //駒を取りあう手のみについてNegaAlphaBetaみたいなことをすればいいんだと思う
    //単に駒を取る手を生成するととんでもない数になるっぽいので、直前に動いた駒を取れる場合だけ考えることにする(交換値)

    //探索局面数を増やす
    node_number_++;

    //直前に動いた駒を一番安い駒で取る手を生成する
    Move capture_move = pos.generateCheapestMoveTo((pos.lastMove().to()));

    //取らなかった場合の評価値
    int not_capture_score = (pos.color() == BLACK) ? pos.initScore() : -pos.initScore();

    if (capture_move.from() == WALL00) { //初期値だったら取る手がないということ
        return not_capture_score;
    } else { //取る手がある
        pos.doMove(capture_move);
        int v = -qsearch(pos, -beta, -alpha, depth + 1);
        pos.undo();
        if (v > not_capture_score) { //取らない手と比べて評価値が良ければ取る手を返す
            //局面表に取り合えず保存(PVを取るため)
            hash_table_.save(pos.hash_val(), capture_move, 0);
            return v;
        } else return not_capture_score;
    }
    return not_capture_score;
}

void Searcher::sendMove(const Move move)
{
    if (move.isDrop()) {
        std::cout << PieceToSfenStr[kind(move.subject())];
        printf("*");
        printf("%d%c ", SquareToFile[move.to()], 'a' - 1 + SquareToRank[move.to()]);
    } else {
        printf("%d%c", SquareToFile[move.from()], 'a' - 1 + SquareToRank[move.from()]);
        printf("%d%c", SquareToFile[move.to()], 'a' - 1 + SquareToRank[move.to()]);
        if (move.isPromote()) {
            printf("+ ");
        } else {
            printf(" ");
        }
    }
}

void Searcher::sendInfo(long long time, int depth, int seldepth, long long node_number, std::string cp_or_mate, int score, std::vector<Move> pv)
{
    //GUIへ読み途中の情報を返す
    std::cout << "info time " << time << " ";
    std::cout << "depth " << depth << " ";
    std::cout << "seldepth " << seldepth << " ";
    std::cout << "nodes " << node_number << " ";
    std::cout << "score " << cp_or_mate << " " << score << " ";
    std::cout << "pv ";
    sendPV(pv);
    std::cout << std::endl;
}

void Searcher::sendInfo(Position k, int depth, std::string cp_or_mate, int score)
{
    //GUIへ読み途中の情報を返す
    //引数ができるだけ少なくなるようにこっちでいろいろやるバージョン
    //時間
    auto now_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now_time - start_);

    //PV
    std::vector<Move> pv = makePV(k);

    std::cout << "info time " << elapsed.count() << " ";
    std::cout << "depth " << depth << " ";
    std::cout << "seldepth " << pv.size() << " ";
    std::cout << "nodes " << node_number_ << " ";
    std::cout << "score " << cp_or_mate << " " << score << " ";
    std::cout << "pv ";
    sendPV(pv);
    std::cout << std::endl;
    std::cout << "info nps " << static_cast<double>(node_number_) / elapsed.count() * 1000.0 << "\n";
    std::cout << "info hashfull " << hash_table_.hashfull() << "\n";
}

std::vector<Move> Searcher::makePV(Position pos)
{
    std::vector<Move> pv;
    for (int depth = 0; depth < DEPTH_MAX; depth++) {
        if (hash_table_.find(pos.hash_val()) == NULL) break;
        Move best_move = hash_table_.find(pos.hash_val())->best_move_;
        if (!pos.isLegalMove(best_move)) break;
        pv.push_back(best_move);
        pos.doMove(best_move);
    }
    return pv;
}

bool Searcher::isTimeOver()
{
    auto now_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now_time - start_);
    return (elapsed.count() >= limit_msec_);
}

std::vector<Move> Searcher::pv()
{
    std::vector<Move> pv;
    for (int i = 0; i < DEPTH_MAX && !(best_moves_[0][i] == NULL_MOVE); i++) {
        pv.push_back(best_moves_[0][i]);
    }
    //初期化してみる
    for (int i = 0; i < DEPTH_MAX; i++)
        for (int j = 0; j < DEPTH_MAX; j++)
            best_moves_[i][j] = NULL_MOVE;
    return pv;
}