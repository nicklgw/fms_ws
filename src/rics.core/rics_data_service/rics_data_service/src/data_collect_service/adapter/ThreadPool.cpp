#include <iostream>
// #include <brclcpp/brclcpp.h>
// #include <brclcpp/time/rate.h>
#include <rics_data_service/data_collect_service/adapter/ThreadPool.h>

using namespace rics::data_collect;

ThreadSig::ThreadSig() : m_done(true) { Start(); }

ThreadSig::~ThreadSig() { Finish(); }

void ThreadSig::TimerInit(uint64_t interval_nano) {
  m_pTimerThread = std::make_shared<std::thread>([this, interval_nano]() {
    while (m_done) {
      std::this_thread::sleep_for(std::chrono::nanoseconds(interval_nano));
      ToTaskQueue();
    }
  });
}

void ThreadSig::ToTaskQueue() {
  std::unique_lock<std::mutex> lk(m_mutex);
  while (IsTasksFull()) {
    return;
  }
  m_tasks.push(m_sigTask);
  m_cond.notify_one();
}

void ThreadSig::Finish() {
  m_done = false;
  if (m_pTimerThread && m_pTimerThread->joinable()) {
    m_pTimerThread->join();
  }
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void ThreadSig::RunTask() {
  while (m_done) {
    std::unique_lock<std::mutex> lk(m_mutex);
    while (IsTasksEmpty()) {
      m_cond.wait(lk);
    }
    Task ta = std::move(m_tasks.front());
    m_tasks.pop();
    ta();
    m_cond.notify_one();
  }
}

void ThreadSig::Start() { m_thread = std::thread(&ThreadSig::RunTask, this); }

void ThreadSig::AddTask(const Task& f) {
  std::unique_lock<std::mutex> lk(m_mutex);
  m_sigTask = f;
  m_tasks.push(f);
  m_cond.notify_one();
}

bool ThreadSig::IsTasksEmpty() { return m_tasks.empty(); }

bool ThreadSig::IsTasksFull() { return (m_tasks.size() == 1) ? true : false; }

ThreadPool::ThreadPool() {}

ThreadPool::~ThreadPool() { FinishAllTask(); }

void ThreadPool::AddTask(const Task& f, uint64_t interval_nan, bool isloop) {
  auto pthread = make_shared<ThreadSig>();
  pthread->AddTask(f);
  if (isloop) pthread->TimerInit(interval_nan);
  m_threadPool.push_back(pthread);
}

void ThreadPool::FinishAllTask() {
  for (auto iter = m_threadPool.cbegin(); iter != m_threadPool.cend(); iter++) {
    (*iter)->Finish();
  }
}
