#include"searcher.hpp"
#include"move.hpp"
#include"move_picker.hpp"
#include"usi_options.hpp"
#include"types.hpp"
#include"shared_data.hpp"
#include<iostream>
#include<stdio.h>
#include<string>
#include<algorithm>
#include<utility>
#include<functional>

extern USIOption usi_option;

static const Score DEFAULT_ASPIRATION_WINDOW_SIZE = Score(512);

void Searcher::think() {
    //思考開始時間をセット
    start_ = std::chrono::steady_clock::now();

    //コピーして使う
    Position root = shared_data.root;

    //思考する局面の表示
    if (role_ == MAIN) {
        root.print();
    }

    //History初期化
    history_.clear();

    resetPVTable();

    //ルート局面の合法手を設定
    root_moves_ = root.generateAllMoves();

    SearchStack* ss = searchInfoAt(0);
    ss->killers[0] = ss->killers[1] = NULL_MOVE;

    //合法手が0だったら投了
    if (root_moves_.size() == 0) {
        if (role_ == MAIN) {
            std::cout << "bestmove resign" << std::endl;
        }
        return;
    }

    //合法手が1つだったらすぐ送る
    if (root_moves_.size() == 1) {
        if (role_ == MAIN) {
            std::cout << "bestmove " << root_moves_[0] << std::endl;
        }
        return;
    }

    //指定された手数まで完全ランダムに指す
    std::random_device rd;
    if (root.turn_number() + 1 <= usi_option.random_turn) {
        if (role_ == MAIN) {
            int rnd = rd() % root_moves_.size();
            std::cout << "random mode : " << rnd << std::endl;
            std::cout << "bestmove " << root_moves_[rnd] << std::endl;
        }
        return;
    }

    //探索局面数を初期化
    node_number_ = 0;

    //探索
    //反復深化
    static Score DEFAULT_ASPIRATION_WINDOW_SIZE = Score(512);
    Score aspiration_window_size = DEFAULT_ASPIRATION_WINDOW_SIZE;
    Score best_score, alpha, beta, old_alpha, previous_best_score;

    for (Depth depth = Depth(1); depth <= DEPTH_MAX; ++depth) {
        if (role_ == SLAVE) { //Svaleスレッドは探索深さを深くする
            std::random_device rnd;
            std::mt19937 mt(rnd());
            std::uniform_int_distribution<> rand1to3(1, 3);
            depth += rand1to3(mt);
        }

        //seldepth_の初期化
        seldepth_ = depth;

        //探索窓の設定
        if (depth <= 4) { //深さ4まではASPIRATION_WINDOWを使わずフルで探索する
            old_alpha = alpha = MIN_SCORE;
            beta = MAX_SCORE;
        } else {
            old_alpha = alpha = std::max(previous_best_score - aspiration_window_size, MIN_SCORE);
            beta = std::min(previous_best_score + aspiration_window_size, MAX_SCORE);
        }

        while (true) { //exactな評価値が返ってくるまでウィンドウを広げつつ探索
            //指し手のスコアを最小にしておかないと変になる
            for (auto& root_move : root_moves_) {
                root_move.score = MIN_SCORE;
            }


            //停止確認
            if (shouldStop())
                break;

            best_score = search<true>(root, alpha, beta, depth, 0);
            //best_score = NegaAlphaBeta(root, alpha, beta, depth, 0);

            //停止確認
            if (shouldStop())
                break;

            //詰んでいたら抜ける
            if (isMatedScore(best_score))
                break;

            if (best_score <= old_alpha) {
                //fail-low
                if (role_ == MAIN) {
                    printf("aspiration fail-low, alpha = %d\n", old_alpha);
                    sendInfo(depth, "cp", best_score, UPPER_BOUND);
                }

                beta = (alpha + beta) / 2;
                old_alpha = alpha = old_alpha - aspiration_window_size;
                aspiration_window_size *= 2;
            } else if (best_score >= beta) {
                //fail-high
                if (role_ == MAIN) {
                    printf("aspiration fail-high, beta = %d\n", beta);
                    sendInfo(depth, "cp", best_score, LOWER_BOUND);
                }

                old_alpha = alpha = (alpha + beta) / 2;
                beta = beta + aspiration_window_size;
                aspiration_window_size *= 2;
            } else {
                aspiration_window_size = DEFAULT_ASPIRATION_WINDOW_SIZE;
                break;
            }
        }
        //停止確認してダメだったら保存せずループを抜ける
        if (shouldStop())
            break;

        //指し手の並び替え
        std::stable_sort(root_moves_.begin(), root_moves_.end(), std::greater<Move>());

        //GUIへ読みの情報を送る
        if (role_ == MAIN) {
            if (MATE_SCORE_UPPER_BOUND < root_moves_[0].score && root_moves_[0].score < MATE_SCORE_LOWER_BOUND) {
                //詰みなし
                sendInfo(depth, "cp", root_moves_[0].score, EXACT_BOUND);
            } else {
                //詰みあり
                Score mate_num = MAX_SCORE - std::abs(root_moves_[0].score);
                mate_num *= (mate_num % 2 == 0 ? -1 : 1);
                sendInfo(depth, "mate", mate_num, EXACT_BOUND);
            }
        }

        //置換表への保存
        shared_data.mutex.lock();
        shared_data.hash_table.save(root.hash_value(), root_moves_[0], root_moves_[0].score, depth);
        shared_data.mutex.unlock();

        //詰みがあったらすぐ返す
        if (isMatedScore(root_moves_[0].score))
            break;

        //今回のイテレーションにおけるスコアを記録
        previous_best_score = best_score;

        //PVtableをリセットしていいよな？
        resetPVTable();
    }

    //GUIへBestMoveの情報を送る
    if (role_ == MAIN) {
        std::cout << "bestmove " << root_moves_[0] << std::endl;
    }
}

