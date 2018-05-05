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

struct SearchLog {
    unsigned int hash_cut_num;
    unsigned int razoring_num;
    unsigned int futility_num;
    unsigned int null_move_num;
    void print() const {
        printf("hash_cut_num  = %u\n", hash_cut_num);
        printf("razoring_num  = %u\n", razoring_num);
        printf("futility_num  = %u\n", futility_num);
        printf("null_move_num = %u\n", null_move_num);
    }
};

static SearchLog search_log;

template<bool isPVNode>
Score Searcher::NegaAlphaBeta(Position &pos, Score alpha, Score beta, Depth depth, int distance_from_root) {
    // nodeの種類
    bool isRootNode = (distance_from_root == 0);

    //深さ最大まで来ていたら静止探索する仕様
    //if (depth <= 0) return qsearch<isPVNode>(pos, alpha, beta, Depth(0), distance_from_root);

    //-----------------------------
    // Step1. 初期化
    //-----------------------------

    //assert類
    assert(isPVNode || (alpha + 1 == beta));
    assert(MIN_SCORE <= alpha && alpha < beta && beta <= MAX_SCORE);

    //fprintf(stderr, "start  search(alpha = %5d, beta = %5d, depth = %3d, ply = %3d, %s)\n", alpha, beta, depth, ss->distance_from_root, isPVNode ? "PV" : "nonPV");

    //探索局面数を増やす
    ++node_number_;

    if (isPVNode) {
        seldepth_ = std::max(seldepth_, static_cast<Depth>(distance_from_root));
    }

    //-----------------------------
    // RootNode以外での処理
    //-----------------------------

    //thinkの方で1手動かしているのでここにルートノードはない
    //ルートノードも入れた方が効率いいんだろうか。多分そうなんだろうな

    if (!isRootNode) {

        //-----------------------------
        // Step2. 探索の停止と引き分けの確認
        //-----------------------------

        //停止確認
        if (shouldStop()) {
            return SCORE_ZERO;
        }

        //引き分けの確認
        //現局面が4手前と一致し、現局面と2手前が王手かどうか見ているだけ
        //これでうまくいっているか？
        Score repeate_score;
        if (pos.isRepeating(repeate_score)) {
            pv_table_.closePV(distance_from_root);
            return repeate_score;
        }

        //-----------------------------
        // Step3. Mate distance pruning
        //-----------------------------

        //合ってるのか怪しいぞ
        alpha = std::max(MIN_SCORE + static_cast<int>(distance_from_root), alpha);
        beta = std::min(MAX_SCORE - static_cast<int>(distance_from_root + 1), beta);
        if (alpha >= beta) {
            return alpha;
        }
    }

    //-----------------------------
    // SearchStackの初期化
    //-----------------------------

    SearchStack* ss = searchInfoAt(distance_from_root);
    (ss + 1)->killers[0] = (ss + 1)->killers[1] = NULL_MOVE;
    (ss + 1)->can_null_move = true;

    //-----------------------------
    // Step4. 置換表を見る
    //-----------------------------

    //除外する手(excluded_move)は設定していない
    //Singular extensionで使うやつか

    //posKeyをわざわざ変数として取り出す意味はあるんだろうか

    Move tt_move = NULL_MOVE;

    //-----------------------------
    // Step5. 局面の静的評価
    //-----------------------------

    Score static_score = (pos.color() == BLACK ? pos.score() : -pos.score());

    //王手がかかっているときは下の枝刈りはしない
    if (pos.isKingChecked()) {
        goto moves_loop;
    }

    //-----------------------------
    // static_scoreによる枝刈り
    //-----------------------------

    //-----------------------------
    // Step6. Razoring
    //-----------------------------
    if (!isPVNode
        && depth < 4 * PLY
        && static_score + depthMargin(depth) <= alpha) {
        if (depth <= 1 * PLY) {
            search_log.razoring_num++;
            return qsearch<isPVNode>(pos, alpha, beta, Depth(0), distance_from_root);
        }

        Score ralpha = alpha - depthMargin(depth);
        Score v = qsearch<isPVNode>(pos, ralpha, ralpha + 1, Depth(0), distance_from_root);
        if (v <= ralpha) {
            search_log.razoring_num++;
            return v;
        }
    }
    //-----------------------------
    // Step7. Futility pruning
    //-----------------------------

    //子ノードでのっていうのはどういうことなんだろう
    //上のRazoringと対応関係にあると思うんだけど、マージンとかはそろえるべきなのか
    if (depth < 7 * PLY && (static_score - depthMargin(depth) >= beta)) {
        search_log.futility_num++;
        return static_score;
    }


    //-----------------------------
    // Step8. Null Move Pruning
    //-----------------------------

    //うーん、弱くなるなぁ。なぜだかよくわからない

    if (!isPVNode
        && static_score >= beta
        && depth >= 2 * PLY
        && ss->can_null_move) {
        Depth rdepth = depth / 2;

        //2回連続null_moveになるのを避ける
        (ss + 1)->can_null_move = false;
        pos.doNullMove();
        Score null_score = -search<false>(pos, -beta, -beta + 1, rdepth, distance_from_root + 1);
        pos.undoNullMove();
        (ss + 1)->can_null_move = true;
        if (null_score >= beta) {
            search_log.null_move_num++;
            return null_score;
        }
    }

moves_loop:

    //Counter Move Historyやらなにやら
    //Gainとか

    std::vector<Move> quiet_moves;
    quiet_moves.reserve(600);

    int move_count = 0;

    //指し手を生成 
    MovePicker mp(pos, tt_move, depth, history_, ss->killers);
    Score best_score = MIN_SCORE + distance_from_root;
    Move best_move = NULL_MOVE;

    //-----------------------------
    // Step11. Loop through moves
    //-----------------------------

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

        //-----------------------------
        // extension
        //-----------------------------

        //ない

        //-----------------------------
        // Step12. Singular延長と王手延長
        //-----------------------------

        //-----------------------------
        // 1手進める前の枝刈り
        //-----------------------------

        //探索深さの増減もここで決めたり
        //Depth new_depth = depth >= 2 ? depth - 1 : depth;

        //-----------------------------
        // Step13. 浅い深さでの枝刈り
        //-----------------------------

        //Historyとかmove_countに基づいていろいろやるらしい
        //浅い深さというかMoveの性質に依存した枝刈り？

        //-----------------------------
        // Step14. 1手進める
        //-----------------------------

        //合法性判定は必要かどうか
        //今のところ合法手しかこないはずだけど

#if DEBUG
        if (!pos.isLegalMove(current_move)) {
            pos.isLegalMove(current_move);
            current_move.print();
            pos.printForDebug();
            assert(false);
        }
#endif
        pos.doMove(current_move);

        if (isPVNode) {
            pv_table_.closePV(distance_from_root + 1);
        }

        Score score;
        bool shouldSearchFullDepth = true;

        //-----------------------------
        // Step15. Move countに応じてdepthを減らした探索(Late Move Reduction)
        //-----------------------------

        //これめちゃくちゃバグってるっぽい
        if (depth >= PLY * 4 && move_count >= 5) {
            //alphaを更新しそうか確認
            //単に-2(普通はマイナス1だからそれよりも1手浅いということ)
            //雑なreductionにもほどがある
            Depth new_depth = depth - PLY - move_count * PLY / 30;
            score = (new_depth < PLY ? -qsearch<false>(pos, -alpha - 1, -alpha, Depth(0), distance_from_root + 1)
                : -NegaAlphaBeta<false>(pos, -alpha - 1, -alpha, new_depth - PLY, distance_from_root + 1));
            shouldSearchFullDepth = (score > alpha);
        }

        //-----------------------------
        // Step16. Full Depth Search
        //-----------------------------

        if (shouldSearchFullDepth) {
            //Null Window Searchでalphaを超えそうか確認
            //これ入れた方がいいのかなぁ
            score = (depth < PLY ? -qsearch<isPVNode>(pos, -alpha - 1, -alpha, Depth(0), distance_from_root + 1)
                : -NegaAlphaBeta<false>(pos, -alpha - 1, -alpha, depth - PLY, distance_from_root + 1));

            if (alpha < score && score < beta) {
                //isPVNodeって条件いるかな
                //nonPVならalpha + 1 == betaとなっているはずだからいらない気がする

                //いい感じのスコアだったので再探索
                score = (depth < PLY ? -qsearch<isPVNode>(pos, -beta, -alpha, Depth(0), distance_from_root + 1)
                    : -NegaAlphaBeta<isPVNode>(pos, -beta, -alpha, depth - PLY, distance_from_root + 1));
            }
            //#endif

        }

        //-----------------------------
        // Step17. 1手戻す
        //-----------------------------
        pos.undo();

        //-----------------------------
        // Step18. 停止確認
        //-----------------------------

        //停止確認
        if (shouldStop()) {
            return Score(0);
        }

        //-----------------------------
        // 探索された値によるalpha更新
        //-----------------------------
        if (score > best_score) {
            if (isRootNode) {
                //ルートノードならスコアを更新しておく
                root_move_itr->score = score;
            }

            best_score = score;
            best_move = current_move;
            pv_table_.update(best_move, distance_from_root);

            if (score >= beta) {
                //fail-high
                break; //betaカット
            } else if (score > alpha) {
                alpha = std::max(alpha, best_score);
            }
        }
        if (current_move.isQuiet()) {
            quiet_moves.push_back(current_move);
        }
    }

    //-----------------------------
    // Step20. 詰みの確認
    //-----------------------------

    //打ち歩の処理はここでやった方がいい
    //↑ほんとか？

    if (move_count == 0) {
        //詰みということ
        return MIN_SCORE + distance_from_root;
    }

    if (best_move == NULL_MOVE) {
        //fail-low
        //なにかやるべきことはあるか
    } else if (best_move.isQuiet()) {
        history_.updateBetaCutMove(best_move, depth);
        ss->updateKillers(best_move);

        for (auto quiet_move : quiet_moves) {
            history_.updateNonBetaCutMove(quiet_move, depth);
        }
    }

    return best_score;
}

