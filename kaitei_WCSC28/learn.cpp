#include"learn.hpp"
#include"position.hpp"
#include"searcher.hpp"
#include"load_game.hpp"
#include"eval_params.hpp"
#include"usi_options.hpp"
#include"thread.hpp"
#include<chrono>
#include<vector>
#include<algorithm>
#include<utility>
#include<functional>
#include<iostream>
#include<iomanip>

extern USIOption usi_option;

//1回パラメータを更新するまでの学習局数
static int MINI_BATCH_SIZE;

//学習する棋譜
static std::vector<Game> games;

//学習しているスレッドの数
static unsigned int thread_num;

//評価関数バイナリに書き込むときは排他制御必要だと思う
std::mutex MUTEX;

//L1正則化項にかける係数
static double L1_GRAD_COEFFICIENT;

//勝率と勝敗の負の対数尤度項の係数
static const double COEFFICIENT_OF_WIN_RATE = 10.0;

//学習情報を書き込むファイル
static std::ofstream loss_history;

static std::ofstream explain;

//学習する際の深さ
static Depth learn_depth;

enum Optimizer {
    SGD, ADAGRAD, RMSPROP, ADADELTA
};
static int optimizer_mode;

//学習率
static double learn_rate;

//次に使う棋譜のid
static std::atomic<int> grobal_game_id;

//損失関数にsigmoidを使うかy = axを使うか
static int loss_function_mode;

//シグモイド関数のゲイン
//どのくらいの値にすればいいのかはよくわからないけどとりあえずは http://woodyring.blog.so-net.ne.jp/2015-02-06-1 を参考に
static const double GAIN = 0.0274;

//sigmoidのx = 0における微分値に合わせよう
//static const double linear_coefficient = GAIN / 4;
static const double linear_coefficient = 1.0;

//学習用のdouble型パラメータ
static std::unique_ptr<EvalParams<double>> parameters(new EvalParams<double>);

//勾配
static std::unique_ptr<EvalParams<double>> grad(new EvalParams<double>);

//勾配の平均(Root Mean Square):AdaGrad,RMSProp,AdaDeltaの時に使う.AdaGradの時は二乗の和だけど、まぁ別に用意するのも変な気がするので
static std::unique_ptr<EvalParams<double>> RMSgrad;

//パラメータの移動量の平均(Root Mean Square):AdaDeltaの時に使う
static std::unique_ptr<EvalParams<double>> RMSdelta;

//RMSProp,AdaDeltaの時の減衰指数
static const double DECAY_RATE = 0.9;

//学習した局面の数
static std::atomic<int> learned_position_num;

//学習した棋譜の数
//これがMINI_BATCH_SIZEに達するごとにパラメータを更新し、これを0に戻す
static std::atomic<int> learned_games_num;

//指し手が一致した局面の数
static std::atomic<int> succeeded_position_num;

//損失
static std::atomic<double> Loss;

