// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_LEPUS_PROXY_JS_DEBUG_PROXY_LEPUS_H_
#define DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_LEPUS_PROXY_JS_DEBUG_PROXY_LEPUS_H_

#include <memory.h>

#include "devtool/lynx_devtool/js_debug/helper/js_debug_proxy.h"

namespace lynx {
namespace devtool {

class JSDebugProxyLepus : public JSDebugProxy {
 public:
  JSDebugProxyLepus() = default;
  ~JSDebugProxyLepus() override = default;

  std::unique_ptr<lepus::LepusInspectorManager> CreateLepusInspectorManager()
      override;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_LEPUS_PROXY_JS_DEBUG_PROXY_LEPUS_H_
