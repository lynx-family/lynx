// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef EXPLORER_EMBEDDER_LYNX_EXPLORER_MODULE_LYNX_DEMO_EXTENSION_MODULE_H_
#define EXPLORER_EMBEDDER_LYNX_EXPLORER_MODULE_LYNX_DEMO_EXTENSION_MODULE_H_

#include <memory>

#include "lynx_extension_module.h"
#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace example {

class LynxDemoExtensionModule : public pub::LynxExtensionModule {
 public:
  static lynx_extension_module_t* CreateCModule(void* opaque);

  LynxDemoExtensionModule() = default;

  void OnRuntimeAttach(
      napi_env env,
      std::unique_ptr<pub::VSyncObserver> vsync_observer) override;

  void Destroy() override;

  bool GetFlag() { return flag_; }

 private:
  bool flag_ = false;
};

}  // namespace example
}  // namespace lynx

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif

#endif  // EXPLORER_EMBEDDER_LYNX_EXPLORER_MODULE_LYNX_DEMO_EXTENSION_MODULE_H_