void Searcher::think() {
    //思考開始時間をセット
    start_ = std::chrono::steady_clock::now();

    //コピーして使う
    Position root = shared_data.root;

    if (role_ == MATE) {
        search_mate(root);
        return;
    }

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
    ss->can_null_move = true;

    //合法手が0だったら投了
    if (root_moves_.size() == 0) {
        if (role_ == MAIN) {
            std::cout << "bestmove resign" << std::endl;
        }
        return;
    }

    //合法手が1つだったらすぐ送る
    //これ別にいらないよな
    //これもUSIオプション化した方が良いか
    //if (root_moves_.size() == 1) {
    //    if (role_ == MAIN) {
    //        std::cout << "bestmove " << root_moves_[0] << std::endl;
    //    }
    //    return;
    //}

    //指定された手数まで完全ランダムに指す
    static std::random_device rd;
    if (root.turn_number() + 1 <= usi_option.random_turn) {
        if (role_ == MAIN) {
            int rnd = rd() % root_moves_.size();
            std::cout << "bestmove " << root_moves_[rnd] << std::endl;
        }
        return;
    }

    //探索局面数を初期化
    node_number_ = 0;

    //探索
    //反復深化
    static const Score DEFAULT_ASPIRATION_WINDOW_SIZE = Score(64);
    Score aspiration_window_size = DEFAULT_ASPIRATION_WINDOW_SIZE;
    Score best_score, alpha, beta, previous_best_score;

    for (Depth depth = PLY * 1; depth <= DEPTH_MAX; depth += PLY) {
        if (role_ == SLAVE) { //Svaleスレッドは探索深さを深くする
            static std::mt19937 mt(rd());
            static std::uniform_int_distribution<> distribution(0, 2);
            depth += distribution(mt) * PLY;
        }

        //seldepth_の初期化
        seldepth_ = depth;

        //探索窓の設定
        if (depth <= 4 * PLY) { //深さ4まではASPIRATION_WINDOWを使わずフルで探索する
            alpha = MIN_SCORE;
            beta = MAX_SCORE;
        } else {
            alpha = std::max(previous_best_score - aspiration_window_size, MIN_SCORE);
            beta = std::min(previous_best_score + aspiration_window_size, MAX_SCORE);
        }

        while (!shouldStop()) { //exactな評価値が返ってくるまでウィンドウを広げつつ探索
            //指し手のスコアを最小にしておかないと変になる
            for (auto& root_move : root_moves_) {
                root_move.score = MIN_SCORE;
            }

            best_score = search<true>(root, alpha, beta, depth, 0);

            //history_.print();

            //詰んでいたら抜ける
            if (isMatedScore(best_score) || shouldStop()) {
                break;
            }

            if (best_score <= alpha) {
                //fail-low
                if (role_ == MAIN) {
                    printf("aspiration fail-low, alpha = %d\n", alpha);
                    sendInfo(depth, "cp", best_score, UPPER_BOUND);
                }

                beta = (alpha + beta) / 2;
                alpha -= aspiration_window_size;
                aspiration_window_size *= 3;
            } else if (best_score >= beta) {
                //fail-high
                if (role_ == MAIN) {
                    printf("aspiration fail-high, beta = %d\n", beta);
                    sendInfo(depth, "cp", best_score, LOWER_BOUND);
                }

                alpha = (alpha + beta) / 2;
                beta += aspiration_window_size;
                aspiration_window_size *= 3;
            } else {
                aspiration_window_size = DEFAULT_ASPIRATION_WINDOW_SIZE;
                break;
            }
        }

        //停止確認してダメだったら保存せずループを抜ける
        if (shouldStop()) {
            break;
        }

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
        shared_data.hash_table.save(root.hash_value(), root_moves_[0], root_moves_[0].score, depth);

        //詰みがあったらすぐ返す
        if (isMatedScore(root_moves_[0].score)) {
            break;
        }

        //今回のイテレーションにおけるスコアを記録
        previous_best_score = best_score;

        //PVtableをリセットしていいよな？
        resetPVTable();
    }

    //GUIへBestMoveの情報を送る
    if (role_ == MAIN) {
        std::cout << "bestmove " << root_moves_[0] << std::endl;

        //ログを出力
        search_log.print();
    }
}

