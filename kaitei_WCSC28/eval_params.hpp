#pragma once
#ifndef EVAL_PARAMS_HPP
#define EVAL_PARAMS_HPP

#include"piece.hpp"
#include"eval_elements.hpp"
#include"square.hpp"
#include<climits>
#include<cmath>
#include<map>
#include<memory>

//評価関数のパラメータをまとめたもの
//インスタンスとしては実際のパラメータ(int)と、学習時の勾配(float?)がありえる

template<typename T>
class EvalParams {
public:
    T pp[PieceStateNum][PieceStateNum];
    T kp[81][PieceStateNum];

    void updateGradient(const Features &ee, const T delta);
    void updateParamsSGD(std::unique_ptr<EvalParams<double>>& grad, const double learn_rate);
    void updateParamsAdaGrad(std::unique_ptr<EvalParams<double>>& grad, std::unique_ptr<EvalParams<double>>& RMSgrad, const double learn_rate);
    void updateParamsAdaDelta(std::unique_ptr<EvalParams<double>>& grad, std::unique_ptr<EvalParams<double>>& RMSgrad, std::unique_ptr<EvalParams<double>>& RMSdelta, const double decay_rate);
    void clear();
    void readFile(std::string file_name = "parameters.bin");
    void writeFile(std::string file_name = "parameters.bin");
    T abs() const;
    T max_abs() const;

    //すべてのパラメータに同じ操作をする場合これを使って書けばいいのでは
    template<typename Function> void forEach(Function f);

    template<typename U> void copy(std::unique_ptr<EvalParams<U>>& source);
    template<typename U> void roundCopy(std::unique_ptr<EvalParams<U>>& source);

    EvalParams& operator*=(double rhs) {
        forEach([&](T& value) {
            value = static_cast<T>(value * rhs);
        });
        return *this;
    }

    EvalParams<double> operator*(const double x);
    EvalParams<double> operator+(const EvalParams<double> rhs);
    EvalParams<double> square();

    void calcRMS(std::unique_ptr<EvalParams<double>>& grad, const double decay_rate);
    void addSquare(std::unique_ptr<EvalParams<double>>& grad);

    void addL1grad(const EvalParams<int>& eval_params, const double coefficient) {
        //これはforEachを使っては書けない気がするなぁ
        for (int p = 0; p < PieceStateNum; p++) {
            for (int k = 0; k < 81; k++) {
                if (eval_params.kp[k][p] == 0)
                    continue;
                kp[k][p] += (eval_params.kp[k][p] > 0 ? coefficient : -coefficient);
            }
            for (int p2 = 0; p2 < PieceStateNum; p2++) {
                if (eval_params.pp[p][p2] == 0)
                    continue;
                pp[p][p2] += (eval_params.pp[p][p2] > 0 ? coefficient : -coefficient);
            }
        }
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
            for (int p2 = 0; p2 < PieceStateNum; p2++) {
                if (std::abs(pp[p1][p2]) >= threshold) {
                    os << "pp[" << PieceState(p1) << "][" << PieceState(p2) << "] = " << pp[p1][p2] << std::endl;
                }
            }
            for (int k = 0; k < 81; k++) {
                if (std::abs(kp[k][p1]) >= threshold) {
                    os << "kp[" << SquareList[k] << "][" << PieceState(p1) << "] = " << kp[k][p1] << std::endl;
                }
            }
        }
    }

    void printParamsSorted(std::ostream& os) {
        for (auto compare = max_abs(); compare >= -max_abs(); --compare) {
            if (compare == 0) {
                continue;
            }
            for (int p1 = 0; p1 < PieceStateNum; p1++) {
                for (int p2 = 0; p2 < PieceStateNum; p2++) {
                    if (pp[p1][p2] == compare) {
                        os << "pp[" << PieceState(p1) << "][" << PieceState(p2) << "] = " << pp[p1][p2] << std::endl;
                    }
                }
                for (int k = 0; k < 81; k++) {
                    if (kp[k][p1] == compare) {
                        os << "kp[" << SquareList[k] << "][" << PieceState(p1) << "] = " << kp[k][p1] << std::endl;
                    }
                }
            }
        }
    }
};

