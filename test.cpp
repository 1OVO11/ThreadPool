// 编译: g++ -std=c++11 -pthread test_main.cpp -o tp_demo
#include "ThreadPool.h"

#include <iostream>

int main() {
  ThreadPool pool(4); // 4 个工作线程

  // 提交 10 个任务
  for (int i = 0; i < 10; ++i) {
    pool.Submit([i]() { std::cout << "Running Task " << i << std::endl; });
  }

  std::cout << "Main thread will exit." << std::endl;
  return 0; // pool 析构时会 Stop + join，等 10 个任务全部跑完
}