template<bool isPVNode>
Score Searcher::qsearch(Position &pos, Score alpha, Score beta, Depth depth, int distance_from_root) {

    //連続王手の千日手風になったときは無限ループになるので適当なところで抜ける
    //こんなやりかたはダサすぎる気がするけど……
    if (distance_from_root >= DEPTH_MAX) {
        assert(false);
        return (pos.color() == BLACK ? pos.score() : -pos.score());
    }

    pv_table_.closePV(distance_from_root);

    assert(isPVNode || alpha + 1 == beta);
    assert(0 <= distance_from_root && distance_from_root <= DEPTH_MAX);

    //変数宣言
    Move best_move = NULL_MOVE;
    Score old_alpha = alpha;

    SearchStack* ss = searchInfoAt(distance_from_root);

    //探索局面数を増やす
    node_number_++;

    //seldepthはいろんな局面の静止探索の最大値になっている
    seldepth_ = std::max(seldepth_, static_cast<Depth>(distance_from_root));

    //停止確認をここでやる必要があるのかはわからない
    if (shouldStop()) return SCORE_ZERO;

    //置換表を見る
    Move tt_move = NULL_MOVE;
    {
        Score tt_score;
        Depth tt_depth;
        bool tt_hit;

        shared_data.mutex.lock();
        if ((tt_hit = shared_data.hash_table.find(pos.hash_value()) != nullptr)) {
            tt_score = shared_data.hash_table.find(pos.hash_value())->score_;
            tt_move = shared_data.hash_table.find(pos.hash_value())->best_move_;
            tt_depth = shared_data.hash_table.find(pos.hash_value())->depth_;
        }
        shared_data.mutex.unlock();

        //置換表の枝刈り
        if (!pos.isKingChecked(pos.color())) { //この条件いる？
            //置換表での枝刈り
            if (!isPVNode
                && tt_hit
                && tt_depth >= depth
                && tt_score >= beta) {
                return tt_score;
            }
        }
    }

    //指し手を生成 
    MovePicker mp(pos, tt_move, depth, history_);
    Score best_score = (pos.isKingChecked(pos.color()) ? MIN_SCORE + distance_from_root //王手がかかっていたら最低点から始める
        : pos.color() == BLACK ? pos.score() : -pos.score());                            //そうでなかったら評価関数を呼び出した値から始める
    int move_count = 0;
    for (Move current_move = mp.nextMove(); current_move != NULL_MOVE; current_move = mp.nextMove()) {
        //停止確認
        if (shouldStop())
            return Score(0);

        //Null Window Search
        //qsearchではしない方がいい？ 暫定的に外してみる
        pos.doMove(current_move);
        Score score = -qsearch<isPVNode>(pos, -beta, -alpha, depth - 1, distance_from_root + 1);

        if (score > best_score) {
            best_score = score;
            best_move = current_move;

            if (best_score >= beta) {
                //fail-high
                pos.undo();
                if (best_move.isQuiet()) {
                    //静止探索でQuietな指し手になるわけがなくない？
                    history_.updateBetaCutMove(best_move, depth);
                    ss->updateKillers(best_move);
                }
                //fail-highとして置換表にsaveするべき
                return best_score; //betaカット
            }

            pv_table_.update(best_move, distance_from_root);

            alpha = std::max(alpha, best_score);
        }

        pos.undo();

        ++move_count;
    }

    //ループを抜けるのは、beta-cutが起きず
    //(1)best_scoreは更新されたとき
    //(2)best_scoreが更新されないとき(fail-low,詰みも含む)

    if (best_move == NULL_MOVE) {
        //(2)のパターン
        //なにかすべきことはあるかな
        //詰みの値を返す？→best_scoreは詰みの値で初期化されてるのでなにもしなくてもそうなってる
    } else {
        //(1)のパターン
        //置換表への保存  todo:old_alphaと比較して値の信頼性を付け加えるべき
        shared_data.mutex.lock();
        shared_data.hash_table.save(pos.hash_value(), best_move, best_score, Depth(0));
        shared_data.mutex.unlock();
    }

    return best_score;
}

