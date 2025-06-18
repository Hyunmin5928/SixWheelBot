#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class SafeQueue {
  std::queue<T>               q;
  mutable std::mutex          m;
  std::condition_variable     cv;
public:
  void push(const T& item) {
    {
      std::lock_guard<std::mutex> lk(m);
      q.push(item);
    }
    cv.notify_one();
  }

  // 대기 후 하나 꺼내기
  T pop() {
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk, [&]{ return !q.empty(); });
    T item = q.front(); q.pop();
    return item;
  }

  // 비차단(pop이 아니라 존재 여부 확인만)
  bool empty() const {
    std::lock_guard<std::mutex> lk(m);
    return q.empty();
  }
};
