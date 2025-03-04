// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_UI_DELEGATE_H_
#define CORE_PUBLIC_UI_DELEGATE_H_

#include <memory>

#include "base/include/fml/task_runner.h"
#include "core/public/jsb/native_module_factory.h"
#include "core/public/layout_ctx_platform_impl.h"
#include "core/public/lynx_engine_proxy.h"
#include "core/public/lynx_resource_loader.h"
#include "core/public/lynx_runtime_proxy.h"
#include "core/public/painting_ctx_platform_impl.h"
namespace lynx {
namespace tasm {

/*
 * UIDelegate is used for communication between LynxShell and UI rendering
 * module.
 *
 * It can take some initialization parameters from the UI rendering module and
 * pass some objects to the UI rendering module after initialization.
 */
class UIDelegate {
 public:
  virtual ~UIDelegate() = default;
  virtual std::unique_ptr<PaintingCtxPlatformImpl> CreatePaintingContext() = 0;
  virtual std::unique_ptr<LayoutCtxPlatformImpl> CreateLayoutContext() = 0;
  virtual std::unique_ptr<PropBundleCreator> CreatePropBundleCreator() = 0;
  virtual std::unique_ptr<piper::NativeModuleFactory>
  GetCustomModuleFactory() = 0;

  // Indicates whether to use logical pixels as the layout unit on current
  // platform.
  // If true, the layout unit is logical pixels, otherwise it is physical
  // pixels.
  virtual bool UsesLogicalPixels() const = 0;

  virtual void OnLynxCreate(
      const std::shared_ptr<shell::LynxEngineProxy>& engine_proxy,
      const std::shared_ptr<shell::LynxRuntimeProxy>& runtime_proxy,
      const std::shared_ptr<pub::LynxResourceLoader>& resource_loader,
      const fml::RefPtr<fml::TaskRunner>& ui_task_runner,
      const fml::RefPtr<fml::TaskRunner>& layout_task_runner) = 0;

  virtual void OnUpdateScreenMetrics(float width, float height,
                                     float device_pixel_ratio) {}

  void SetInstanceId(int32_t id) { instance_id_ = id; }
  int32_t GetInstanceId() const { return instance_id_; }

 private:
  // Represents an unknown instance ID. Typically set proactively during event
  // reporting, indicating that the current event does not need to distinguish
  // the LynxShell runtime environment and does not need to associate common
  // parameters.
  int32_t instance_id_ = -1;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_PUBLIC_UI_DELEGATE_H_
