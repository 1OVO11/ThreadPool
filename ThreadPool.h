#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <algorithm>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>


// 阻塞队列
//   关闭语义：Stop() 之后 Push 一律拒绝；已入队的任务仍会被逐个取走执行。

template <typename T> class BlockQueue {
public:
  // 入队
  bool Push(T task);

  // 出队
  bool Pop(T &task);

  // 关闭：拒绝后续 Push，并唤醒全部等待中的消费者。
  void Stop();

private:
  std::queue<T> m_queue;
  std::mutex m_mtx;
  std::condition_variable m_cv;
  bool m_stop = false;
};

template <typename T> bool BlockQueue<T>::Push(T task) {
  {
    std::lock_guard<std::mutex> lock(m_mtx);
    if (m_stop) {
      return false; // 已关闭：拒绝新任务
    }
    m_queue.push(std::move(task));
  } // 先离开临界区

  // 通知放在锁外：否则被唤醒的线程会立刻因抢不到锁而重新阻塞，
  m_cv.notify_one();
  return true;
}

template <typename T> bool BlockQueue<T>::Pop(T &task) {
  std::unique_lock<std::mutex> lock(m_mtx);
  m_cv.wait(lock, [this]() { return !m_queue.empty() || m_stop; });

  if (m_stop && m_queue.empty()) {
    return false;
  }
  task = std::move(m_queue.front());
  m_queue.pop();
  return true;
}

template <typename T> void BlockQueue<T>::Stop() {
  {
    std::lock_guard<std::mutex> lock(m_mtx);
    m_stop = true;
  }
  m_cv.notify_all(); // 同样移出临界区
}

// 线程池

class ThreadPool {
public:
  explicit ThreadPool(int threadNum);
  ~ThreadPool();

  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;

  // 提交任意可调用对象及其参数，完美转发
  // 线程池已关闭时返回一个【无效 future】：get() 会立刻抛 std::future_error，
  // 而不是永久阻塞。忽略返回值即可当作 fire-and-forget 用。
  template <typename F, typename... Args>
  auto Submit(F &&f, Args &&...args)
      -> std::future<typename std::result_of<F(Args...)>::type>;

private:
  std::vector<std::thread> m_workers;
  BlockQueue<std::function<void()>> m_queue;
};

template <typename F, typename... Args>
auto ThreadPool::Submit(F &&f, Args &&...args)
    -> std::future<typename std::result_of<F(Args...)>::type> {
  using ReturnType = typename std::result_of<F(Args...)>::type;

  // 用 packaged_task 承载调用：返回值经 future 取回，任务内抛出的异常

  auto task = std::make_shared<std::packaged_task<ReturnType()>>(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));

  std::future<ReturnType> fut = task->get_future();

  if (!m_queue.Push([task]() { (*task)(); })) {
    return std::future<ReturnType>(); // 无效 future，调用方 valid() 可判断
  }
  return fut;
}

inline ThreadPool::ThreadPool(int threadNum) {
  // threadNum <= 0 时退化为硬件并发度，至少 1 个，避免任务永远无人执行。
  int n = threadNum;
  if (n <= 0) {
    n = static_cast<int>(std::thread::hardware_concurrency());
  }
  if (n <= 0) {
    n = 1;
  }

  m_workers.reserve(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    m_workers.emplace_back([this]() {
      std::function<void()> task;
      while (m_queue.Pop(task)) {
        try {
          task();
        } catch (...) {
          // packaged_task 已把异常收进 future
        }
      }
    });
  }
}

inline ThreadPool::~ThreadPool() {
  m_queue.Stop(); // 关闭：拒绝新任务，唤醒全部线程
  for (auto &t : m_workers) {
    if (t.joinable()) {
      t.join(); // 排空已入队任务后再退出
    }
  }
}

#endif 
