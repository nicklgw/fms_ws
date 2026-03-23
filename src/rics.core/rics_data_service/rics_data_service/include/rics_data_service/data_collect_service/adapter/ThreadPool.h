#pragma once
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <functional>
// #include <brclcpp/timer/timer.h>
using namespace std;

namespace rics
{
namespace data_collect
{
class ThreadSig
{
private:
    typedef function<void()> Task;

    // shared_ptr<brclcpp::Timer> m_pTimer;

    std::shared_ptr<std::thread> m_pTimerThread;

    thread m_thread;

    Task m_sigTask;

    queue<Task> m_tasks;

    mutex m_mutex;
    condition_variable m_cond;

    bool m_done;

    bool IsTasksEmpty();

    bool IsTasksFull();

    void RunTask();

    void ToTaskQueue();


public:
    ThreadSig();
    ~ThreadSig();
    void Start();

    void AddTask(const Task& f);

    void TimerInit(uint64_t interval_nan);

    void Finish();
};

class ThreadPool
{
private:
    typedef function<void()> Task;
    vector<shared_ptr<ThreadSig>> m_threadPool;
    void FinishAllTask();

public:
    ThreadPool();
    ~ThreadPool();

    void AddTask(const Task& f, uint64_t interval_nan, bool isloop = false);
};


}  // namespace data_collect
}  // namespace rics