template<typename T>
void EvalParams<T>::updateGradient(const Features &ee, const T delta) {
    //eeに出てくる特徴量に関わる勾配すべてをdeltaだけ変える
    for (unsigned int i = 0; i < PIECE_STATE_LIST_SIZE; i++) {
        kp[ee.black_king_sq][ee.piece_state_list[i]] += delta;
        kp[ee.white_king_sq_reversed][invPieceState(ee.piece_state_list[i])] -= delta;
        for (unsigned int j = i; j < PIECE_STATE_LIST_SIZE; j++) {
            //普通のp1,p2
            pp[ee.piece_state_list[i]][ee.piece_state_list[j]] += delta;

            //inv(p1), inv(p2)について
            pp[invPieceState(ee.piece_state_list[i])][invPieceState(ee.piece_state_list[j])] -= delta;
            
            if (ee.piece_state_list[i] != ee.piece_state_list[j]) {
                //p1,p2の順番逆にしたものにも同じ勾配を足す
                pp[ee.piece_state_list[j]][ee.piece_state_list[i]] += delta;
                pp[invPieceState(ee.piece_state_list[j])][invPieceState(ee.piece_state_list[i])] -= delta;
            }
        }
    }
}

template<typename T>
void EvalParams<T>::updateParamsSGD(std::unique_ptr<EvalParams<double>>& grad, const double learn_rate) {
    double sum_of_delta = 0, max_delta = 0;

    //ヒストグラムを取ってみよう
    std::map<int, int> histgram;

    for (int p1 = 0; p1 < PieceStateNum; p1++) {
        for (int p2 = 0; p2 < PieceStateNum; p2++) {
            double delta = ((learn_rate * grad->pp[p1][p2]));

            max_delta = std::max(max_delta, std::fabs(delta));

            histgram[static_cast<int>(delta)]++;

            //パラメータ更新
            pp[p1][p2] -= static_cast<T>(delta);

            //合計いくら変わったか見るため
            sum_of_delta += std::fabs(delta);
        }
        for (int k = 0; k < 81; k++) {
            double delta = ((learn_rate * grad->kp[k][p1]));

            max_delta = std::max(max_delta, std::abs(delta));

            histgram[static_cast<int>(delta)]++;

            //パラメータ更新
            kp[k][p1] -= static_cast<T>(delta);

            //合計いくら変わったか見るため
            sum_of_delta += std::fabs(delta);
        }
    }
    std::cout << "sum_of_delta = " << sum_of_delta << ", max_delta_abs = " << max_delta << std::endl;
    std::cout << "delta-histgram" << std::endl;
    for (auto e : histgram) {
        std::cout << e.first << ":" << e.second << ", ";
    }
    std::cout << std::endl;
}

template<typename T>
inline void EvalParams<T>::updateParamsAdaGrad(std::unique_ptr<EvalParams<double>>& grad, std::unique_ptr<EvalParams<double>>& RMSgrad, const double learn_rate) {
    double sum_of_delta = 0, max_delta = 0;

    //ヒストグラムを取ってみよう
    std::map<int, int> histgram;

    static const double epcilon = 0.00000001;
    for (int p1 = 0; p1 < PieceStateNum; p1++) {
        for (int p2 = 0; p2 < PieceStateNum; p2++) {
            double delta = (learn_rate * grad->pp[p1][p2]) / std::sqrt(RMSgrad->pp[p1][p2] + epcilon);

            max_delta = std::max(max_delta, std::fabs(delta));

            histgram[static_cast<int>(delta)]++;

            //パラメータ更新
            pp[p1][p2] -= delta;

            //合計いくら変わったか見るため
            sum_of_delta += std::fabs(delta);
        }
        for (int k = 0; k < 81; k++) {
            double delta = (learn_rate * grad->kp[k][p1]) / std::sqrt(RMSgrad->kp[k][p1] + epcilon);

            max_delta = std::max(max_delta, std::abs(delta));

            histgram[static_cast<int>(delta)]++;

            //パラメータ更新
            kp[k][p1] -= delta;

            //合計いくら変わったか見るため
            sum_of_delta += std::fabs(delta);
        }
    }
    std::cout << "delta = " << sum_of_delta << ", max_delta_abs = " << max_delta << std::endl;
    std::cout << "delta-histgram" << std::endl;
    for (auto e : histgram) {
        std::cout << e.first << ":" << e.second << ", ";
    }
    std::cout << std::endl;
}

