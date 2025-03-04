// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_UI_DELEGATE_DARWIN_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_UI_DELEGATE_DARWIN_H_

#include <memory>

#import "LynxShadowNodeOwner.h"
#import "LynxUIOwner.h"
#include "core/public/ui_delegate.h"

namespace lynx {
namespace tasm {

class UIDelegateDarwin : public UIDelegate {
 public:
  UIDelegateDarwin(LynxUIOwner* ui_owner, bool enable_create_ui_async,
                   LynxShadowNodeOwner* shadow_node_owner)
      : ui_owner_(ui_owner),
        enable_create_ui_async_(enable_create_ui_async),
        shadow_node_owner_(shadow_node_owner) {}
  ~UIDelegateDarwin() override = default;

  std::unique_ptr<PaintingCtxPlatformImpl> CreatePaintingContext() override;
  std::unique_ptr<LayoutCtxPlatformImpl> CreateLayoutContext() override;
  std::unique_ptr<PropBundleCreator> CreatePropBundleCreator() override;

  bool UsesLogicalPixels() const override { return true; }

  std::unique_ptr<piper::NativeModuleFactory> GetCustomModuleFactory()
      override {
    // TODO(chenyouhui): Implement this after unify lepus module and js module.
    return nullptr;
  }
  void OnLynxCreate(
      const std::shared_ptr<shell::LynxEngineProxy>& engine_proxy,
      const std::shared_ptr<shell::LynxRuntimeProxy>& runtime_proxy,
      const std::shared_ptr<pub::LynxResourceLoader>& resource_loader,
      const fml::RefPtr<fml::TaskRunner>& ui_task_runner,
      const fml::RefPtr<fml::TaskRunner>& layout_task_runner) override {}

 private:
  __weak LynxUIOwner* ui_owner_;
  bool enable_create_ui_async_;
  __weak LynxShadowNodeOwner* shadow_node_owner_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_UI_DELEGATE_DARWIN_H_
