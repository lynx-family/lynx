// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_UI_DELEGATE_CLAY_H_
#define CLAY_LYNX_ADAPTOR_UI_DELEGATE_CLAY_H_

#include <memory>
#include <vector>

#include "clay/lynx_adaptor/lynx_event_dispatcher.h"
#include "core/public/lynx_resource_loader.h"
#include "core/public/ui_delegate.h"

namespace clay {
class ViewContext;
}

namespace lynx {
namespace tasm {

class PaintingContextClay;
class LayoutContextClay;

class UIDelegateClay : public UIDelegate {
 public:
  UIDelegateClay(
      clay::ViewContext* view_context,
      std::unique_ptr<lynx::runtime::NativeModuleFactory> module_factory);
  ~UIDelegateClay() override;

  std::unique_ptr<PaintingCtxPlatformImpl> CreatePaintingContext() override;
  std::unique_ptr<LayoutCtxPlatformImpl> CreateLayoutContext() override;
  std::unique_ptr<PropBundleCreator> CreatePropBundleCreator() override;
  std::unique_ptr<runtime::NativeModuleFactory> GetCustomModuleFactory()
      override;
  bool UsesLogicalPixels() const override;

  double GetScreenScaleFactor() const override;

  void OnLynxCreate(
      const std::shared_ptr<shell::ListEngineProxy>& list_engine_proxy,
      const std::shared_ptr<shell::LynxEngineProxy>& engine_proxy,
      const std::shared_ptr<shell::LynxRuntimeProxy>& runtime_proxy,
      const std::shared_ptr<shell::LynxLayoutProxy>& layout_proxy,
      const std::shared_ptr<shell::PerfControllerProxy>& perf_controller_proxy,
      const std::shared_ptr<pub::LynxResourceLoader>& resource_loader,
      const fml::RefPtr<fml::TaskRunner>& ui_task_runner,
      const fml::RefPtr<fml::TaskRunner>& layout_task_runner,
      int32_t instance_id, bool is_embedded_mode = false) override;

  void TakeSnapshot(
      size_t max_width, size_t max_height, int quality,
      float screen_scale_factor,
      const lynx::fml::RefPtr<lynx::fml::TaskRunner>& screenshot_runner,
      TakeSnapshotCompletedCallback callback) override;

  int GetNodeForLocation(int x, int y) override;

  std::vector<float> GetTransformValue(
      int id, const std::vector<float>& pad_border_margin_layout) override;

  clay::ViewContext* GetViewContext() const { return view_context_; }

 private:
  clay::ViewContext* view_context_;
  std::unique_ptr<runtime::NativeModuleFactory> module_factory_;
  std::unique_ptr<clay::LynxEventDispatcher> event_dispatcher_;
  // Save a PaintingContextClay raw pointer to set the LynxEngineProxy and
  // LynxRuntimeProxy objects after the Lynx instance is created.
  // Its lifecycle is managed by Lynx.
  PaintingContextClay* painting_context_ = nullptr;
  LayoutContextClay* layout_context_ = nullptr;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_UI_DELEGATE_CLAY_H_
