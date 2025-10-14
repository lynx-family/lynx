// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_ONE_SHOT_CALLBACK_H_
#define CLAY_SHELL_COMMON_ONE_SHOT_CALLBACK_H_

#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>

namespace clay {

template <typename... Args>
class OneShotCallback {
 public:
  OneShotCallback() = default;
  ~OneShotCallback() = default;

  template <typename F, typename = std::enable_if_t<
                            !std::is_same_v<std::decay_t<F>, OneShotCallback>>>
  explicit OneShotCallback(F&& f)
      : shared_state_(std::make_shared<SharedState>(std::forward<F>(f))) {}

  OneShotCallback(const OneShotCallback& other) = default;
  OneShotCallback(OneShotCallback&& other) noexcept = default;
  OneShotCallback& operator=(const OneShotCallback& other) = default;
  OneShotCallback& operator=(OneShotCallback&& other) noexcept = default;

  void operator()(Args&&... args) {
    if (shared_state_) {
      shared_state_->Execute(std::forward<Args>(args)...);
    }
  }

  explicit operator bool() const { return UnDone(); }

  bool UnDone() const { return shared_state_ && shared_state_->callback; }

  void Reset() { shared_state_.reset(); }

 private:
  struct SharedState {
    std::function<void(Args...)> callback;
    std::once_flag flag;

    template <typename F>
    explicit SharedState(F&& f) : callback(std::forward<F>(f)) {}

    void Execute(Args&&... args) {
      std::call_once(flag, [this, &args...]() {
        if (callback) {
          std::invoke(callback, std::forward<Args>(args)...);
          callback = nullptr;
        }
      });
    }
  };

  std::shared_ptr<SharedState> shared_state_;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_ONE_SHOT_CALLBACK_H_
