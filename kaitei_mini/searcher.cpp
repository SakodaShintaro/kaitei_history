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
    if (is_main_thread_)
        root.print();

    //History初期化
    history_.clear();

    //合法手生成
    std::vector<Move> root_moves = root.generateAllMoves();

    //合法手が0だったら投了
    if (root_moves.size() == 0) {
        if (is_main_thread_) {
            std::cout << "bestmove resign" << std::endl;
        }
        return;
    }

    //合法手が1つだったらすぐ送る
    if (root_moves.size() == 1) {
        if (is_main_thread_) {
            std::cout << "bestmove " << root_moves[0] << std::endl;
        }
        return;
    }

    //指定された手数まで完全ランダムに指す
    std::random_device rd;
    if (root.turn_number() + 1 <= usi_option.random_turn) {
        if (is_main_thread_) {
            int rnd = rd() % root_moves.size();
            std::cout << "random mode : " << rnd << std::endl;
            std::cout << "bestmove " << root_moves[rnd] << std::endl;
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

    for (Depth depth_max = Depth(0); depth_max <= DEPTH_MAX; ++depth_max) {
        //seldepth_の設定
        seldepth_ = depth_max;

        //探索窓の設定
        if (depth_max <= 4) {
            old_alpha = alpha = MIN_SCORE;
            beta = MAX_SCORE;
        } else {
            old_alpha = alpha = previous_best_score - aspiration_window_size;
            beta = previous_best_score + aspiration_window_size;
        }

        while (true) {
            //停止確認
            if (shouldStop())
                break;

            //exactな評価値が返ってくるまでウィンドウを広げつつ探索
            best_score = MIN_SCORE;

            for (auto& move : root_moves) {

                root.doMove(move);
                move.score = -search<true>(root, -beta, -alpha, depth_max, 1);
                root.undo();

                //停止確認
                if (shouldStop())
                    break;

                if (move.score > best_score) {
                    best_score = move.score;
                    //fail-high
                    if (best_score >= beta) {
                        break;
                    }

                    alpha = std::max(alpha, best_score);

                    //pv更新
                    pv_.update(move, 0);
                }
            }

            //停止確認
            if (shouldStop())
                break;

            //詰んでいたら抜ける
            if (isMatedScore(best_score))
                break;

            if (best_score <= old_alpha) {
                //fail-low
                if (is_main_thread_) {
                    printf("aspiration fail-low, alpha = %d\n", old_alpha);
                    sendInfo(depth_max + 1, "cp", best_score, UPPER_BOUND);
                }

                beta = (alpha + beta) / 2;
                old_alpha = alpha = old_alpha - aspiration_window_size;
                aspiration_window_size *= 2;
            } else if (best_score >= beta) {
                //fail-high
                if (is_main_thread_) {
                    printf("aspiration fail-high, beta = %d\n", beta);
                    sendInfo(depth_max + 1, "cp", best_score, LOWER_BOUND);
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
        std::stable_sort(root_moves.begin(), root_moves.end(), std::greater<Move>());

        //GUIへ読みの情報を送る
        if (is_main_thread_) {
            if (MATE_SCORE_UPPER_BOUND < root_moves[0].score && root_moves[0].score < MATE_SCORE_LOWER_BOUND) {
                //詰みなし
                sendInfo(depth_max + 1, "cp", root_moves[0].score, EXACT_BOUND);
            } else {
                //詰みあり
                Score mate_num = MAX_SCORE - std::abs(root_moves[0].score);
                mate_num *= (mate_num % 2 == 0 ? -1 : 1);
                sendInfo(depth_max + 1, "mate", mate_num, EXACT_BOUND);
            }
        }

        //置換表への保存
        shared_data.mutex.lock();
        shared_data.hash_table.save(root.hash_value(), root_moves[0], root_moves[0].score, depth_max);
        shared_data.mutex.unlock();

        //詰みがあったらすぐ返す
        if (isMatedScore(root_moves[0].score))
            break;

        //今回のイテレーションにおけるスコアを記録
        previous_best_score = best_score;
    }

    //GUIへBestMoveの情報を送る
    if (is_main_thread_)
        std::cout << "bestmove " << root_moves[0] << std::endl;
}

template<bool isPV>
Score Searcher::qsearch(Position &pos, Score alpha, Score beta, Depth depth, int distance_from_root) {

    // -----------------------
    //     最初のチェック
    // -----------------------

    //連続王手の千日手風になったときは無限ループになるので適当なところで抜ける
    //こんなやりかたはダサすぎる気がするけど……
    if (distance_from_root >= DEPTH_MAX) {
        return (pos.color() == BLACK ? pos.score() : -pos.score());
    }

    pv_.closePV(distance_from_root);

    assert(isPV || alpha + 1 == beta);
    assert(0 <= distance_from_root && distance_from_root <= DEPTH_MAX);

    // -----------------------
    //     変数宣言とかnodeの初期化
    // -----------------------

    Move best_move = NULL_MOVE;

    //探索局面数を増やす
    node_number_++;

    seldepth_ = std::max(seldepth_, static_cast<Depth>(distance_from_root));

    // -----------------------
    //     停止確認
    // -----------------------

    //ここでやる必要があるのかはわからない
    if (shouldStop()) return SCORE_ZERO;

    // -----------------------
    //     置換表を見る
    // -----------------------
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
            if (!isPV
                && tt_hit
                && tt_depth >= depth
                && tt_score >= beta) {
                return tt_score;
            }
        }
    }

    //指し手を生成 
    MovePicker mp(pos, tt_move, depth, history_, pos.lastMove().to());
    Score best_score = (pos.isKingChecked(pos.color()) ? MIN_SCORE + distance_from_root //王手がかかっていたら最低点から始める
        :pos.color() == BLACK ? pos.score() : -pos.score());                            //そうでなかったら評価関数を呼び出した値から始める
    int move_count = 0;

    for (Move current_move; (current_move = mp.nextMove()) != NULL_MOVE;) {
        //停止確認
        if (shouldStop())
            return Score(0);

        //Null Window Search
        //qsearchではしない方がいい？ 暫定的に外してみる
        pos.doMove(current_move);
        Score score = -qsearch<isPV>(pos, -beta, -alpha, depth - 1, distance_from_root + 1);

        if (score > best_score) {
            best_score = score;
            best_move = current_move;

            if (best_score >= beta) {
                //fail-high
                pos.undo();
                history_.updateBetaCutMove(best_move, depth);
                //fail-highとして置換表にsaveするべき
                return best_score; //betaカット
            }

            //Windowを戻して再探索

            pv_.update(best_move, distance_from_root);

            alpha = std::max(alpha, best_score);
        }

        history_.updateNonBetaCutMove(current_move, depth);
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

bool Searcher::search_check(Position &now, int depth, int depth_max) {
    //時間を確認
    if (isTimeOver()) return false;

    //局面数を増やす
    node_number_++;

    //王手を生成
    //ここ嘘
    std::vector<Move> check_move_list;

    //可能な王手の数が0だったら詰まない
    if (check_move_list.size() == 0) return false;

    //深さ最大まで来ていてまだ合法手があるなら不詰み
    //王手側で深さ最大になることはないはずだけど一応
    if (depth == depth_max) return false;

    //全ての王手について1手読みを深める
    for (unsigned int i = 0; i < check_move_list.size(); i++) {
        now.doMove(check_move_list[i]);
        if (search_evasion(now, depth + 1, depth_max)) { //詰み
            //PV更新
            now.undo();
            return true;
        }
        now.undo();
    }

    return false;
}

bool Searcher::search_evasion(Position &now, int depth, int depth_max) {
    //時間を確認
    if (isTimeOver()) return false;

    //局面数を増やす
    node_number_++;

    //指し手を全て生成
    std::vector<Move> move_list;

    //可能な指し手の数が0だったらtrueを返す
    if (move_list.size() == 0) return true;

    //深さ最大まで来ていてまだ合法手があるなら不詰み
    if (depth == depth_max) return false;

    //全ての指し手について1手読みを深める
    for (unsigned int i = 0; i < move_list.size(); i++) {
        now.doMove(move_list[i]);
        if (search_check(now, depth + 1, depth_max) == false) { //詰まない逃れ方がある
            now.undo();
            return false;
        }
        now.undo();
    }

    return true;
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
    for (auto move : pv_) {
        std::cout << move << " ";
    }
    std::cout << std::endl;
    std::cout << "info nps " << static_cast<double>(node_number_) / elapsed.count() * 1000.0 << "\n";
    std::cout << "info hashfull " << shared_data.hash_table.hashfull() << "\n";
}

void Searcher::search_mate(Position pos) {
    int mate_depth = pos.turn_number() >= 60 ? MATE_DEPTH_MAX : 0;
    if (mate_depth > 0) {
        for (int i = 1; i <= mate_depth; i += 2) {
            if (search_check(pos, 0, i)) {
                return;
            }
        }
    }
}

template<bool isPV>
Score Searcher::search(Position &pos, Score alpha, Score beta, Depth depth, int distance_from_root) {

    //変数宣言
    Move best_move;

    //assert見てるのはこれだけ
    assert(isPV || alpha + 1 == beta);
    assert(MIN_SCORE <= alpha && alpha < beta && beta <= MAX_SCORE);

    //探索局面数を増やす
    ++node_number_;

    if (isPV) {
        seldepth_ = std::max(seldepth_, static_cast<Depth>(distance_from_root));
    }

    //停止確認
    if (shouldStop())
        return SCORE_ZERO;

    //引き分けの確認
    //現局面が4手前と一致し、現局面と2手前が王手かどうか見ているだけ
    //これでうまくいっているか？
    Score repeate_score;
    if (pos.isRepeating(repeate_score)) {
        return repeate_score;
    }

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
    if (!isPV
        && tt_hit
        && tt_depth >= depth
        && tt_score >= beta) {
        
        //tt_moveがちゃんとしたMoveならこれでHistory更新
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
    if (!isPV
        && depth < 4
        && static_score + razorMargin(depth) <= alpha) {
        if (depth <= 1)
            return qsearch<isPV>(pos, alpha, beta, Depth(0), distance_from_root);

        Score ralpha = alpha - razorMargin(depth);
        Score v = qsearch<isPV>(pos, ralpha, ralpha + 1, Depth(0), distance_from_root);
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

    //深さ最大まで来ていたら静止探索する仕様
    if (depth == 0) return qsearch<isPV>(pos, alpha, beta, Depth(0), distance_from_root);

    std::vector<Move> quiet_moves;

    int move_count = 0;

    //指し手を生成 
    Move current_move;
    MovePicker mp(pos, tt_move, depth, history_);
    Score best_score = MIN_SCORE + distance_from_root;

    //1手ずつ調べる
    while (!((current_move = mp.nextMove()) == NULL_MOVE)) {
        Score score;

        ++move_count;

        //1手進める
        assert(pos.isLegalMove(current_move));
        pos.doMove(current_move);

        if (isPV)
            pv_.closePV(distance_from_root + 1);

        //Null Window Search
        score = -search<false>(pos, -alpha - 1, -alpha, depth - 1, distance_from_root + 1);

        if (alpha < score && score < beta) {
            //isPVって条件いるかな
            //nonPVならalpha + 1 == betaとなっているはずだからいらない気がする
            
            //いい感じのスコアだったので再探索
            score = -search<isPV>(pos, -beta, -alpha, depth - 1, distance_from_root + 1);
        }

        //1手戻す
        pos.undo();

        //停止確認
        if (shouldStop())
            return Score(0);

        //探索された値によってalphaなど更新
        if (score > best_score) {
            best_score = score;
            if (score >= beta) {
                //fail-high
                history_.updateBetaCutMove(current_move, depth);
                pv_.update(current_move, distance_from_root);
                return score; //betaカット
            } else if (score > alpha) {
                best_move = current_move;
                alpha = std::max(alpha, best_score);

                pv_.update(best_move, distance_from_root);
            }
        }
        history_.updateNonBetaCutMove(current_move, depth);
    }

    if (move_count == 0) { //詰み
        best_score = MIN_SCORE + distance_from_root;
    }
    
    //-----------------------------
    // 置換表に保存
    //-----------------------------
    shared_data.mutex.lock();
    shared_data.hash_table.save(pos.hash_value(), best_move, best_score, depth);
    shared_data.mutex.unlock();

    return best_score;
}