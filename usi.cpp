#include"usi.hpp"
#include"move.hpp"
#include"position.hpp"
#include"search.hpp"
#include<iostream>
#include<string>

static unsigned int fixed_depth;

void USI::loop()
{
    std::string input;
    while (1) {
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
		}
    }
}

void USI::usi()
{
    printf("id name kaitei_WCSC27\n");
    printf("id author Sakoda Shintaro\n");
	printf("option name byoyomi_margin type spin default 800 min 0 max 1000\n");
	printf("option name random_turn type spin default 0 min 0 max 100\n");
	printf("option name fixed_depth type spin default 0 min 0 max 100\n");
    printf("usiok\n");
}

void USI::isready()
{
    printf("readyok\n");
}

void USI::setoption()
{
    std::string input;
    while (true) {
        std::cin >> input;
        if (input == "name") {
            std::cin >> input;
			//ここで処理
			if (input == "byoyomi_margin") {
				std::cin >> input; //input == "value"となるなず
				long long byoyomi_margin;
				std::cin >> byoyomi_margin;
				searcher_.setByoyomiMargin(byoyomi_margin);
				return;
			} else if (input == "random_turn") {
				std::cin >> input; //input == "value"となるなず
				unsigned int random_turn;
				std::cin >> random_turn;
				searcher_.setRandomTurn(random_turn);
				return;
			} else if (input == "fixed_depth") {
				std::cin >> input; //input == "value"となるはず
				std::cin >> fixed_depth;
				return;
			}
        }
    }
}

void USI::usinewgame()
{
    //なにかやるべきことあるんだろうか
}

void USI::position()
{
	std::printf("start position\n");
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
	std::printf("before load_sfen\n");
    position_.load_sfen(sfen);
    std::cin >> input;  //input == "moves" or "go"となる
    if (input != "go")
    {
        while (std::cin >> input) {
            if (input == "go") {
                break;
            }
            //inputをMoveに直して局面を動かす
            Move move = stringToMove(input);
            position_.doMove(move);
        }
    }
    go();
}

void USI::go()
{
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
		if (fixed_depth > 0) {
			searcher_.think_fixed_depth(position_, fixed_depth);
			return;
		}
		if (byoyomi != 0) searcher_.think(position_, byoyomi);
		else searcher_.think(position_, binc);
        return;
    } else if (input == "infinite") {
        //stop来るまで思考し続ける
		searcher_.think(position_, 9999999999999999999);
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

void USI::stop()
{
    //どうすればいいのか考え中
}

void USI::ponderhit()
{
    //まだ未実装
}

void USI::quit()
{
    exit(0);
}

void USI::gameover()
{
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

Move stringToMove(std::string input)
{
	std::map<char, Piece> charToPiece = {
		{'P', PAWN},
		{'L', LANCE},
		{'N', KNIGHT},
		{'S', SILVER},
		{'G', GOLD},
		{'B', BISHOP},
		{'R', ROOK},
	};
	Square to, from;
	bool isDrop, isPromote;
	Piece subject;
    if ('A' <= input[0] && input[0] <= 'Z') { //持ち駒を打つ手
        to = FRToSquare[input[2] - '0'][input[3] - 'a' + 1];
		isDrop = true;
		isPromote = false;
		subject = charToPiece[input[0]];
		return dropMove(to, subject);
    } else { //盤上の駒を動かす手
        from = FRToSquare[input[0] - '0'][input[1] - 'a' + 1];
        to   = FRToSquare[input[2] - '0'][input[3] - 'a' + 1];
		isDrop = false;
        if (input.size() == 5 && input[4] == '+') isPromote = true;
		else isPromote = false;
		return fullMove(to, from, isDrop, isPromote, EMPTY, EMPTY);
    }
}
