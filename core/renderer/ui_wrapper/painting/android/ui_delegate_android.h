// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_UI_DELEGATE_ANDROID_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_UI_DELEGATE_ANDROID_H_

#include <jni.h>

#include <memory>

#include "core/public/ui_delegate.h"

namespace lynx {
namespace tasm {

class UIDelegateAndroid : public UIDelegate {
 public:
  static bool RegisterJNI(JNIEnv* env);

  UIDelegateAndroid(long painting_context, long layout_context)
      : painting_context_(
            reinterpret_cast<PaintingCtxPlatformImpl*>(painting_context)),
        layout_context_(
            reinterpret_cast<LayoutCtxPlatformImpl*>(layout_context)) {}

  ~UIDelegateAndroid() override = default;

  std::unique_ptr<PaintingCtxPlatformImpl> CreatePaintingContext() override;
  std::unique_ptr<LayoutCtxPlatformImpl> CreateLayoutContext() override;
  std::unique_ptr<PropBundleCreator> CreatePropBundleCreator() override;

  bool UsesLogicalPixels() const override { return false; }

  std::unique_ptr<piper::NativeModuleFactory> GetCustomModuleFactory()
      override {
    return nullptr;
  }
  void OnLynxCreate(
      const std::shared_ptr<shell::LynxEngineProxy>& engine_proxy,
      const std::shared_ptr<shell::LynxRuntimeProxy>& runtime_proxy,
      const std::shared_ptr<pub::LynxResourceLoader>& resource_loader,
      const fml::RefPtr<fml::TaskRunner>& ui_task_runner,
      const fml::RefPtr<fml::TaskRunner>& layout_task_runner) override {}

 protected:
  PaintingCtxPlatformImpl* painting_context_;
  LayoutCtxPlatformImpl* layout_context_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_UI_DELEGATE_ANDROID_H_
