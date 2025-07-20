//#include "pch.h"
//#include <iostream>
//#include <thread>
//#include <chrono>
//#include <atomic>
//#include <memory>
//#include <string>
//#include <stdexcept>
//#include <ctime>
//#include <iomanip>
//#include <mutex>
//#include <queue>
//#include <condition_variable>
//#include <utility>
//#include <functional>
//#include "ThreadSafeLogger.h"
//#include "pubg_name.h"
//#include "constant.h"
//#include "nloJson.h"
//#include "common.h"
//#include "HttpClient.h"
//#include "pubg.h"
//
//template<typename T>
//class DelayedQueue {
//public:
//	void push(const T& item) {
//		std::lock_guard<std::mutex> lock(mutex_);
//		queue_.push(item);
//		cond_.notify_one();
//	}
//
//	void push(T&& item) {
//		std::lock_guard<std::mutex> lock(mutex_);
//		queue_.push(std::move(item));
//		cond_.notify_one();
//	}
//
//	bool try_pop(T& item) {
//		std::lock_guard<std::mutex> lock(mutex_);
//		if (queue_.empty()) {
//			return false;
//		}
//		item = std::move(queue_.front());
//		queue_.pop();
//		return true;
//	}
//
//	bool pop(T& item) {
//		std::unique_lock<std::mutex> lock(mutex_);
//		cond_.wait(lock, [this] { return !queue_.empty(); });
//		item = std::move(queue_.front());
//		queue_.pop();
//		return true;
//	}
//
//	bool empty() const {
//		std::lock_guard<std::mutex> lock(mutex_);
//		return queue_.empty();
//	}
//
//	size_t size() const {
//		std::lock_guard<std::mutex> lock(mutex_);
//		return queue_.size();
//	}
//
//private:
//	mutable std::mutex mutex_;
//	std::condition_variable cond_;
//	std::queue<T> queue_;
//};
//
//
