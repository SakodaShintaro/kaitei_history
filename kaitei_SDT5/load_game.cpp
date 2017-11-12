#include"load_game.hpp"
#include"position.hpp"
#include<iostream>
#include<algorithm>
#include<string>
#include<unordered_map>
#include<experimental/filesystem>

namespace sys = std::experimental::filesystem;

static std::unordered_map<std::string, Piece> CSAstringToPiece = {
	{ "FU", PAWN },
	{ "KY", LANCE },
	{ "KE", KNIGHT },
	{ "GI", SILVER },
	{ "KI", GOLD },
	{ "KA", BISHOP },
	{ "HI", ROOK },
	{ "OU", KING },
	{ "TO", PAWN_PROMOTE },
	{ "NY", LANCE_PROMOTE },
	{ "NK", KNIGHT_PROMOTE },
	{ "NG", SILVER_PROMOTE },
	{ "UM", BISHOP_PROMOTE },
	{ "RY", ROOK_PROMOTE },
};

Game load_game_from_csa(sys::path p) {
    Position pos;
	Game game;
	game.path = p;
	std::ifstream ifs(p);
	std::string buf;
    int move_counter = 1;
	while (getline(ifs, buf)) {
		if (buf[0] == '\'') continue;
		if (buf[0] != '+' && buf[0] != '-') continue;
		if (buf.size() == 1) continue;

		//上の分岐によりMoveだけ残る
		Square from = FRToSquare[buf[1] - '0'][buf[2] - '0'];
		Square to   = FRToSquare[buf[3] - '0'][buf[4] - '0'];
        //移動駒の種類
		Piece subject = CSAstringToPiece[buf.substr(5, 2)];
        //手番を設定
        subject = (pos.color() == BLACK ? toBlack(subject) : toWhite(subject));
        bool isDrop = (from == WALL00);

        //CSAのフォーマットから、動くものが成済なのにfromにある駒が不成の場合、成る手
        bool isPromote = ((subject & PROMOTE) && !(pos.on(from) & PROMOTE));
        if (isPromote) {
            subject = static_cast<Piece>(subject & ~PROMOTE);
        }
        Move move(to, from, isDrop, isPromote, subject);
        move = pos.transformValidMove(move);

        if (!pos.isLegalMove(move)) {
            pos.printForDebug();
            move.print();
            pos.isLegalMove(move);
            std::cout << game.path << std::endl;
            assert(false);
        }
		game.moves.push_back(move);
        pos.doMove(move);

	}
	game.winner = ~pos.color();
	return game;
}

std::vector<Game> load_games(std::string path, int num) {
    const sys::path dir(path);
	std::vector<Game> games;
    for (sys::directory_iterator p(dir); p != sys::directory_iterator(); p++) {
        games.push_back(load_game_from_csa(p->path()));
        if (--num == 0)
            break;
    }
	return games;
}

void clean_games(std::string path) {
    const sys::path dir(path);
	for (sys::directory_iterator p(dir); p != sys::directory_iterator(); p++) {
		std::ifstream ifs(p->path());
		std::string buf;
		int move_counter = 0;
        double black_rate = 0, white_rate = 0;
		while (getline(ifs, buf)) {
			//レート読み込み
			//2800より小さかったら削除
			if (buf.find("'black_rate") < buf.size()) {
				black_rate = std::stod(buf.substr(buf.rfind(':') + 1));
                continue;
			} else if (buf.find("'white_rate") < buf.size()) {
				white_rate = std::stod(buf.substr(buf.rfind(':') + 1));
				continue;
			}

			//summaryが投了以外のものは削除
			if (buf.find("summary") < buf.size()) {
				if (buf.find("toryo") > buf.size()) {
					ifs.close();
					std::cout << p->path().string().c_str() << " ";
					if (remove(p->path().string().c_str()) == 0) {
						std::cout << "削除成功" << std::endl;
					} else {
						std::cout << "失敗" << std::endl;
					}
					goto END;
				}
				continue;
			}

			//std::cout << buf << std::endl;
			if (buf[0] == '\'') continue;
			if (buf[0] != '+' && buf[0] != '-') continue;
			if (buf.size() == 1) continue;
			move_counter++;
		}

        if (black_rate < 2800) {
            ifs.close();
            std::cout << p->path().string().c_str() << " ";
            if (remove(p->path().string().c_str()) == 0) {
                std::cout << "削除成功" << std::endl;
            } else {
                std::cout << "失敗" << std::endl;
            }
            continue;
        }
        if (white_rate < 2800) {
            ifs.close();
            std::cout << p->path().string().c_str() << " ";
            if (remove(p->path().string().c_str()) == 0) {
                std::cout << "削除成功" << std::endl;
            } else {
                std::cout << "失敗" << std::endl;
            }
            continue;
        }

		//手数が50より小さいものはおかしい気がするので削除
		if (move_counter < 50) {
			ifs.close();
			std::cout << p->path().string().c_str() << " ";
			if (remove(p->path().string().c_str()) == 0) {
				std::cout << "削除成功" << std::endl;
			} else {
				std::cout << "失敗" << std::endl;
			}
            continue;
		}

	END:;
	}
}
