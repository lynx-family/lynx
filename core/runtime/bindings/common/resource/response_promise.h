// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_COMMON_RESOURCE_RESPONSE_PROMISE_H_
#define CORE_RUNTIME_BINDINGS_COMMON_RESOURCE_RESPONSE_PROMISE_H_

#include <future>
#include <optional>
#include <utility>
#include <vector>

#include "base/include/closure.h"
#include "base/include/log/logging.h"

namespace lynx {
namespace runtime {

/**
 * @brief A Promise wrapper with callback support
 *
 * This template class wraps std::promise and std::future, allowing you to
 * register a callback function that will be invoked immediately when the result
 * is set. It is useful for scenarios where you want to be notified
 * asynchronously when an operation completes.
 *
 * T The type of the result value
 *
 * Usage example:
 *   ResponsePromise<int> promiseWithCallback;
 *
 *   // Register a callback to be called when the value is set
 *   ResponsePromise.setCallback([](const int& value) {
 *       std::cout << "Received result: " << value << std::endl;
 *   });
 *
 *   // Set the value (e.g., from an async task)
 *   promiseWithCallback.setValue(42);
 *
 *   // Alternatively, get the future and wait for the result
 *   int result = promiseWithCallback.getFuture().get();
 *
 * Thread safety:
 * - setValue and setCallback are protected by a mutex to ensure thread safety.
 * - Make sure setValue is called only once to avoid undefined behavior.
 *
 * Attention:
 *  - if `then` & `wait` are called in sequence, `wait` will get result first
 * and then callback registered by `then` are triggered.
 */
template <typename T>
class ResponsePromise {
 public:
  using ResponsePromiseCallback = base::MoveOnlyClosure<void, T>;

  ResponsePromise() : future_(promise_.get_future()) {}

  void AddCallback(ResponsePromiseCallback callback) {
    LOGI("ResponsePromise: AddCallback. " << this);
    std::lock_guard<std::mutex> locker(mutex_);
    if (result_.has_value()) {
      // if value already set, invoke callback immediately.
      callback(*result_);
      return;
    }
    callbacks_.emplace_back(std::move(callback));
  }

  std::optional<T> Wait(long timeout) {
    LOGI("ResponsePromise: Wait " << timeout << " " << this);
    if (future_.wait_for(std::chrono::seconds(timeout)) ==
        std::future_status::ready) {
      return future_.get();
    }
    return std::nullopt;
  }

  void SetValue(const T& value) {
    LOGI("ResponsePromise: SetValue " << this);
    std::lock_guard<std::mutex> locker(mutex_);
    if (result_.has_value()) {
      // SetValue Should Be Invoke Just Once!
      return;
    }
    promise_.set_value(value);
    for (auto& cb : callbacks_) {
      cb(value);
    }
    callbacks_.clear();
  }

 private:
  std::promise<T> promise_;
  std::future<T> future_;
  std::mutex mutex_;
  std::optional<T> result_{std::nullopt};

  std::vector<ResponsePromiseCallback> callbacks_;
};
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_COMMON_RESOURCE_RESPONSE_PROMISE_H_
