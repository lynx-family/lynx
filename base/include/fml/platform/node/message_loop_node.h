// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef LYNX_BASE_INCLUDE_FML_PLATFORM_NODE_MESSAGE_LOOP_NODE_H_
#define LYNX_BASE_INCLUDE_FML_PLATFORM_NODE_MESSAGE_LOOP_NODE_H_

#include <fcntl.h>

#include <functional>
#include <uv.h>

#include "base/include/fml/macros.h"
#include "base/include/fml/message_loop_impl.h"

namespace lynx {
namespace fml {

class MessageLoopNode : public MessageLoopImpl {
 public:
  MessageLoopNode();
  ~MessageLoopNode() override;

  void Run() override;
  void Terminate() override;

  uv_loop_t* GetUVLoop() { return &uv_loop_; }
  using UVRunCallback = std::function<int(uv_loop_t*, uv_run_mode)>;
  void SetUVRunCallback(UVRunCallback callback);

 protected:
  void WakeUp(fml::TimePoint time_point) override;

 private:
  int RunUV(uv_run_mode mode);

  uv_loop_t uv_loop_{};
  uv_timer_t timer_{};
  uv_async_t async_{};
  UVRunCallback uv_run_callback_;

  bool running_{false};

  void OnEventFired();
};

}  // namespace fml
}  // namespace lynx

#endif  // LYNX_BASE_INCLUDE_FML_PLATFORM_NODE_MESSAGE_LOOP_NODE_H_