void Searcher::sendInfo(Depth depth, std::string cp_or_mate, Score score, Bound bound) {
    auto now_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now_time - start_);
    std::cout << "info time " << elapsed.count() << " ";

    //GUIへ読み途中の情報を返す
    std::cout << "depth " << depth << " ";
    std::cout << "seldepth " << seldepth_ << " ";
    std::cout << "nodes " << node_number_ << " ";
    std::cout << "score " << cp_or_mate << " " << score << " ";
    if (bound == UPPER_BOUND) std::cout << "upperbound ";
    else if (bound == LOWER_BOUND) std::cout << "lowerbound ";
    std::cout << "pv ";
    if (pv_table_.size() == 0) {
        pv_table_.update(root_moves_[0], 0);
    }
    for (auto move : pv_table_) {
        std::cout << move << " ";
    }
    std::cout << std::endl;
    std::cout << "info nps " << static_cast<double>(node_number_) / elapsed.count() * 1000.0 << "\n";
    std::cout << "info hashfull " << shared_data.hash_table.hashfull() << "\n";
}

#define OMIT_PRUNINGS

template<bool isPVNode>
Score Searcher::search(Position &pos, Score alpha, Score beta, Depth depth, int distance_from_root) {
    // nodeの種類
    bool isRootNode = (distance_from_root == 0);

    //深さ最大まで来ていたら静止探索する
    if (depth <= 0) return qsearch<isPVNode>(pos, alpha, beta, Depth(0), distance_from_root);

    //assert類
    assert(isPVNode || alpha + 1 == beta);
    assert(MIN_SCORE <= alpha && alpha < beta && beta <= MAX_SCORE);

    //探索局面数を増やす
    ++node_number_;

    if (isPVNode) {
        seldepth_ = std::max(seldepth_, static_cast<Depth>(distance_from_root));
    }

    if (!isRootNode) {
        //停止確認
        if (shouldStop())
            return SCORE_ZERO;

        //引き分けの確認
        //現局面が4手前と一致し、現局面と2手前が王手かどうか見ているだけ
        //これでうまくいっているか？
        Score repeate_score;
        if (pos.isRepeating(repeate_score)) {
            pv_table_.closePV(distance_from_root);
            return repeate_score;
        }
    }

    // SearchStackの初期化
    SearchStack* ss = searchInfoAt(distance_from_root);
    (ss + 1)->killers[0] = (ss + 1)->killers[1] = NULL_MOVE;

    //置換表を見る
    Score tt_score;
    Move tt_move = NULL_MOVE;
    Depth tt_depth;
    bool tt_hit;

    shared_data.mutex.lock();
    if ((tt_hit = shared_data.hash_table.find(pos.hash_value()) != nullptr)) {
        tt_score = shared_data.hash_table.find(pos.hash_value())->score_;
        tt_move = shared_data.hash_table.find(pos.hash_value())->best_move_;
        if (tt_move != NULL_MOVE && !pos.isLegalMove(tt_move)) {
            tt_move = NULL_MOVE;
        }
        tt_depth = shared_data.hash_table.find(pos.hash_value())->depth_;
    }
    shared_data.mutex.unlock();

    //置換表の値による枝刈り
    if (!isPVNode
        && tt_hit
        && tt_depth >= depth
        && tt_score >= beta) {

        //tt_moveがちゃんとしたMoveならこれでHistory更新するべきか
        if (tt_move != NULL_MOVE) {
            history_.updateBetaCutMove(tt_move, depth);
        }

        return tt_score;
    }

    //局面の静的評価
    Score static_score = (tt_hit ? tt_score : (pos.color() == BLACK ? pos.score() : -pos.score()));

    //王手がかかっているときは下の枝刈りはしない
    if (pos.isKingChecked(pos.color()))
        goto moves_loop;

    //Razoring
    if (!isPVNode
        && depth < 4
        && static_score + razorMargin(depth) <= alpha) {
        if (depth <= 1)
            return qsearch<isPVNode>(pos, alpha, beta, Depth(0), distance_from_root);

        Score ralpha = alpha - razorMargin(depth);
        Score v = qsearch<isPVNode>(pos, ralpha, ralpha + 1, Depth(0), distance_from_root);
        if (v <= ralpha)
            return v;
    }

    //Futility pruning
    //子ノードでのっていうのはどういうことなんだろう
    //上のRazoringと対応関係にあると思うんだけど、マージンとかはそろえるべきなのか
    if (depth < 7 && static_score - depthMargin(depth) >= beta) {
        return static_score;
    }

moves_loop:

    std::vector<Move> quiet_moves;
    quiet_moves.reserve(600);

    int move_count = 0;

    //指し手を生成 
    MovePicker mp(pos, tt_move, depth, history_, ss->killers);
    Score best_score = MIN_SCORE + distance_from_root;
    Move best_move = NULL_MOVE;

    //1手ずつみていく
    for (Move current_move = mp.nextMove(); current_move != NULL_MOVE; current_move = mp.nextMove()) {
        //ルートノードでしか参照されない
        std::vector<Move>::iterator root_move_itr;

        if (isRootNode) {
            root_move_itr = std::find(root_moves_.begin(), root_moves_.end(), current_move);
            if (root_move_itr == root_moves_.end()) {
                //root_moves_に存在しなかったらおかしいので次の手へ
                continue;
            }
        }

        ++move_count;

        //合法性判定は必要かどうか
        //今のところ合法手しかこないはずだけど
        pos.doMove(current_move);

        if (isPVNode) {
            pv_table_.closePV(distance_from_root + 1);
        }

        Score score;
        //Null Window Searchでalphaを超えそうか確認
        //これ入れた方がいいのかなぁ
        score = -search<false>(pos, -alpha - 1, -alpha, depth - 1, distance_from_root + 1);

        if (alpha < score && score < beta) {
            //isPVNodeって条件いるかな
            //nonPVならalpha + 1 == betaとなっているはずだからいらない気がする

            //いい感じのスコアだったので再探索
            score = -search<isPVNode>(pos, -beta, -alpha, depth - 1, distance_from_root + 1);
        }
       
        pos.undo();

        //停止確認
        if (shouldStop())
            return Score(0);

        // 探索された値によってalphaなどを更新
        if (score > best_score) {
            if (isRootNode) {
                //ルートノードならスコアを更新しておく
                root_move_itr->score = score;
            }

            best_score = score;
            if (score >= beta) {
                //fail-high
                best_move = current_move;

                pv_table_.update(current_move, distance_from_root);
                if (current_move.isQuiet()) {
                    history_.updateBetaCutMove(current_move, depth);
                    ss->updateKillers(current_move);
                }
                break; //betaカット
            } else if (score > alpha) {
                best_move = current_move;
                alpha = std::max(alpha, best_score);

                pv_table_.update(best_move, distance_from_root);
            }
        }
        if (current_move.isQuiet()) {
            quiet_moves.push_back(current_move);
        }
    }

    //詰んでいるか
    if (move_count == 0) {
        best_score = MIN_SCORE + distance_from_root;
    }

    if (best_move == NULL_MOVE) {
        //fail-low
        //何かやるべきことがあるかな
    }

    if (best_move.isQuiet()) {
        for (auto quiet_move : quiet_moves) {
            history_.updateNonBetaCutMove(quiet_move, depth);
        }
    }

    //-----------------------------
    // 置換表に保存
    //-----------------------------
    shared_data.mutex.lock();
    shared_data.hash_table.save(pos.hash_value(), best_move, best_score, depth);
    shared_data.mutex.unlock();

    return best_score;
}