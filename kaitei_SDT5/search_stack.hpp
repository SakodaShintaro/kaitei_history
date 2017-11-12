#pragma once
#ifndef SEARCH_STACK_HPP
#define SEARCH_STACK_HPP

#include"move.hpp"
#include"position.hpp"
#include"history.hpp"

struct SearchStack {
    Move killers[2];
    void updateKillers(const Move& move) {
        //cf. https://qiita.com/ak11/items/0c1d20753b1073788275
        if (killers[0] != move) {
            killers[1] = killers[0];
            killers[0] = move;
        }
    }
};

#endif // !SEARCH_STACK_HPP