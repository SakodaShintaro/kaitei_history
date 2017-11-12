#pragma once
#ifndef EVAL_PARAMS_HPP
#define EVAL_PARAMS_HPP

#include"piece.hpp"
#include"eval_elements.hpp"
#include"square.hpp"
#include<climits>
#include<cmath>
#include<map>

//評価関数のパラメータをまとめたもの
//インスタンスとしては実際のパラメータ(int)と、学習時の勾配(float?)がありえる

template<typename T>
class EvalParams {
private:
    std::vector<std::vector<T>> pp_;
    std::vector<std::vector<T>> kp_;
public:
    EvalParams();
    void updateParams(const Features &ee, const T delta);
    void updateParams(const EvalParams<double> &grad, const double learn_rate);
    void clear();
    void readFile(std::string file_name_pp = "pp_table.bin", std::string file_name_kp = "kp_table.bin");
    void writeFile(std::string file_name_pp = "pp_table.bin", std::string file_name_kp = "kp_table.bin");
    T abs() const;
    inline T pp(int p1, int p2) const {
        return (p1 >= p2 ? pp_[p1][p2] : pp_[p2][p1]);
    }
    inline T& pp(int p1, int p2) {
        return (p1 >= p2 ? pp_[p1][p2] : pp_[p2][p1]);
    }

	T kp(int k, int p) const { return kp_[k][p]; }

    void rand() {
        int i = 0;
        forEach([&](T& value) {
            value = i++;
        });
    }

    T max_value() {
        T max_val = INT_MIN;
        forEach([&](T value) {
            max_val = std::max(max_val, std::abs(value));
        });
        return max_val;
    }

    //すべてのパラメータに同じ操作をする場合これを使って書けばいいのでは
    template<typename Function>
    void forEach(Function f) {
        for (int p = 0; p < PieceStateNum; p++) {
            for (int k = 0; k < 81; k++) {
                f(kp_[k][p]);
            }
            for (int p2 = p; p2 < PieceStateNum; p2++) {
                f(pp(p, p2));
            }
        }
    }

    EvalParams& operator*=(double rhs) {
        forEach([&](T& value) {
            value = static_cast<T>(value * rhs);
        });
        return *this;
    }

    void addL1grad(const EvalParams<int>& eval_params, const double coefficient) {
        //これはforEachを使っては書けない気がするなぁ
        for (int p = 0; p < PieceStateNum; p++) {
            for (int k = 0; k < 81; k++) {
                if (eval_params.kp(k, p) == 0)
                    continue;
                kp_[k][p] += (eval_params.kp(k, p) > 0 ? coefficient : -coefficient);
            }
            for (int p2 = p; p2 < PieceStateNum; p2++) {
                if (eval_params.pp(p, p2) == 0)
                    continue;
                pp(p, p2) += (eval_params.pp(p, p2) > 0 ? coefficient : -coefficient);
            }
        }
    }

    bool empty() {
        return (kp_.size() == 0 || pp_.size() == 0);
    }

    void printHistgram() {
        //ヒストグラムを取ってみよう
        std::map<int, int> histgram;
        forEach([&](T value) {
            histgram[static_cast<int>(value)]++;
        });
        for (auto e : histgram) {
            std::cout << e.first << ":" << e.second << ", ";
        }
        std::cout << std::endl;
    }

    void printBigParams(T threshold, std::ostream& os) const {
        for (int p1 = 0; p1 < PieceStateNum; p1++) {
            for (int p2 = p1; p2 < PieceStateNum; p2++) {
                if (std::abs(pp(p1, p2)) >= threshold) {
                    os << "pp[" << PieceState(p1) << "][" << PieceState(p2) << "] = " << pp(p1, p2) << std::endl;
                }
            }
            for (int k = 0; k < 81; k++) {
                if (std::abs(kp(k, p1)) >= threshold) {
                    os << "kp[" << SquareList[k] << "][" << PieceState(p1) << "] = " << kp(k, p1) << std::endl;
                }
            }
        }
    }

    void printParamsSorted(std::ostream& os) {
        for (auto compare = max_value(); compare >= -max_value(); --compare) {
            if (compare == 0) {
                continue;
            }
            for (int p1 = 0; p1 < PieceStateNum; p1++) {
                for (int p2 = p1; p2 < PieceStateNum; p2++) {
                    if (pp(p1, p2) == compare) {
                        os << "pp[" << PieceState(p1) << "][" << PieceState(p2) << "] = " << pp(p1, p2) << std::endl;
                    }
                }
                for (int k = 0; k < 81; k++) {
                    if (kp(k, p1) == compare) {
                        os << "kp[" << SquareList[k] << "][" << PieceState(p1) << "] = " << kp(k, p1) << std::endl;
                    }
                }
            }
        }
    }
};

template<typename T>
EvalParams<T>::EvalParams() {
    //kp_は普通に、pp_は三角行列として必要な分だけ取る
    //pp_[p1][p2]はp1 >= p2であることが前提となる
    pp_ = std::vector<std::vector<T>>(PieceStateNum);
    kp_ = std::vector<std::vector<T>>(81, std::vector<T>(PieceStateNum));
    for (int p = 0; p < PieceStateNum; p++) {
        for (int k = 0; k < 81; k++) {
            kp_[k][p] = 0;
        }

        //pp_[p]は長さpのvector
        pp_[p] = std::vector<T>(p + 1, 0);
    }
}

