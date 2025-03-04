// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_LYNX_ACTOR_H_
#define BASE_INCLUDE_LYNX_ACTOR_H_

#include <memory>
#include <string>
#include <utility>

#include "base/include/fml/task_runner.h"

namespace lynx {
namespace shell {

// if you don't need instance id, just use kUnknownInstanceId.
constexpr static int32_t kUnknownInstanceId = -1;

template <typename C, typename T, typename Enable = void>
class LynxActorMixin {
 protected:
  void BeforeInvoked() {}

  void AfterInvoked() {}
};

// actor for each thread
template <typename T>
class LynxActor : public LynxActorMixin<LynxActor<T>, T>,
                  public std::enable_shared_from_this<LynxActor<T>> {
 public:
  LynxActor(std::unique_ptr<T> impl, fml::RefPtr<fml::TaskRunner> runner,
            int32_t instance_id = kUnknownInstanceId, bool enable = true)
      : impl_(std::move(impl)),
        runner_(std::move(runner)),
        instance_id_(instance_id),
        enable_(enable) {}
  ~LynxActor() {}

  template <typename F>
  void Act(F&& func) {
    if (!enable_) {
      return;
    }
    if (runner_->RunsTasksOnCurrentThread()) {
      Invoke(std::forward<F>(func));
    } else {
      runner_->PostTask([self = this->shared_from_this(),
                         func = std::forward<F>(func)]() mutable {
        self->Invoke(std::forward<F>(func));
      });
    }
  }

  // Warning: ONlY for LynxActor<LayoutContext>! Don't use it in other
  // LynxActors.
  template <typename F>
  void ActLite(F&& func) {
    if (!enable_) {
      return;
    }
    if (runner_->RunsTasksOnCurrentThread()) {
      InvokeLite(func);
    } else {
      runner_->PostTask(
          [self = this->shared_from_this(),
           func = std::forward<F>(func)]() mutable { self->InvokeLite(func); });
    }
  }

  template <typename F>
  void ActAsync(F&& func) {
    if (!enable_) {
      return;
    }

    runner_->PostTask([self = this->shared_from_this(),
                       func = std::forward<F>(func)]() mutable {
      self->Invoke(std::forward<F>(func));
    });
  }

  template <typename F>
  void ActIdle(F&& func) {
    if (!enable_) {
      return;
    }

    runner_->PostIdleTask([self = this->shared_from_this(),
                           func = std::forward<F>(func)]() mutable {
      self->Invoke(std::forward<F>(func));
    });
  }

  template <typename F, typename = std::enable_if_t<!std::is_void<
                            std::result_of_t<F(std::unique_ptr<T>&)>>::value>>
  auto ActSync(F&& func) {
    std::result_of_t<F(std::unique_ptr<T>&)> result;
    ActSync([&result, func = std::forward<F>(func)](auto& impl) mutable {
      result = func(impl);
    });
    return result;
  }

  template <typename F, typename = std::enable_if_t<std::is_void<
                            std::result_of_t<F(std::unique_ptr<T>&)>>::value>>
  void ActSync(F&& func) {
    if (!enable_) {
      return;
    }
    runner_->PostSyncTask([this, func = std::forward<F>(func)]() mutable {
      Invoke(std::forward<F>(func));
    });
  }

  // TODO(heshan):now use for LynxRuntime, will remove,
  // now need for devtool...
  T* Impl() { return impl_.get(); }
  // TODO(lipin): now use for LayoutMediator,maybe remove later
  bool CanRunNow() { return runner_->RunsTasksOnCurrentThread(); }

  int32_t GetInstanceId() { return instance_id_; }

  fml::RefPtr<fml::TaskRunner>& GetRunner() { return runner_; }

 private:
  template <typename F>
  void Invoke(F&& func) {
    LynxActorMixin<LynxActor<T>, T>::BeforeInvoked();

    if (impl_ != nullptr) {
      func(impl_);
    }

    LynxActorMixin<LynxActor<T>, T>::AfterInvoked();
  }

  template <typename F>
  void InvokeLite(F&& func) {
    if (impl_ != nullptr) {
      func(impl_);
    }
  }

  std::unique_ptr<T> impl_;

  fml::RefPtr<fml::TaskRunner> runner_;

  // Generated in the LynxShell, id of LynxShell.
  // instance_id_ is a value greater than or equal to 0, the initial value is
  // -1.
  const int32_t instance_id_;

  bool enable_ = true;
};

}  // namespace shell
}  // namespace lynx

#endif  // BASE_INCLUDE_LYNX_ACTOR_H_
