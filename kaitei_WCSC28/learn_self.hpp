#pragma once
#include"types.hpp"

void learnSelf();
void learnSelfSlave(int id);
double calcGrad(Score shallow_score, Score deep_score, double result, int curr_turn_num, int finish_turn_num);
std::pair<double, double> calcLoss(Score shallow_score, Score deep_score, double result);