Move Searcher::thinkForGenerateLearnData(Position& root, Depth fixed_depth, unsigned int random_turn) {
    //思考開始時間をセット
    start_ = std::chrono::steady_clock::now();

    //History初期化
    history_.clear();

    //PV初期化
    resetPVTable();

    //ルート局面の合法手を設定
    root_moves_ = root.generateAllMoves();

    SearchStack* ss = searchInfoAt(0);
    ss->killers[0] = ss->killers[1] = NULL_MOVE;
    ss->can_null_move = true;

    //合法手が0だったら投了
    if (root_moves_.size() == 0) {
        return NULL_MOVE;
    }

    //指定された手数まで完全ランダムに指す
    static std::random_device rd;
    if (root.turn_number() + 1 <= random_turn) {
        int rnd = rd() % root_moves_.size();
        root_moves_[rnd].score = MIN_SCORE;
        return root_moves_[rnd];
    }

    //探索局面数を初期化
    node_number_ = 0;

    for (auto& root_move : root_moves_) {
        root_move.score = MIN_SCORE;
    }

    //Score best_score = search<true>(root, MIN_SCORE, MAX_SCORE, fixed_depth * (int)PLY, 0);
    Score best_score = (fixed_depth < PLY ? qsearch<true>(root, MIN_SCORE, MAX_SCORE, Depth(0), 0) : NegaAlphaBeta<true>(root, MIN_SCORE, MAX_SCORE, fixed_depth, 0));

    return *std::max_element(root_moves_.begin(), root_moves_.end());
}

