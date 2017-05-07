#ifndef USI_HPP
#define USI_HPP

#include"common.hpp"
#include"position.hpp"
#include"search.hpp"
#include"move.hpp"
#include<string>

class USIGoOption;
class USIinfo;

class USI{
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
private:
    Position position_;
    Searcher searcher_;
};

Move stringToMove(std::string input);

#endif
