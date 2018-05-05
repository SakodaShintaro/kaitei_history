#ifndef USI_HPP
#define USI_HPP

#include"common.hpp"
#include"position.hpp"
#include"searcher.hpp"
#include"move.hpp"
#include"thread.hpp"
#include<string>

class USI {
public:
    void loop();
    void usi();
    void isready();
    void setoption();
    void usinewgame();
    void position();
    void go();
    void stop();
    void ponderhit();
    void quit();
    void gameover();
};

#endif