template<bool isPVNode>
Score Searcher::qsearch(Position &pos, Score alpha, Score beta, Depth depth, int distance_from_root) {

    // -----------------------
    //     最初のチェック
    // -----------------------

    pv_table_.closePV(distance_from_root);

    assert(isPVNode || alpha + 1 == beta);
    assert(0 <= distance_from_root && distance_from_root <= DEPTH_MAX);

    // -----------------------
    //    変数宣言とかnodeの初期化
    // -----------------------

    Move best_move = NULL_MOVE;
    Score old_alpha = alpha;

    SearchStack* ss = searchInfoAt(distance_from_root);

    //探索局面数を増やす
    node_number_++;

    seldepth_ = std::max(seldepth_, static_cast<Depth>(distance_from_root));
    //fprintf(stderr, "start qsearch(alpha = %5d, beta = %5d, depth = %3d, ply = %3d, %s)\n", alpha, beta, depth, ss->distance_from_root, isPVNode ? "PV" : "nonPV");

    // -----------------------
    //     停止確認
    // -----------------------

    //ここでやる必要があるのかはわからない
    if (shouldStop()) return SCORE_ZERO;

    // -----------------------
    //     置換表を見る
    // -----------------------
    auto hash_entry = shared_data.hash_table.find(pos.hash_value());
    Score tt_score = hash_entry ? hash_entry->best_move_.score : MIN_SCORE;

    //tt_moveの合法性判定はここでやらなくてもMovePickerで確認するはず
    Move tt_move = hash_entry ? hash_entry->best_move_ : NULL_MOVE;
    Depth tt_depth = hash_entry ? hash_entry->depth_ : Depth(0);

    //置換表による枝刈り
    if (!pos.isKingChecked()) { //この条件いる？
        //置換表での枝刈り
        if (!isPVNode
            && hash_entry
            && tt_depth >= depth
            && tt_score >= beta) {
            search_log.hash_cut_num++;
            return tt_score;
        }
    }

    //指し手を生成 
    MovePicker mp(pos, tt_move, depth, history_);
    Score best_score = (pos.isKingChecked() ? MIN_SCORE + distance_from_root //王手がかかっていたら最低点から始める
        : pos.color() == BLACK ? pos.score() : -pos.score());                            //そうでなかったら評価関数を呼び出した値から始める
    int move_count = 0;
    for (Move current_move = mp.nextMove(); current_move != NULL_MOVE; current_move = mp.nextMove()) {
        //停止確認
        if (shouldStop()) {
            return Score(0);
        }

        //Null Window Searchはした方がいいんだろうか.今はしてない

        pos.doMove(current_move);
        Score score = -qsearch<isPVNode>(pos, -beta, -alpha, depth - 1, distance_from_root + 1);

        if (score > best_score) {
            best_score = score;
            best_move = current_move;

            if (best_score >= beta) {
                //fail-high
                pos.undo();
                //静止探索でQuietな指し手になるわけがなくない？
                //fail-highとして置換表にsaveするべき？
                return best_score; //betaカット
            }

            pv_table_.update(best_move, distance_from_root);

            alpha = std::max(alpha, best_score);
        }

        pos.undo();

        ++move_count;
    }

    if (best_move != NULL_MOVE) {
        //fail-lowではない
        //置換表への保存  todo:old_alphaと比較して値の信頼性を付け加えるべき
        //静止探索の結果なんて保存しても仕方なくない？
        //抜いてみる
        //shared_data.hash_table.save(pos.hash_value(), best_move, best_score, Depth(0));
    }

    return best_score;
}

