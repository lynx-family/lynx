// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HARMONY_PLATFORM_CALL_BACK_HARMONY_H_
#define CORE_SHELL_HARMONY_PLATFORM_CALL_BACK_HARMONY_H_

#include <node_api.h>

#include "core/shell/common/platform_call_back.h"

namespace lynx {
namespace shell {

class PlatformCallBackHarmony : public PlatformCallBack {
 public:
  PlatformCallBackHarmony(napi_env env, napi_value callback);
  ~PlatformCallBackHarmony() override;

  void InvokeWithValue(const lepus::Value& value) override;

 private:
  napi_env env_{nullptr};
  napi_ref callback_ref_{nullptr};
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HARMONY_PLATFORM_CALL_BACK_HARMONY_H_
