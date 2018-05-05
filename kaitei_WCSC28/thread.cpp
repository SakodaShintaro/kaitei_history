#include"thread.hpp"

#include"position.hpp"
#include"types.hpp"
#include"move_picker.hpp"
#include"shared_data.hpp"
#include"usi_options.hpp"

ThreadPool threads;
SharedData shared_data;

void Thread::idleLoop() {
    while (!exit_) {
        std::unique_lock<std::mutex> lock(mutex_);

        //exit_かsearching_がtrueになるまでスリープする
        sleep_condition_.wait(lock, [this]() {
            return exit_ || searching_;
        });

        lock.unlock();
        if (exit_)
            break;

        //ここで探索開始
        searcher_.think();

        //探索を抜けたのでフラグをfalseに
        mutex_.lock();
        searching_ = false;
        sleep_condition_.notify_one();
        mutex_.unlock();
    }
}

ThreadPool::ThreadPool() {
    //メインスレッドを作る
    this->emplace_back(new MainThread(0));
}

void ThreadPool::init() {
    //Mainの分と、使うならMateの分を引く
    int slave_num = (usi_option.do_use_mate_search ? usi_option.thread_num - 2 : usi_option.thread_num - 1);

    //メインスレッドを作る
    this->emplace_back(new MainThread(0));

    //オプションの指定に沿って詰み探索スレッドを作る
    if (usi_option.do_use_mate_search) {
        this->emplace_back(new Thread(1));
        this->at(1)->setSeacherRole(Searcher::Role::MATE);
    }

    //スレーブスレッドを作る
    int slave_thread_start = (usi_option.do_use_mate_search ? 2 : 1);
    for (int i = 0; i < slave_num; ++i) {
        this->emplace_back(new Thread(slave_thread_start + i));
        this->at(slave_thread_start + i)->setSeacherRole(Searcher::Role::SLAVE);
    }
}