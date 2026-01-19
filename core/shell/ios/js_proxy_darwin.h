// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_IOS_JS_PROXY_DARWIN_H_
#define CORE_SHELL_IOS_JS_PROXY_DARWIN_H_

#import <Foundation/Foundation.h>
#import <Lynx/LynxErrorReceiverProtocol.h>
#import <Lynx/LynxRuntimeLifecycleListener.h>

#include <memory>
#include <string>

#include "core/shell/lynx_shell.h"
#include "core/shell/runtime/bts/lynx_bts_runtime_proxy_impl.h"

namespace lynx {
namespace shell {

class JSProxyDarwin : public LynxBTSRuntimeProxyImpl {
 public:
  ~JSProxyDarwin() = default;

  static std::shared_ptr<JSProxyDarwin> Create(
      const std::shared_ptr<LynxActor<BTSRuntime>>& actor,
      id<LynxErrorReceiverProtocol> error_handler, int64_t id,
      const std::string& js_group_thread_name,
      bool runtime_standalone_mode = false);

  void RunOnJSThread(dispatch_block_t task);

  int64_t GetId() const { return id_; }

  using LynxBTSRuntimeProxyImpl::AddLifecycleListener;
  void AddLifecycleListener(id<LynxRuntimeLifecycleListener> listener);

 private:
  JSProxyDarwin(const std::shared_ptr<LynxActor<BTSRuntime>>& actor,
                id<LynxErrorReceiverProtocol> error_handler, int64_t id,
                const std::string& js_group_thread_name,
                bool runtime_standalone_mode);

  __weak id<LynxErrorReceiverProtocol> _error_handler;

  const int64_t id_;

  std::string js_group_thread_name_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_IOS_JS_PROXY_DARWIN_H_
