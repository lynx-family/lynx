// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_MESSAGE_HANDLER_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_MESSAGE_HANDLER_HARMONY_H_

#include <node_api.h>
#include <uv.h>

#include <memory>
#include <string>

#include "devtool/embedder/core/devtools_message_handler.h"

namespace lynx {
namespace devtool {

class MessageHandlerHarmony
    : public MessageHandler,
      public std::enable_shared_from_this<MessageHandlerHarmony> {
 public:
  MessageHandlerHarmony(napi_env env, napi_value js_object);
  virtual ~MessageHandlerHarmony() { napi_delete_reference(env_, ref_); }

  void OnMessage(const std::string& message) override;

 private:
  napi_env env_;
  napi_ref ref_;
  uv_loop_t* loop_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_MESSAGE_HANDLER_HARMONY_H_
