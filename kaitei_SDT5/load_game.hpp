#pragma once

#ifndef LOAD_GAME_HPP
#define LOAD_GAME_HPP

#include"move.hpp"
#include<string>
#include<vector>
#include<experimental/filesystem>

struct Game {
	//初期局面:どうせ普通の初期局面から始まった棋譜しか使わないし要らない気がする
	std::string sfen_start_pos;

	//指し手のvector
	std::vector<Move> moves;

	//ファイル名を表示するのにしか使ってない
	std::experimental::filesystem::path path;
	
	//先手が勝ちなら1,後手が勝ちなら0
	//学習で使うときの都合でそうなっているけど、そう加工するのは使うときにするべきであって、ここではColor型で保持するのが筋かもしれない
	int winner;
};

Game load_game_from_csa(std::experimental::filesystem::path p);
std::vector<Game> load_games(std::string path, int num);
void clean_games(std::string path);

#endif // !LOAD_GAME_HPP