// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_PLATFORM_HARMONY_MESSAGE_LOOP_HARMONY_H_
#define BASE_INCLUDE_FML_PLATFORM_HARMONY_MESSAGE_LOOP_HARMONY_H_

#include <node_api.h>
#include <uv.h>

#include <atomic>

#include "base/include/base_export.h"
#include "base/include/fml/macros.h"
#include "base/include/fml/message_loop_impl.h"
#include "base/include/fml/unique_fd.h"

namespace lynx {
namespace fml {

/// Harmony implementation of \p MessageLoopImpl.
///
class MessageLoopHarmony : public MessageLoopImpl {
 public:
  void* GetPlatformLoop() const { return static_cast<void*>(looper_); }
  BASE_EXPORT void SetupNapiCallback(napi_env env);

 private:
  uv_loop_t* looper_;
  bool is_looper_owner_;
  uv_poll_t poll_;
  fml::UniqueFD timer_fd_;
  bool running_;
  napi_env env_ = nullptr;
  napi_ref run_expired_tasks_callback_ = nullptr;
  bool did_run_expired_tasks_in_microtask_ = false;

  void Run() override;

  void Terminate() override;

  void OnEventFired();

  bool RunExpiredTasksByPromiseMicrotask();

  static napi_value RunExpiredTasksInMicrotask(napi_env env,
                                               napi_callback_info info);

  FML_FRIEND_MAKE_REF_COUNTED(MessageLoopHarmony);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(MessageLoopHarmony);
  BASE_DISALLOW_COPY_AND_ASSIGN(MessageLoopHarmony);

 protected:
  void WakeUp(fml::TimePoint time_point) override;

  explicit MessageLoopHarmony(void* platform_loop);

  ~MessageLoopHarmony() override;
};

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::MessageLoopHarmony;
}  // namespace fml

#endif  // BASE_INCLUDE_FML_PLATFORM_HARMONY_MESSAGE_LOOP_HARMONY_H_