bool Searcher::search_check(Position &pos, Depth depth) {
    //時間を確認
    if (shouldStop()) return false;

    //局面数を増やす
    node_number_++;

    //王手を生成
    Move check_moves[MAX_MOVE_LIST_SIZE];
    Move* end_ptr = check_moves;
    pos.generateCheckMoveBB(end_ptr);

    //可能な王手の数が0だったら詰まない
    if (end_ptr == check_moves) return false;

    //深さ最大まで来ていてまだ合法手があるなら不詰み
    //王手側で深さ最大になることはないはずだけど一応
    if (depth <= 0) return false;

    //全ての王手について1手読みを深める
    for (Move* start_ptr = check_moves; start_ptr < end_ptr; start_ptr++) {
        pos.doMove(*start_ptr);
        if (search_evasion(pos, depth - 1)) { //詰み
            //PV更新
            pv_table_.update(*start_ptr, depth);

            //局面を戻す
            pos.undo();

            return true;
        }
        pos.undo();
    }

    return false;
}

bool Searcher::search_evasion(Position &pos, Depth depth) {
    //時間を確認
    if (shouldStop()) return false;

    //局面数を増やす
    node_number_++;

    //指し手を全て生成
    std::vector<Move> move_list;
    Move evasion_moves[MAX_MOVE_LIST_SIZE];
    Move* move_ptr = evasion_moves;
    pos.generateEvasionMovesBB(move_ptr);

    //番兵を設置
    *move_ptr = NULL_MOVE;

    //可能な指し手の数が0だったらtrueを返す
    if (evasion_moves[0] == NULL_MOVE) return true;

    //深さ最大まで来ていてまだ合法手があるなら不詰み
    if (depth <= 0) return false;

    //全ての指し手について1手読みを深める
    for (Move* current_move = &evasion_moves[0]; *current_move != NULL_MOVE; ++current_move) {
        pos.doMove(*current_move);
        if (!search_check(pos, depth - 1)) { //詰まない逃れ方がある
            pos.undo();
            return false;
        }
        pos.undo();
    }

    //どの逃げ方でも詰んでいたら詰み
    return true;
}

