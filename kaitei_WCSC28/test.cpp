#include"test.hpp"
#include"usi.hpp"
#include"piece.hpp"
#include"square.hpp"
#include"position.hpp"
#include"hand.hpp"
#include"load_game.hpp"
#include<cassert>

void testInvPieceState() {
    for (auto c : { BLACK, WHITE }) {
        for (auto p : { PAWN, LANCE, KNIGHT, SILVER, GOLD, BISHOP, ROOK }) {
            auto ps = pieceState(p, 3, c);
            std::cout << ps << " の逆 " << invPieceState(ps) << std::endl;
        }
    }
}

void testHand() {
	Hand h;
	h.set(PAWN, 10);
	assert(h.num(PAWN) == 10);
	h.set(LANCE, 3);
	assert(h.num(LANCE) ==3);
	h.set(KNIGHT, 2);
	assert(h.num(KNIGHT) == 2);
	h.set(SILVER, 1);
	assert(h.num(SILVER) == 1);
}

void testSFEN() {
    std::string sfen1 = "1nsgsk2l/l8/1pppp1+R1n/p4pB1p/9/2P1P4/PP1P1PP1P/1B1S5/LN1GKGSNL w RG3P 1";
    std::cout << sfen1 << std::endl;
    Position p;
    p.load_sfen(sfen1);
    p.printForDebug();
    std::string sfen2 = "lnsgkgsn1/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL w - 1";
    std::cout << sfen2 << std::endl;
    p.load_sfen(sfen2);
    p.printForDebug();
}

void testVectorSpeed() {
    constexpr int N = 100000;
    int normal_array[N];
    std::array<int, N> st_array;
    std::vector<int> vec1(N), vec2;

    std::cout << "write" << std::endl;
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        normal_array[i] = i;
    }
    auto end = std::chrono::steady_clock::now();
    auto elapsed = end - start;
    std::cout << "normal_array:" << elapsed.count() << std::endl;

    start = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        st_array[i] = i;
    }
    end = std::chrono::steady_clock::now();
    elapsed = end - start;
    std::cout << "st_array[]  :" << elapsed.count() << std::endl;

    start = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        st_array.at(i) = i;
    }
    end = std::chrono::steady_clock::now();
    elapsed = end - start;
    std::cout << "st_arrayat  :" << elapsed.count() << std::endl;

    start = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        vec1[i] = i;
    }
    end = std::chrono::steady_clock::now();
    elapsed = end - start;
    std::cout << "vector      :" << elapsed.count() << std::endl;

    start = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        vec2.push_back(i);
    }
    end = std::chrono::steady_clock::now();
    elapsed = end - start;
    std::cout << "vector.push :" << elapsed.count() << std::endl;

    std::cout << "read" << std::endl; //-------------------------------------------------------------------------------------
    long long sum = 0;

    int read_array[N];
    for (int i = 0; i < N; ++i) {
        read_array[i] = i;
    }
    std::random_device seed_gen;
    std::mt19937 engine(seed_gen());
    std::shuffle(read_array, read_array + N, engine);

    start = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        sum += normal_array[read_array[i]];
    }
    end = std::chrono::steady_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout << "normal_array:" << elapsed.count() << std::endl;

    sum = 0;
    start = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        sum += st_array[read_array[i]];
    }
    end = std::chrono::steady_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout << "st_array[]  :" << elapsed.count() << std::endl;

    sum = 0;
    start = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        sum += st_array.at(read_array[i]);
    }
    end = std::chrono::steady_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout << "st_arrayat  :" << elapsed.count() << std::endl;

    sum = 0;
    start = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        sum += vec1[read_array[i]];
    }
    end = std::chrono::steady_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout << "vector      :" << elapsed.count() << std::endl;

    int size = 10;
    int* a = new int[size];
    auto p = new std::array<int, PieceStateNum>;
    (*p)[5] = 5;
}