void BonanzaMethod() {
    std::string path;
    std::cout << "学習する棋譜があるフォルダへのパス : ";
    std::cin >> path;

    int games_size;
    std::cout << "学習する棋譜の数 : ";
    std::cin >> games_size;

    std::cout << "ミニバッチサイズ : ";
    std::cin >> MINI_BATCH_SIZE;

    std::cout << "L1正則化項の係数 : ";
    std::cin >> L1_GRAD_COEFFICIENT;

    //std::cout << "学習する際の深さ : ";
    //std::cin >> learn_depth;
    //深さは1じゃないと現実的な時間で終わらない
    learn_depth = Depth(1);

    std::string optimizer_name;
    while (true) { //Optimizerの選択
        std::cout << "Optimizer(SGD, AdaGrad, RMSProp, AdaDelta から選択):";
        std::cin >> optimizer_name;
        if (optimizer_name == "SGD") {
            optimizer_mode = SGD;
            break;
        } else if (optimizer_name == "AdaGrad") {
            optimizer_mode = ADAGRAD;
            break;
        } else if (optimizer_name == "RMSProp") {
            optimizer_mode = RMSPROP;
            break;
        } else if (optimizer_name == "AdaDelta") {
            optimizer_mode = ADADELTA;
            break;
        } else {
            std::cerr << "Optimizerは[SGD, AdaGrad, RMSProp, AdaDelta]から選択" << std::endl;
        }
    }
    //Adadeltaでは学習率を設定しない
    if (optimizer_mode == ADADELTA) {
        //適当に1にでもしておく
        learn_rate = 1;
    } else {
        std::cout << "学習率           : ";
        std::cin >> learn_rate;
    }

    //std::cout << "損失関数(0:sigmoid, 1:Linear): ";
    //std::cin >> loss_function_mode;
    //Linearで統一
    loss_function_mode = 1;

    std::cout << "スレッド数       : ";
    std::cin >> thread_num;

    std::cout << "start BonanzaMethod" << std::endl;

    //これで待機してた探索用スレッドは終了してくれるはず
    threads.clear();

    grobal_game_id = 0;

    loss_history.open("loss_history.txt", std::ios::out);

    explain.open("explain.txt", std::ios::out);

    //評価関数ロード
    eval_params->readFile();
    std::cout << "eval_params->printHistgram()" << std::endl;
    parameters->copy(eval_params);
    eval_params->printHistgram();

    if (optimizer_mode == ADAGRAD || optimizer_mode == RMSPROP) {
        //AdaGrad, RMSPropならRMSgradだけ準備する
        RMSgrad.reset(new EvalParams<double>());
        RMSgrad->readFile("RMSgrad.bin");
        std::cout << "RMSgrad->printHistgram()" << std::endl;
        RMSgrad->printHistgram();
    } else if (optimizer_mode == ADADELTA) {
        //AdaDeltaならRMSgradとRMSdeltaを準備する
        RMSgrad.reset(new EvalParams<double>());
        RMSgrad->readFile("RMSgrad.bin");
        std::cout << "RMSgrad->printHistgram()" << std::endl;
        RMSgrad->printHistgram();

        RMSdelta.reset(new EvalParams<double>());
        RMSdelta->readFile("RMSdelta.bin");
        std::cout << "RMSdelta->printHistgram()" << std::endl;
        RMSdelta->printHistgram();
    }

    //棋譜を読み込む
    std::cout << "start load_games ...";
    games = load_games(path, games_size);
    std::cout << " done" << std::endl;

    //棋譜シャッフル
    std::random_device rd;
    for (int i = 0; i < games.size(); i++) {
        unsigned int j = rd() % games.size();
        std::swap(games[i], games[j]);
    }

    //勾配初期化
    grad->clear();
    grad->addL1grad(*eval_params, L1_GRAD_COEFFICIENT);

    //学習情報の初期化
    Loss = L1_GRAD_COEFFICIENT * eval_params->abs();
    learned_games_num = 0;
    learned_position_num = 0;
    succeeded_position_num = 0;

    //学習パラメータの設定を書きだしておく
    explain << "最初のabs : " << eval_params->abs() << ", 学習する棋譜の数 : " << games.size() << ", ミニバッチサイズ : " << MINI_BATCH_SIZE << ", L1正則化項の係数 : " << L1_GRAD_COEFFICIENT
        << ", 学習探索の深さ : " << learn_depth << ", 学習率 : " << learn_rate << ", 損失関数 : " <<  loss_function_mode << ", スレッド数 : " << thread_num << ", オプティマイザ : " << optimizer_name << std::endl;

    //lossファイルの各行が何を表すかを示しておく
    loss_history << "SumLoss, L1_NORM, loss_average, Percentage of correct move, eval_params.abs()" << std::endl;
    loss_history << std::fixed;

    //探索が止まらないようにする
    shared_data.stop_signal = false;

    thread_num = std::min(std::max(1u, thread_num), std::thread::hardware_concurrency());
    std::cout << "使用するスレッド数 : " << thread_num << std::endl;
    std::cout << std::fixed;

    std::vector<std::thread> learn_threads(thread_num);
    for (unsigned int i = 0; i < thread_num; ++i) {
        learn_threads[i] = std::thread(BonanzaMethodSlave, i);
    }
    for (auto& t : learn_threads) {
        t.join();
    }

    loss_history.close();
    exit(0);
}