template<typename T>
void EvalParams<T>::updateParams(const Features &ee, const T delta) {
    //eeに出てくる特徴量に関わるパラメータすべてをdeltaだけ変える
    for (unsigned int i = 0; i < ee.piece_state_list.size(); i++) {
        kp_[ee.black_king_sq][ee.piece_state_list[i]] += delta;
        kp_[ee.white_king_sq_reversed][invPieceState(ee.piece_state_list[i])] -= delta;
        for (unsigned int j = i; j < ee.piece_state_list.size(); j++) {
            pp(ee.piece_state_list[i], ee.piece_state_list[j]) += delta;
        }
    }
}

template<typename T>
void EvalParams<T>::updateParams(const EvalParams<double> & grad, const double learn_rate) {
    double sum_of_delta = 0, max_delta = 0;

    //ヒストグラムを取ってみよう
    std::map<int, int> histgram;

    for (int p1 = 0; p1 < PieceStateNum; p1++) {
        for (int p2 = p1; p2 < PieceStateNum; p2++) {
            double delta = (learn_rate * grad.pp(p1, p2));

            //四捨五入
            delta = std::round(delta);

            max_delta = std::max(max_delta, std::fabs(delta));

            histgram[static_cast<int>(delta)]++;

            //パラメータ更新
            pp(p1, p2) -= static_cast<T>(delta);

            //合計いくら変わったか見るため
            sum_of_delta += std::fabs(delta);
        }
        for (int k = 0; k < 81; k++) {
            double delta = (learn_rate * grad.kp(k, p1));

            //四捨五入
            delta = std::round(delta);

            max_delta = std::max(max_delta, std::abs(delta));

            histgram[static_cast<int>(delta)]++;

            //パラメータ更新
            kp_[k][p1] -= static_cast<T>(delta);

            //合計いくら変わったか見るため
            sum_of_delta += std::fabs(delta);
        }
    }
    std::cout << "delta = " << sum_of_delta << ", max_delta_abs = " << max_delta << std::endl;

    for (auto e : histgram) {
        std::cout << e.first << ":" << e.second << ", ";
    }
    std::cout << std::endl;
}

template<typename T>
void EvalParams<T>::clear() {
    for (int p1 = 0; p1 < PieceStateNum; p1++) {
        for (int p2 = p1; p2 < PieceStateNum; p2++) {
            pp(p1, p2) = 0;
        }
        for (int k = 0; k < 81; k++) {
            kp_[k][p1] = 0;
        }
    }
}

template<typename T>
void EvalParams<T>::readFile(std::string file_name_pp, std::string file_name_kp) {
    printf("start readEvalTable\n");

    //pp
    std::ifstream ifs_pp(file_name_pp, std::ios::binary);
    if (ifs_pp.fail()) {
        std::cout << file_name_pp << " cannot open (mode r)" << std::endl;
        return;
    }

    //kp
    std::ifstream ifs_kp(file_name_kp, std::ios::binary);
    if (ifs_kp.fail()) {
        std::cout << file_name_kp << " cannot open (mode r)" << std::endl;
        return;
    }

    for (int p1 = 0; p1 < PieceStateNum; p1++) {
        for (int p2 = p1; p2 < PieceStateNum; p2++) {
            ifs_pp >> pp(p1, p2);
        }
        for (int k = 0; k < 81; k++) {
            ifs_kp >> kp_[k][p1];
        }
    }

    printf("finish readEvalTable\n");
}

template<typename T>
void EvalParams<T>::writeFile(std::string file_name_pp, std::string file_name_kp) {
    printf("start writeEvalTable\n");

    //pp
    std::ofstream ofs_pp(file_name_pp, std::ios::binary);
    if (ofs_pp.fail()) {
        std::cout << file_name_pp << " cannot open (mode w)" << std::endl;
        return;
    }

    //kp
    std::ofstream ofs_kp(file_name_kp, std::ios::binary);
    if (ofs_kp.fail()) {
        std::cout << file_name_kp << " cannot open (mode w)" << std::endl;
        return;
    }

    for (int p1 = 0; p1 < PieceStateNum; p1++) {
        for (int p2 = p1; p2 < PieceStateNum; p2++) {
            ofs_pp << pp(p1, p2) << " ";
        }
        for (int k = 0; k < 81; k++) {
            ofs_kp << kp_[k][p1] << " ";
        }
    }

    printf("finish writeEvalTable\n");
}

template<typename T>
T EvalParams<T>::abs() const {
    T sum = 0;
    for (int p = 0; p < PieceStateNum; p++) {
        for (int k = 0; k < 81; k++) {
            sum += std::abs(kp_[k][p]);
        }
        for (int p2 = p; p2 < PieceStateNum; p2++) {
            sum += std::abs(pp(p, p2));
        }
    }
    return sum;
}

extern EvalParams<int> eval_params;

#endif // !EVAL_PARAMS_HPP