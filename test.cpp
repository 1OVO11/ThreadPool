#include "ThreadPool.h"
#include <chrono>

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