void BonanzaMethodSlave(const int thread_id) {
    //探索クラス:スレーブとして作れば停止はstop_signalだけなので止まらない
    Searcher searcher(Searcher::Role::SLAVE);

    //時間初期化
    auto start = std::chrono::system_clock::now();

    //使用する棋譜のidは各スレッドで共有(std::atomic<int>)
    while (true) {
        //局面を初期化(初期局面でいいはず)
        Position position;

        int game_id = grobal_game_id++;

        if (game_id >= games.size()) {
            break;
        }

        //棋譜
        const Game& game = games[game_id];

        //最後の方は王手ラッシュで意味がないと判断して0.8をかけた手数で打ち切り
        int num_moves = static_cast<int>(game.moves.size() * 0.8);

        //局面を1手ごと進めながら各局面について損失,勾配を計算する
        for (int j = 0; j < num_moves; j++) {
            //全指し手生成
            std::vector<Move> move_list = position.generateAllMoves();

            if (move_list.size() == 0) {
                //おそらく詰みの局面には行かないはずだけど……
                break;
            }

            if (move_list.size() == 1) {
                //可能な指し手が1つしかないなら学習に適してないのでスキップ
                //一手進める
                position.doMove(game.moves[j]);
                continue;
            }

            //教師となる指し手
            const Move teacher_move = game.moves[j];

            //全ての指し手に評価値をつけてPVを設定する
            //この評価値は手番側から見たものだから、教師手が最大になってほしい
            std::vector<std::pair<Move, std::vector<Move>>> move_and_pv;
            for (Move& move : move_list) {
                position.doMove(move);
                searcher.setRoot(position);
                searcher.resetPVTable();
                move.score = -searcher.search<true>(position, MIN_SCORE, MAX_SCORE, learn_depth, 0);
                position.undo();
                std::vector<Move> pv = searcher.pv();
                if (pv.size() == 0) {
                    //ときどき0になってることは確認できるけど、とくにどうしようもなくダメということもないので表示しなくていいかぁ
                    //std::cout << "pv.size() == 0でした" << std::endl;
                }
                pv.insert(pv.begin(), move);
                move_and_pv.emplace_back(move, pv);
            }

            //ソート
            std::sort(move_and_pv.begin(), move_and_pv.end(), std::greater<std::pair<Move, std::vector<Move>>>());

            //教師手を探す
            int teacher_index = -1;
            for (unsigned int k = 0; k < move_and_pv.size(); k++) {
                if (move_and_pv[k].first == teacher_move) {
                    teacher_index = k;
                    break;
                }
            }
            if (teacher_index == -1) {
                //生成された合法手の中に教師手が含まれなかったということ
                //飛・角・歩とかの不成はここに来る
                //学習には適さないのでスキップする
                position.doMove(teacher_move);
                continue;
            } else if (teacher_index == 0) {
                //一致したということ
                learned_position_num++;
                succeeded_position_num++;
                position.doMove(teacher_move);
                continue;
            }

            //教師手を先頭に持ってくる
            std::swap(move_and_pv[0], move_and_pv[teacher_index]);

            //教師手のpvを棋譜のものに変更する
            //これやらないほうがいいのでは
            //for (auto l = 0; l < move_and_pv[0].second.size(); ++l) {
            //    move_and_pv[0].second[l] = game.moves[j + l];
            //}

            //教師手以外をスコアでソート
            std::sort(move_and_pv.begin() + 1, move_and_pv.end(), std::greater<std::pair<Move, std::vector<Move>>>());

            //Lossの本体:第1項 教師手との差
            //教師手のリーフノードの特徴量を用意
#if DEBUG
            printf("教師手のリーフノード----------------------------\n");
#endif
            Color teacher_leaf_color;
            Features teacher_leaf_element = position.makeEvalElements(move_and_pv[0].second, teacher_leaf_color);

            unsigned int fail_move_num = 0;
            double progress = std::min(static_cast<double>(j) / 100.0, 0.85);
            Score margin = Score(10) + (int)(256 * progress);
            //各パラメータについて勾配を計算
            //各指し手についてリーフに出てくる特徴量に対応する勾配を増やし,その分だけ教師手リーフに出てくる特徴量に対応する勾配を減らす
            for (unsigned int m = 1; m < move_and_pv.size(); m++) {
                if (move_and_pv[m].first.score + margin >= move_and_pv[0].first.score) {
                    //どれだけの手が教師手より良いと誤って判断されたのかカウントする
                    fail_move_num++;
                } else {
                    break;
                }
            }
            if (fail_move_num > 0) {
                for (unsigned int m = 1; m < move_and_pv.size(); m++) {
                    auto diff = move_and_pv[m].first.score + margin - move_and_pv[0].first.score;
                    if (diff < 0) { //教師手の方がスコアが良かったということ
                        break;
                    }
                    //atomicだと+=演算子使えないっぽい
                    Loss = Loss + loss_function(diff) / fail_move_num;
#if DEBUG
                    printf("\nダメ手のリーフノード\n");
                    printf("スコアの差 = %d\n", move_and_pv[m].first.score - move_and_pv[0].first.score);
#endif
                    //勾配を変化させる量は損失を微分した値 / ダメだった手の数
                    double delta = d_loss_function(diff) / fail_move_num;
                    if (position.color() == WHITE) {
                        delta = -delta;
                    }

                    //リーフノードの特徴量
                    Color leaf_color;
                    Features leaf_element = position.makeEvalElements(move_and_pv[m].second, leaf_color);

                    MUTEX.lock();
                    //高く評価しすぎた手について勾配を増やす
                    grad->updateGradient(leaf_element, delta);
                    //教師手について勾配を減らす
                    grad->updateGradient(teacher_leaf_element, -delta);
                    MUTEX.unlock();
                }
            }

            //Lossの本体:第2項 勝率と勝敗の負の対数尤度
            //予測勝率をp, 1回の試行で先手が勝った回数をy(ようは先手が勝ったかどうか)とすると
            //尤度 = 1Cw * p^y + (1 - p)^(1 - y)
            //最小化問題に帰着させたいので負にする、累乗は扱いにくいので対数

            if (false) {

                //いわゆるponanza定数
                const double GAIN_OF_WIN_RATE = 1 / 600.0;

                //この局面を探索した評価値
                searcher.setRoot(position);
                searcher.resetPVTable();
                int score = searcher.NegaAlphaBeta<true>(position, MIN_SCORE, MAX_SCORE, Depth(3), 0);

                //先手から見た勝率に変換したいので先手から見たスコアにする
                score *= position.color() == BLACK ? 1 : -1;

                //あまりにも大きい評価値がでる局面は学習に適してないだろう
                if (std::abs(score) >= 10000) break;

                //std::printf("score = %d, ", score);
                std::vector<Move> pv = searcher.pv();
                double p = sigmoid(score, GAIN_OF_WIN_RATE);
                int y = game.winner;
                Loss = Loss + COEFFICIENT_OF_WIN_RATE * (-(y * std::log(p) + (1 - y) * std::log(1.0 - p)));

                //勾配を計算
                double grad_of_win_rate = COEFFICIENT_OF_WIN_RATE * GAIN_OF_WIN_RATE * (p - y);
#if DEBUG
                printf("grad_of_win_rate = %f\n", grad_of_win_rate);
#endif

                //リーフノードで出る特徴量に勾配を足しこむ
                Color leaf_color;
                Features feature_for_win_rate = position.makeEvalElements(pv, leaf_color);
                //下のlead_colorに応じて変えないといけない
                MUTEX.lock();
                grad->updateGradient(feature_for_win_rate, grad_of_win_rate);
                MUTEX.unlock();
            }

            //学習した局面を増やす
            learned_position_num++;

            //一手進める
            position.doMove(game.moves[j]);

        } //ここで1局分が終わる

        if ((game_id + 1) <= 50 || (game_id + 1) % 10 == 0) {
            auto time = std::chrono::system_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(time - start);
            MUTEX.lock();
            printf("finish %5d games, 経過時間:%3ld時間%3ld分, ", game_id + 1, elapsed.count() / 60, elapsed.count() % 60);
            int r_minutes = static_cast<int>((double)(games.size() - (game_id + 1)) / (game_id + 1) * elapsed.count());
            printf("残り時間:%3d時間%3d分\n", r_minutes / 60, r_minutes % 60);
            MUTEX.unlock();
        }

        //必要局数に達していたらパラメータ更新
        if (++learned_games_num == MINI_BATCH_SIZE) {
            std::unique_lock<std::mutex> lock(MUTEX);
            update();
        }
    }

    //最後にあまりが残っていると思う
    std::cout << "learned_games_num = " << learned_games_num << std::endl;

    //必要局数の8割くらいやってたらもったいないからパラメータ更新する
    if (learned_games_num >= MINI_BATCH_SIZE * 0.8) {
        update();
    }

    std::cout << "BonanzaMethod finish" << std::endl;
}

