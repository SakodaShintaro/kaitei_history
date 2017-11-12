#include"usi.hpp"
#include"move.hpp"
#include"position.hpp"
#include"searcher.hpp"
#include"usi_options.hpp"
#include"learn.hpp"
#include"shared_data.hpp"
#include"eval_params.hpp"
#include"load_game.hpp"
#include<iostream>
#include<string>
#include<thread>

USIOption usi_option;

void USI::loop() {
    std::string input;
    while (true) {
        std::cin >> input;
        if (input == "usi") {
            usi();
        } else if (input == "isready") {
            isready();
        } else if (input == "setoption") {
            setoption();
        } else if (input == "usinewgame") {
            usinewgame();
        } else if (input == "position") {
            position();
        } else if (input == "go") {
            go();
        } else if (input == "stop") {
            stop();
        } else if (input == "ponderhit") {
            ponderhit();
        } else if (input == "quit") {
            quit();
        } else if (input == "gameover") {
            gameover();
        } else if (input == "prepareForLearn") {
            prepareForLearn();
        } else if (input == "cleanGame") {
            std::cout << "棋譜のあるフォルダへのパス : ";
            std::string file_path;
            std::cin >> file_path;
            clean_games(file_path);
        } else if (input == "BonanzaMethod") {
            BonanzaMethod();
        } else if (input == "changeEvalParams") {
            eval_params.readFile();
            std::cout << "eval_params.abs() = " << eval_params.abs() << std::endl;
            double scale;
            std::cin >> scale;
            eval_params *= scale;
            std::cout << "eval_params.abs() = " << eval_params.abs() << std::endl;
            eval_params.writeFile();
        } else if (input == "printEvalParams") {
            eval_params.readFile();
            eval_params.printHistgram();
            eval_params.printParamsSorted(std::cout);
        } else {
            std::cout << "Illegal input" << std::endl;
        }
    }
}

void USI::usi() {
    printf("id name kaitei_SDT5\n");
    printf("id author Sakoda Shintaro\n");
	printf("option name byoyomi_margin type spin default 500 min 0 max 1000\n");
	printf("option name random_turn type spin default 0 min 0 max 100\n");
	printf("usiok\n");
}

void USI::isready() {
    shared_data.hash_table.setSize(usi_option.USI_Hash);
    eval_params.readFile();
    printf("readyok\n");
}

void USI::setoption() {
    std::string input;
    while (true) {
        std::cin >> input;
        if (input == "name") {
            std::cin >> input;
			//ここで処理
			if (input == "byoyomi_margin") {
				std::cin >> input; //input == "value"となるなず
				std::cin >> usi_option.byoyomi_margin;
				return;
			} else if (input == "random_turn") {
				std::cin >> input; //input == "value"となるなず
				std::cin >> usi_option.random_turn;
				return;
            } else if (input == "USI_Hash") {
                std::cin >> input; //input == "value"となるはず
                std::cin >> usi_option.USI_Hash;
                return;
            } else if (input == "USI_Ponder") {
                std::cin >> input; //input == "value"となるなず
                std::cin >> input; //特になにもしていない
                return;
            }
        }
    }
}

void USI::usinewgame() {
    //なにかやるべきことあるんだろうか
    //置換表のリセットか
    shared_data.hash_table.clear();
    shared_data.hash_table.setSize(usi_option.USI_Hash);

    //PositionのコンストラクタがBitboard類の初期化より前に呼ばれてしまうので
    //ここで呼びなおさないとおかしくなる
    shared_data.root = Position();
}

void USI::position() {
    //rootを初期化
    shared_data.root = Position();

    std::string input, sfen;
    std::cin >> input;
    if (input == "startpos") {
        sfen = "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1";
    } else {
        for (int i = 0; i < 4; i++) {
            std::cin >> input;
            sfen += input;
            sfen += " ";
        }
    }
    shared_data.root.load_sfen(sfen);

    std::cin >> input;  //input == "moves" or "go"となる
    if (input != "go") {
        while (std::cin >> input) {
            if (input == "go") {
                break;
            }
            //inputをMoveに直して局面を動かす
            Move move = stringToMove(input);
            move = shared_data.root.transformValidMove(move);
            shared_data.root.doMove(move);
        }
    }

    go();
}

void USI::go() {
    shared_data.stop_signal = false;
    std::string input;
	long long btime, wtime, byoyomi = 0, binc = 0, winc = 0;
    std::cin >> input;
    if (input == "ponder") {
        //ponderの処理
    } else if (input == "btime") {
        std::cin >> input;
        btime = atoi(input.c_str());
        std::cin >> input; //input == "wtime" となるはず
        std::cin >> input;
        wtime = atoi(input.c_str());
        std::cin >> input; //input == "byoyomi" or "binc"となるはず
        if (input == "byoyomi") {
            std::cin >> input;
            byoyomi = atoi(input.c_str());
        } else {
            std::cin >> input;
            binc = atoi(input.c_str());
            std::cin >> input; //input == "winc" となるはず
            std::cin >> input;
            winc = atoi(input.c_str());
        }
        //ここまで持ち時間の設定
        //ここから思考部分
        if (byoyomi != 0) {
            shared_data.limit_msec = byoyomi;
        } else {
            shared_data.limit_msec = binc;
        }
        threads[0]->startSearch();
        return;
    } else if (input == "infinite") {
        //stop来るまで思考し続ける
        shared_data.limit_msec = LLONG_MAX;
        usi_option.random_turn = 0;
		threads[0]->startSearch();
    } else if (input == "mate") {
        //詰み探索
        std::cin >> input;
        if (input == "infinite") {
            //stop来るまで
        } else {
            //思考時間が指定された場合
            //どう実装すればいいんだろう
        }
    }
}

void USI::stop() {
    shared_data.stop_signal = true;
    for (auto& t : threads) {
        t->waitForFinishSearch();
    }
}

void USI::ponderhit() {
    //まだ未実装
}

void USI::quit() {
    stop();
    exit(0);
}

void USI::gameover() {
    std::string input;
    std::cin >> input;
    if (input == "win") {
        //勝ち
    } else if (input == "lose") {
        //負け
    } else if (input == "draw") {
        //引き分け
		return;
	}
}