template<typename T>
inline void EvalParams<T>::updateParamsAdaDelta(std::unique_ptr<EvalParams<double>>& grad, std::unique_ptr<EvalParams<double>>& RMSgrad, std::unique_ptr<EvalParams<double>>& RMSdelta, const double decay_rate) {
    double sum_of_delta = 0, max_delta = 0;

    //ヒストグラムを取ってみよう
    std::map<int, int> histgram;
    static const double e = 0.1;

    for (int p1 = 0; p1 < PieceStateNum; p1++) {
        for (int p2 = 0; p2 < PieceStateNum; p2++) {
            double delta = std::sqrt(RMSdelta->pp[p1][p2] + e) / std::sqrt(RMSgrad->pp[p1][p2] + e) * grad->pp[p1][p2];

            //四捨五入
            delta = std::round(delta);

            max_delta = std::max(max_delta, std::fabs(delta));

            histgram[static_cast<int>(delta)]++;

            //パラメータ更新
            pp[p1][p2] -= static_cast<T>(delta);

            //合計いくら変わったか見るため
            sum_of_delta += std::fabs(delta);

            //減衰させながら足す
            RMSdelta->pp[p1][p2] = decay_rate * RMSdelta->pp[p1][p2] + (1 - decay_rate) * delta * delta;
        }
        for (int k = 0; k < 81; k++) {
            double delta = std::sqrt(RMSdelta->kp[k][p1] + e) / std::sqrt(RMSgrad->kp[k][p1] + e) * grad->kp[k][p1];

            //四捨五入
            delta = std::round(delta);

            max_delta = std::max(max_delta, std::abs(delta));

            histgram[static_cast<int>(delta)]++;

            //パラメータ更新
            kp[k][p1] -= static_cast<T>(delta);

            //合計いくら変わったか見るため
            sum_of_delta += std::fabs(delta);

            //減衰させながら足す
            RMSdelta->kp[k][p1] = decay_rate * RMSdelta->kp[k][p1] + (1 - decay_rate) * delta * delta;
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
        for (int p2 = 0; p2 < PieceStateNum; p2++) {
            pp[p1][p2] = 0;
        }
        for (int k = 0; k < 81; k++) {
            kp[k][p1] = 0;
        }
    }
}

template<typename T>
inline void EvalParams<T>::readFile(std::string file_name) {
    std::ifstream ifs(file_name, std::ios::binary);
    if (ifs.fail()) {
        std::cerr << file_name << " cannot open (mode r)" << std::endl;
        clear();
        return;
    }
    ifs.read(reinterpret_cast<char*>(&pp[0][0]), PieceStateNum * PieceStateNum * sizeof(T));
    ifs.read(reinterpret_cast<char*>(&kp[0][0]), 81 * PieceStateNum * sizeof(T));
}

template<typename T>
inline void EvalParams<T>::writeFile(std::string file_name) {
    std::ofstream ofs(file_name, std::ios::binary | std::ios::trunc);
    if (ofs.fail()) {
        std::cerr << file_name << " cannot open (mode w)" << std::endl;
        return;
    }
    ofs.write(reinterpret_cast<char*>(&pp[0][0]), PieceStateNum * PieceStateNum * sizeof(T));
    ofs.write(reinterpret_cast<char*>(&kp[0][0]), 81 * PieceStateNum * sizeof(T));
}

template<typename T>
T EvalParams<T>::abs() const {
    T sum = 0;
    for (int p = 0; p < PieceStateNum; p++) {
        for (int k = 0; k < 81; k++) {
            sum += std::abs(kp[k][p]);
        }
        for (int p2 = 0; p2 < PieceStateNum; p2++) {
            sum += std::abs(pp[p][p2]);
        }
    }
    return sum;
}

template<typename T>
inline T EvalParams<T>::max_abs() const {
    T max_val = INT_MIN;
    for (int p = 0; p < PieceStateNum; p++) {
        for (int k = 0; k < 81; k++) {
            max_val = std::max(max_val, std::abs(kp[k][p]));
        }
        for (int p2 = 0; p2 < PieceStateNum; p2++) {
            max_val = std::max(max_val, std::abs(pp[p][p2]));
        }
    }
    return max_val;
}

template<typename T>
template<typename Function>
void EvalParams<T>::forEach(Function f) {
    for (int p = 0; p < PieceStateNum; p++) {
        for (int k = 0; k < 81; k++) {
            f(kp[k][p]);
        }
        for (int p2 = 0; p2 < PieceStateNum; p2++) {
            f(pp[p][p2]);
        }
    }
}

template<typename T>
template<typename U>
inline void EvalParams<T>::copy(std::unique_ptr<EvalParams<U>>& source) {
    for (int p = 0; p < PieceStateNum; ++p) {
        for (int k = 0; k < 81; ++k) {
            kp[k][p] = static_cast<T>(source->kp[k][p]);
        }
        for (int p2 = 0; p2 < PieceStateNum; ++p2) {
            pp[p][p2] = static_cast<T>(source->pp[p][p2]);
        }
    }
}

template<typename T>
template<typename U>
inline void EvalParams<T>::roundCopy(std::unique_ptr<EvalParams<U>>& source) {
    for (int p = 0; p < PieceStateNum; ++p) {
        for (int k = 0; k < 81; ++k) {
            kp[k][p] = static_cast<T>(std::round(source->kp[k][p]));
        }
        for (int p2 = 0; p2 < PieceStateNum; ++p2) {
            pp[p][p2] = static_cast<T>(std::round(source->pp[p][p2]));
        }
    }
}

template<typename T>
inline EvalParams<double> EvalParams<T>::operator*(const double x) {
    auto copy = *this;
    copy.forEach([&](auto& value) {
        value *= x;
    });
    return copy;
}

template<typename T>
inline EvalParams<double> EvalParams<T>::operator+(const EvalParams<double> rhs) {
    auto copy = *this;
    for (int p = 0; p < PieceStateNum; p++) {
        for (int k = 0; k < 81; k++) {
            copy.kp[k][p] += rhs.kp[k][p];
        }
        for (int p2 = 0; p2 < PieceStateNum; p2++) {
            copy.pp[p][p2] += rhs.pp[p][p2];
        }
    }
    return copy;
}

template<typename T>
inline EvalParams<double> EvalParams<T>::square() {
    auto copy = *this;
    copy.forEach([&](auto& value) {
        value *= value;
    });
    return copy;
}

template<typename T>
inline void EvalParams<T>::calcRMS(std::unique_ptr<EvalParams<double>>& grad, const double decay_rate) {
    //*RMSgrad = *RMSgrad * DECAY_RATE + grad->square() * (1 - DECAY_RATE);
    for (int p = 0; p < PieceStateNum; ++p) {
        for (int k = 0; k < 81; ++k) {
            kp[k][p] = kp[k][p] * decay_rate + grad->kp[k][p] * grad->kp[k][p] * (1 - decay_rate);
        }
        for (int p2 = 0; p2 < PieceStateNum; ++p2) {
            pp[p][p2] = pp[p][p2] * decay_rate + grad->pp[p][p2] * grad->pp[p][p2] * (1 - decay_rate);
        }
    }
}

template<typename T>
inline void EvalParams<T>::addSquare(std::unique_ptr<EvalParams<double>>& grad) {
    for (int p = 0; p < PieceStateNum; ++p) {
        for (int k = 0; k < 81; ++k) {
            kp[k][p] += grad->kp[k][p] * grad->kp[k][p];
        }
        for (int p2 = 0; p2 < PieceStateNum; ++p2) {
            pp[p][p2] += grad->pp[p][p2] * grad->pp[p][p2];
        }
    }
}

extern std::unique_ptr<EvalParams<int>> eval_params;

#endif // !EVAL_PARAMS_HPP