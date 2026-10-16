# ThreadPool

C++11 通用线程池，单头文件、只依赖标准库。

## 特性

- **单头文件**：`#include "ThreadPool.h"` 即可，无额外依赖
- **任意可调用对象 + 任意参数**：可变参数模板 + `std::forward` 完美转发
- **能取返回值**：`Submit` 返回 `std::future`；任务里的异常经 future 抛回，不会终止进程
- **零 CPU 空转**：工作线程用 `wait` 阻塞等待，没任务时不占 CPU
- **优雅关闭**：析构时拒绝新任务、唤醒全部线程，已入队的任务执行完再退出

## 快速开始

```cpp
#include "ThreadPool.h"
#include <iostream>

int main() {
  ThreadPool pool(4); // 4 个工作线程

  for (int i = 0; i < 10; ++i) {
    pool.Submit([i]() { std::cout << "Running Task " << i << std::endl; });
  }

  return 0; // 析构时等待全部任务完成
}
```

```bash
g++ -std=c++11 -pthread main.cpp -o demo && ./demo
```

## API

| 接口 | 说明 |
|---|---|
| `ThreadPool(int n)` | 创建 n 个工作线程；`n <= 0` 时取 CPU 核心数 |
| `Submit(f, args...)` | 提交任务，返回 `std::future` |
| `BlockQueue<T>` | 内部任务队列，也对外公开：`Push` / `Pop` / `Stop` |

```cpp
// 取返回值
auto f = pool.Submit([](int a, int b) { return a + b; }, 2, 3);
std::cout << f.get() << std::endl; // 5

// 忽略返回值就是普通的 fire-and-forget
pool.Submit([]() { std::cout << "done\n"; });
```

## 说明

- 需要 C++11 及以上，编译加 `-pthread`；`-Wall -Wextra` 无告警
- 任务里的异常不会让进程挂掉，会在 `f.get()` 时重新抛出
- 线程池关闭后 `Submit` 返回无效 future（`valid()` 为 false），不会永久阻塞
- `BlockQueue` 关闭后 `Push` 返回 `false` 拒绝新任务，但已入队的仍会执行完，不丢弃

## License

学习交流项目，自由使用。
