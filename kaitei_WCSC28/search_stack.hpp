#pragma once
#ifndef SEARCH_STACK_HPP
#define SEARCH_STACK_HPP

#include"move.hpp"
#include"position.hpp"
#include"history.hpp"

struct SearchStack {
    Move killers[2];
    bool can_null_move;

    void updateKillers(const Move& move) {
        //cf. https://qiita.com/ak11/items/0c1d20753b1073788275
        if (killers[0] != move) {
            killers[1] = killers[0];
            killers[0] = move;
//#if DEBUG
//            std::cout << "killers are updated" << std::endl;
//            std::cout << "killers[0] = ";
//            killers[0].print();
//            std::cout << "killers[1] = ";
//            killers[1].print();
//#endif
        }
    }
};

#endif // !SEARCH_STACK_HPP