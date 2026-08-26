// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_RENDERER_CONTEXT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_RENDERER_CONTEXT_H_

#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

#include "base/include/fml/memory/ref_counted.h"

namespace lynx {
namespace tasm {
namespace harmony {

class LynxContext;
class UIOwner;

class LynxRendererContext {
 public:
  explicit LynxRendererContext(std::weak_ptr<LynxContext> context)
      : context_(std::move(context)) {}
  ~LynxRendererContext() = default;

  UIOwner* GetUIOwner() const;
  std::shared_ptr<LynxContext> GetLynxContext() const;

  void UpdateTextBundle(
      int32_t id, fml::RefPtr<fml::RefCountedThreadSafeStorage> text_bundle);
  void DestroyTextBundle(int32_t id);
  fml::RefPtr<fml::RefCountedThreadSafeStorage> GetTextBundle(int32_t id) const;

 private:
  std::weak_ptr<LynxContext> context_;
  mutable std::mutex text_bundles_mutex_;
  std::unordered_map<int32_t, fml::RefPtr<fml::RefCountedThreadSafeStorage>>
      text_bundles_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_RENDERER_LYNX_RENDERER_CONTEXT_H_
