// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_UI_DELEGATE_H_
#define CORE_PUBLIC_UI_DELEGATE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "core/public/box_model.h"
#include "core/public/event_tracker_proxy.h"
#include "core/public/jsb/native_module_factory.h"
#include "core/public/layout_ctx_platform_impl.h"
#include "core/public/list_engine_proxy.h"
#include "core/public/lynx_engine_proxy.h"
#include "core/public/lynx_layout_proxy.h"
#include "core/public/lynx_resource_loader.h"
#include "core/public/lynx_runtime_proxy.h"
#include "core/public/painting_ctx_platform_impl.h"
#include "core/public/perf_controller_proxy.h"

namespace lynx {
namespace tasm {
using TakeSnapshotCompletedCallback =
    std::function<void(const std::string&, float timestamp, float device_width,
                       float device_height, float page_scale_factor)>;
class PageConfig;
/*
 * UIDelegate is used for communication between LynxShell and UI rendering
 * module.
 *
 * It can take some initialization parameters from the UI rendering module and
 * pass some objects to the UI rendering module after initialization.
 */
class UIDelegate {
 public:
  UIDelegate() : weak_flag_(std::make_shared<WeakFlag>(this)) {}
  virtual ~UIDelegate() = default;
  virtual std::unique_ptr<PaintingCtxPlatformImpl> CreatePaintingContext() = 0;
  virtual std::unique_ptr<LayoutCtxPlatformImpl> CreateLayoutContext() = 0;
  virtual std::unique_ptr<PropBundleCreator> CreatePropBundleCreator() = 0;
  virtual std::unique_ptr<runtime::NativeModuleFactory>
  GetCustomModuleFactory() = 0;

  // Indicates whether to use logical pixels as the layout unit on current
  // platform.
  // If true, the layout unit is logical pixels, otherwise it is physical
  // pixels.
  virtual bool UsesLogicalPixels() const = 0;
  // Get real device pixel ratio of the screen which LynxView current
  // displaying
  virtual double GetScreenScaleFactor() const { return 1.0; }
  virtual void OnLynxCreate(
      const std::shared_ptr<shell::ListEngineProxy>& list_engine_proxy,
      const std::shared_ptr<shell::LynxEngineProxy>& engine_proxy,
      const std::shared_ptr<shell::LynxRuntimeProxy>& runtime_proxy,
      const std::shared_ptr<shell::LynxLayoutProxy>& layout_proxy,
      const std::shared_ptr<shell::PerfControllerProxy>& perf_controller_proxy,
      const std::shared_ptr<shell::EventTrackerProxy>& event_tracker_proxy,
      const std::shared_ptr<pub::LynxResourceLoader>& resource_loader,
      const fml::RefPtr<fml::TaskRunner>& ui_task_runner,
      const fml::RefPtr<fml::TaskRunner>& layout_task_runner,
      int32_t instance_id, bool is_embedded_mode = false) = 0;

  virtual void OnUpdateScreenMetrics(float width, float height,
                                     float device_pixel_ratio) {}

  void SetInstanceId(int32_t id) { instance_id_ = id; }
  int32_t GetInstanceId() const { return instance_id_; }
  virtual void OnPageConfigDecoded(const std::shared_ptr<PageConfig>& config) {}
  virtual void TakeSnapshot(
      size_t max_width, size_t max_height, int quality,
      float screen_scale_factor,
      const lynx::fml::RefPtr<lynx::fml::TaskRunner>& screenshot_runner,
      TakeSnapshotCompletedCallback callback) {}
  virtual int GetNodeForLocation(int x, int y) { return -1; }
  virtual std::vector<float> GetTransformValue(
      int id, const std::vector<float>& pad_border_margin_layout) {
    return {};
  }
  virtual std::string GetLynxUITree() { return std::string(); }
  virtual std::string GetUINodeInfo(int id) { return std::string(); }
  virtual int SetUIStyle(int id, const std::string& name,
                         const std::string& content) {
    return 0;
  }

  // WeakFlag is used to create a weak reference to the UIDelegate instance.
  // This allows checking if the UIDelegate is still alive before accessing it.
  struct WeakFlag : public std::enable_shared_from_this<WeakFlag> {
    explicit WeakFlag(UIDelegate* ui_delegate) : ui_delegate(ui_delegate) {}
    UIDelegate* ui_delegate;
  };

  std::weak_ptr<WeakFlag> GetWeakFlag() { return weak_flag_; }

 private:
  // Represents an unknown instance ID. Typically set proactively during event
  // reporting, indicating that the current event does not need to distinguish
  // the LynxShell runtime environment and does not need to associate common
  // parameters.
  int32_t instance_id_ = -1;
  std::shared_ptr<WeakFlag> weak_flag_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_PUBLIC_UI_DELEGATE_H_