void update() {
    //double型の学習用重みをアップデート
    if (optimizer_mode == SGD) {
        parameters->updateParamsSGD(grad, learn_rate);
    } else if (optimizer_mode == ADAGRAD) {
        //2乗和を足す
        RMSgrad->addSquare(grad);
        //重み更新
        parameters->updateParamsAdaGrad(grad, RMSgrad, learn_rate);
        //RMSgradを書き出す
        RMSgrad->writeFile("RMSgrad.bin");
    } else if (optimizer_mode == RMSPROP) {
        //減衰させながら足す
        RMSgrad->calcRMS(grad, DECAY_RATE);
        //更新の式はAdaGradと同じ
        parameters->updateParamsAdaGrad(grad, RMSgrad, learn_rate);
        //RMSgradを書き出す
        RMSgrad->writeFile("RMSgrad.bin");
    } else if (optimizer_mode == ADADELTA) {
        //減衰させながら足す
        RMSgrad->calcRMS(grad, DECAY_RATE);
        //重み更新:内部でRMSdeltaも更新される
        parameters->updateParamsAdaDelta(grad, RMSgrad, RMSdelta, DECAY_RATE);
        //RMS系を書き出しておく
        RMSgrad->writeFile("RMSgrad.bin");
        RMSdelta->writeFile("RMSdelta.bin");
    }
    //int型の計算用重みにコピーして書き出し
    eval_params->copy(parameters);
    eval_params->writeFile();

    //学習情報を出力
    std::cout << "SumLoss = " << Loss << ", L1_PENALTY = " << L1_GRAD_COEFFICIENT * eval_params->abs() << ", loss_average = " << Loss / learned_position_num
        << ", Percentage of correct move = " << (double)succeeded_position_num / learned_position_num * 100 << ", grad.max_abs() = " << grad->max_abs()
        << ", eval_params->abs() = " << eval_params->abs() << std::endl;
    eval_params->printHistgram();

    loss_history << Loss << " \t" << L1_GRAD_COEFFICIENT * eval_params->abs() << "\t" << Loss / learned_position_num
        << "\t" << (double)succeeded_position_num / learned_position_num * 100 << "\t" << eval_params->abs() << std::endl;

    //勾配初期化
    grad->clear();
    grad->addL1grad(*eval_params, L1_GRAD_COEFFICIENT);

    //学習情報を0に戻す
    Loss = L1_GRAD_COEFFICIENT * eval_params->abs();
    learned_games_num = 0;
    learned_position_num = 0;
    succeeded_position_num = 0;
}

double loss_function(int score_diff) {
    //どれかを利用する
    if (loss_function_mode == 0) {
        //sigmoid
        return sigmoid(score_diff, GAIN);
    } else if (loss_function_mode == 1) {
        //線形
        return linear_coefficient * score_diff;
    } else {
        //ここには来ないはず
        assert(false);
        return 0.0;
    }
}

double d_loss_function(int score_diff) {
    //どれかを利用する
    if (loss_function_mode == 0) {
        //sigmoid
        return d_sigmoid(score_diff, GAIN);
    } else if (loss_function_mode == 1) {
        //線形
        return linear_coefficient;
    } else {
        //ここには来ないはず
        assert(false);
        return 0.0;
    }
}