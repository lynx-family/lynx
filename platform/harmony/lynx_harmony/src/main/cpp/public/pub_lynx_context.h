// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_PUBLIC_PUB_LYNX_CONTEXT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_PUBLIC_PUB_LYNX_CONTEXT_H_

#include <memory>
#include <string>

#include "base/include/base_export.h"
#include "base/include/closure.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/task_runner.h"
#include "core/public/lynx_engine_proxy.h"
#include "core/public/lynx_runtime_proxy.h"
#include "core/public/pub_value.h"

namespace lynx {
namespace pub {
class LynxResourceLoader;
}
namespace tasm {
namespace harmony {
class PubUIOwner;
class PubShadowNodeOwner;
class LynxContext;
class TouchEvent;
class CustomEvent;

struct BASE_EXPORT PubLynxContextDelegate {
  void (*touch_event_callback)(const std::string& name, int32_t tag, float x,
                               float y, float client_x, float client_y,
                               float page_x, float page_y,
                               void* data) = nullptr;
  void (*custom_event_callback)(const std::string& name, int32_t tag,
                                const pub::Value& params,
                                const std::string& params_name,
                                void* data) = nullptr;
  void (*multi_touch_event_callback)(const std::string& name, int32_t tag,
                                     float x, float y, float client_x,
                                     float client_y, float page_x, float page_y,
                                     void* data) = nullptr;
  void (*global_event_callback)(const pub::Value& params, void* data) = nullptr;
  void* data = nullptr;
  float screen_width = 0.f;
  float screen_height = 0.f;
  float device_pixel_ratio = 1.f;
};

class BASE_EXPORT PubLynxContext {
 public:
  PubLynxContext(
      PubUIOwner* ui_owner, PubShadowNodeOwner* node_owner,
      const std::shared_ptr<PubLynxContextDelegate>& delegate,
      const std::shared_ptr<pub::LynxResourceLoader>& resource_loader);
  const std::shared_ptr<LynxContext>& Context() const;

 private:
  std::shared_ptr<LynxContext> context_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_PUBLIC_PUB_LYNX_CONTEXT_H_