void Searcher::sendInfo(Depth depth, std::string cp_or_mate, Score score, Bound bound) {
    auto now_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now_time - start_);
    std::cout << "info time " << elapsed.count() << " ";

    //GUIへ読み途中の情報を返す
    std::cout << "depth " << depth / PLY << " ";
    std::cout << "seldepth " << seldepth_ / PLY << " ";
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

void Searcher::search_mate(Position& root) {
    for (Depth depth = Depth(1); ; depth += 2) {
        if (shouldStop()) {
            break;
        }

        if (search_check(root, depth)) {
            std::cout << "詰み" << std::endl;

            //詰みを発見したので他に動いているスレッドを止める
            shared_data.stop_signal = true;

            //詰みを発見したというシグナルそのものも必要そうな気がする
            //shared_data.find_mate = true;

            //GUIに読み筋を送る
            sendInfo(depth, "mate", static_cast<Score>(depth), EXACT_BOUND);

            //GUIにbest_moveを送る
            //std::cout << "bestmove " << mate_move << std::endl;
        }
    }
}

#define OMIT_PRUNINGS

template<bool isPVNode>
Score Searcher::search(Position &pos, Score alpha, Score beta, Depth depth, int distance_from_root) {
    // nodeの種類
    bool isRootNode = (distance_from_root == 0);

    //-----------------------------
    // Step1. 初期化
    //-----------------------------

    //assert類
    assert(isPVNode || (alpha + 1 == beta));
    assert(MIN_SCORE <= alpha && alpha < beta && beta <= MAX_SCORE);

    //fprintf(stderr, "start  search(alpha = %5d, beta = %5d, depth = %3d, ply = %3d, %s)\n", alpha, beta, depth, ss->distance_from_root, isPVNode ? "PV" : "nonPV");

    //探索局面数を増やす
    ++node_number_;

    if (isPVNode) {
        seldepth_ = std::max(seldepth_, static_cast<Depth>(distance_from_root));
    }

    //-----------------------------
    // RootNode以外での処理
    //-----------------------------

    //thinkの方で1手動かしているのでここにルートノードはない
    //ルートノードも入れた方が効率いいんだろうか。多分そうなんだろうな

    if (!isRootNode) {

        //-----------------------------
        // Step2. 探索の停止と引き分けの確認
        //-----------------------------

        //停止確認
        if (shouldStop()) {
            return SCORE_ZERO;
        }

        //引き分けの確認
        //現局面が4手前と一致し、現局面と2手前が王手かどうか見ているだけ
        //これでうまくいっているか？
        Score repeate_score;
        if (pos.isRepeating(repeate_score)) {
            pv_table_.closePV(distance_from_root);
            return repeate_score;
        }

        //-----------------------------
        // Step3. Mate distance pruning
        //-----------------------------

        //合ってるのか怪しいぞ
        alpha = std::max(MIN_SCORE + static_cast<int>(distance_from_root), alpha);
        beta = std::min(MAX_SCORE - static_cast<int>(distance_from_root + 1), beta);
        if (alpha >= beta) {
            return alpha;
        }
    }

    //-----------------------------
    // SearchStackの初期化
    //-----------------------------

    SearchStack* ss = searchInfoAt(distance_from_root);
    (ss + 1)->killers[0] = (ss + 1)->killers[1] = NULL_MOVE;
    (ss + 1)->can_null_move = true;

    //-----------------------------
    // Step4. 置換表を見る
    //-----------------------------

    //除外する手(excluded_move)は設定していない
    //Singular extensionで使うやつか

    auto hash_entry = shared_data.hash_table.find(pos.hash_value());
    Score tt_score = hash_entry ? hash_entry->best_move_.score : MIN_SCORE;

    //tt_moveの合法性判定はここでやらなくてもMovePickerで確認するはず
    Move tt_move = hash_entry ? hash_entry->best_move_ : NULL_MOVE;
    Depth tt_depth = hash_entry ? hash_entry->depth_ : Depth(0);

    //置換表の値による枝刈り
    if (!isPVNode
        && hash_entry
        && tt_depth >= depth
        && tt_score >= beta) {

        //tt_moveがちゃんとしたMoveならこれでHistory更新
        if (tt_move != NULL_MOVE) {
            history_.updateBetaCutMove(tt_move, depth);
        }
        search_log.hash_cut_num++;
        return tt_score;
    }

    //-----------------------------
    // 宣言勝ち
    //-----------------------------

    //まだない

    //-----------------------------
    // 1手詰み判定
    //-----------------------------

    //まだない

    //-----------------------------
    // Step5. 局面の静的評価
    //-----------------------------

    Score static_score = (hash_entry ? tt_score : (pos.color() == BLACK ? pos.score() : -pos.score()));

    //王手がかかっているときは下の枝刈りはしない
    if (pos.isKingChecked()) {
        goto moves_loop;
    }

    //-----------------------------
    // static_scoreによる枝刈り
    //-----------------------------

    //-----------------------------
    // Step6. Razoring
    //-----------------------------
    if (!isPVNode
        && depth < 4 * PLY
        && static_score + depthMargin(depth) <= alpha) {
        if (depth <= 1 * PLY) {
            search_log.razoring_num++;
            return qsearch<isPVNode>(pos, alpha, beta, Depth(0), distance_from_root);
        }

        Score ralpha = alpha - depthMargin(depth);
        Score v = qsearch<isPVNode>(pos, ralpha, ralpha + 1, Depth(0), distance_from_root);
        if (v <= ralpha) {
            search_log.razoring_num++;
            return v;
        }
    }
    //-----------------------------
    // Step7. Futility pruning
    //-----------------------------

    //子ノードでのっていうのはどういうことなんだろう
    //上のRazoringと対応関係にあると思うんだけど、マージンとかはそろえるべきなのか
    if (depth < 7 * PLY && (static_score - depthMargin(depth) >= beta)) {
        search_log.futility_num++;
        return static_score;
    }


    //-----------------------------
    // Step8. Null Move Pruning
    //-----------------------------

    //うーん、弱くなるなぁ。なぜだかよくわからない

    if (!isPVNode
        && static_score >= beta
        && depth >= 2 * PLY
        && ss->can_null_move) {
        Depth rdepth = depth / 2;

        //2回連続null_moveになるのを避ける
        (ss + 1)->can_null_move = false;
        pos.doNullMove();
        Score null_score = -search<false>(pos, -beta, -beta + 1, rdepth, distance_from_root + 1);
        pos.undoNullMove();
        (ss + 1)->can_null_move = true;
        if (null_score >= beta) {
            search_log.null_move_num++;
            return null_score;
        }
    }

#ifndef OMIT_PRUNINGS
    //-----------------------------
    // Step9. ProbCut
    //-----------------------------

    if (!isPVNode
        && depth >= 5 * PLY) {
        Score rbeta = std::min(beta + 300, MAX_SCORE);
        Depth rdepth = depth - 4;
        MovePicker mp(pos, tt_move, rdepth, history_, ss->killers);
        for (Move move = mp.nextMove(); move != NULL_MOVE; move = mp.nextMove()) {
            pos.doMove(move);
            Score score = -search<false>(pos, -rbeta, -rbeta + 1, rdepth, distance_from_root + 1);
            pos.undo();
            if (score >= rbeta) {
                return score;
            }
        }
    }
#endif

    //-----------------------------
    // Step10. 多重反復深化
    //-----------------------------

    //まだない
    //あまり必要性も感じない
    //いや、本質的なのでは？
    if (depth >= 7 * PLY
        && tt_move == NULL_MOVE
        && (isPVNode || static_score + 256 >= beta)) {
        Depth d = (depth * 3 / 4) - 2 * PLY;
        ss->can_null_move = false;
        search<isPVNode>(pos, alpha, beta, d, distance_from_root);
        ss->can_null_move = true;

        hash_entry = shared_data.hash_table.find(pos.hash_value());
        tt_move = hash_entry ? hash_entry->best_move_ : NULL_MOVE;
    }

moves_loop:

    //Counter Move Historyやらなにやら
    //Gainとか

    std::vector<Move> quiet_moves;
    quiet_moves.reserve(600);

    int move_count = 0;

    //指し手を生成 
    MovePicker mp(pos, tt_move, depth, history_, ss->killers);
    Score best_score = MIN_SCORE + distance_from_root;
    Move best_move = NULL_MOVE;

    //-----------------------------
    // Step11. Loop through moves
    //-----------------------------

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

        //-----------------------------
        // extension
        //-----------------------------

        //ない

        //-----------------------------
        // Step12. Singular延長と王手延長
        //-----------------------------

        //-----------------------------
        // 1手進める前の枝刈り
        //-----------------------------

        //探索深さの増減もここで決めたり
        //Depth new_depth = depth >= 2 ? depth - 1 : depth;

        //-----------------------------
        // Step13. 浅い深さでの枝刈り
        //-----------------------------

        //Historyとかmove_countに基づいていろいろやるらしい
        //浅い深さというかMoveの性質に依存した枝刈り？

        //-----------------------------
        // Step14. 1手進める
        //-----------------------------

        //合法性判定は必要かどうか
        //今のところ合法手しかこないはずだけど

#if DEBUG
        if (!pos.isLegalMove(current_move)) {
            pos.isLegalMove(current_move);
            current_move.print();
            pos.printForDebug();
            assert(false);
        }
#endif

        pos.doMove(current_move);

        if (isPVNode) {
            pv_table_.closePV(distance_from_root + 1);
        }

        Score score;
        bool shouldSearchFullDepth = true;

        //-----------------------------
        // Step15. Move countに応じてdepthを減らした探索(Late Move Reduction)
        //-----------------------------

        //これめちゃくちゃバグってるっぽい
        if (depth >= PLY * 4 && move_count >= 5) {
            //alphaを更新しそうか確認
            //雑なreductionにもほどがある
            Depth new_depth = depth - move_count * PLY / 30;
            score = (new_depth < PLY ? -qsearch<false>(pos, -alpha - 1, -alpha, Depth(0), distance_from_root + 1)
                : -search<false>(pos, -alpha - 1, -alpha, new_depth - PLY, distance_from_root + 1));
            shouldSearchFullDepth = (score > alpha);
        }

        //-----------------------------
        // Step16. Full Depth Search
        //-----------------------------

        if (shouldSearchFullDepth) {
            //Null Window Searchでalphaを超えそうか確認
            //これ入れた方がいいのかなぁ
            score = (depth < PLY ? -qsearch<isPVNode>(pos, -alpha - 1, -alpha, Depth(0), distance_from_root + 1)
                : -search<false>(pos, -alpha - 1, -alpha, depth - PLY, distance_from_root + 1));

            if (alpha < score && score < beta) {
                //isPVNodeって条件いるかな
                //nonPVならalpha + 1 == betaとなっているはずだからいらない気がする

                //いい感じのスコアだったので再探索
                score = (depth < PLY ? -qsearch<isPVNode>(pos, -beta, -alpha, Depth(0), distance_from_root + 1)
                    : -search<isPVNode>(pos, -beta, -alpha, depth - PLY, distance_from_root + 1));
            }
            //#endif

        }

        //-----------------------------
        // Step17. 1手戻す
        //-----------------------------
        pos.undo();

        //-----------------------------
        // Step18. 停止確認
        //-----------------------------

        //停止確認
        if (shouldStop()) {
            return Score(0);
        }

        //-----------------------------
        // 探索された値によるalpha更新
        //-----------------------------
        if (score > best_score) {
            if (isRootNode) {
                //ルートノードならスコアを更新しておく
                root_move_itr->score = score;
            }

            best_score = score;
            best_move = current_move;
            pv_table_.update(best_move, distance_from_root);

            if (score >= beta) {
                //fail-high
                break; //betaカット
            } else if (score > alpha) {
                alpha = std::max(alpha, best_score);
            }
        }
        if (current_move.isQuiet()) {
            quiet_moves.push_back(current_move);
        }
    }

    //-----------------------------
    // Step20. 詰みの確認
    //-----------------------------

    //打ち歩の処理はここでやった方がいい
    //↑ほんとか？

    if (move_count == 0) {
        //詰みということ
        //置換表に保存しても仕方ないのでさっさと値を返す
        return MIN_SCORE + distance_from_root;
    }

    if (best_move == NULL_MOVE) {
        //fail-low
        //なにかやるべきことはあるか
    } else if (best_move.isQuiet()) {
        history_.updateBetaCutMove(best_move, depth);
        ss->updateKillers(best_move);

        for (auto quiet_move : quiet_moves) {
            history_.updateNonBetaCutMove(quiet_move, depth);
        }
    }

    //-----------------------------
    // 置換表に保存
    //-----------------------------
    shared_data.hash_table.save(pos.hash_value(), best_move, best_score, depth);

    return best_score;
}