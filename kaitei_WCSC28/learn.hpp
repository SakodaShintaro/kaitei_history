#pragma once

#ifndef LEARN_HPP
#define LEARN_HPP

#include"piece.hpp"
#include"common.hpp"
#include"piece_state.hpp"

double loss_function(int score_diff);
double d_loss_function(int score_diff);

//sigmoidを取って評価値の差から更新する量を計算する
void BonanzaMethod();

//並列化するとき各スレッドが実行する関数
void BonanzaMethodSlave(const int thread_id);

void update();

#endif // !LEARN_HPP
