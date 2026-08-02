#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

using namespace std;

// 阻塞队列
template <typename T> class BlockQueue {
public:
  void Push(T task) {
    lock_guard<mutex> lock(m_mtx);
    m_queue.push(task);
    m_cv.notify_one();
  }

  bool Pop(T &task) {
    unique_lock<mutex> lock(m_mtx);
    m_cv.wait(lock, [this]() { return !m_queue.empty() || m_stop; });
    if (m_stop && m_queue.empty()) {
      return false;
    }
    task = m_queue.front();
    m_queue.pop();
    return true;
  }

  void Stop() {
    lock_guard<mutex> lock(m_mtx);
    m_stop = true;
    m_cv.notify_all();
  }

private:
  queue<T> m_queue;
  mutex m_mtx;
  condition_variable m_cv;
  bool m_stop = false;
};

// 线程池
class ThreadPool {
public:
  ThreadPool(int threadNum) {
    for (int i = 0; i < threadNum; ++i) {
      m_workers.emplace_back([this]() {
        function<void()> task;
        while (m_queue.Pop(task)) {
          cout << "Worker Thread ID:" << this_thread::get_id() << endl;
          task();
        }
        cout << "Thread exit, ID:" << this_thread::get_id() << endl;
      });
    }
  }

  ~ThreadPool() {
    m_queue.Stop();
    for (auto &t : m_workers) {
      if (t.joinable())
        t.join();
    }
  }

  template <typename Func> void Submit(Func func) { m_queue.Push(func); }

private:
  vector<thread> m_workers;
  BlockQueue<function<void()>> m_queue;
};

int main() {
  // 创建4个工作线程的线程池
  ThreadPool pool(4);

  // 提交10个任务
  for (int i = 0; i < 10; ++i) {
    pool.Submit([i]() { cout << "Running Task " << i << endl; });
  }

  // 主线程等待一下，方便观察输出
  this_thread::sleep_for(chrono::seconds(2));
  cout << "Main thread will exit." << endl;
  return 0;
}