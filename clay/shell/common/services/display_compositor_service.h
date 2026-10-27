// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_DISPLAY_COMPOSITOR_SERVICE_H_
#define CLAY_SHELL_COMMON_SERVICES_DISPLAY_COMPOSITOR_SERVICE_H_
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

#include "clay/common/service/service.h"
#include "clay/flow/compositor/compositor_frame_builder.h"
#include "clay/shell/common/display/display_compositor.h"
#include "clay/shell/common/display/frame_mailbox.h"
#include "clay/shell/common/services/display_frame_service.h"
namespace clay {
class DisplayCompositorService
    : public Service<DisplayCompositorService, Owner::kRaster> {
 public:
  using TargetFactory =
      std::function<std::unique_ptr<display::DisplayTarget>()>;
  using Snapshot = display::DisplayCompositor::Snapshot;

  static std::shared_ptr<DisplayCompositorService> Create();
  void SetBinding(display::RasterBindingKey binding,
                  std::shared_ptr<display::FrameMailbox> mailbox);
  void SetOutputConfig(display::OutputConfig config);
  void NotifyLayoutSubmitted(uint64_t version);
  void Attach(TargetFactory factory, display::WindowKey window,
              display::DisplayCompositor::Presented presented,
              std::function<void()> request_retained, Snapshot snapshot);
  void Detach();
  void Close();
  void SetEnabled(bool enabled);
  void SetHybridCompositionActive(bool active);
  void SetErrorHandler(std::function<void(display::PipelineError)> handler) {
    error_ = std::move(handler);
  }
  void Drain();

 private:
  void OnInit(ServiceManager&, const RasterServiceContext&) override;
  void OnDestroy() override;
  void StartTarget();
  void StopTarget();
  void ReportError(display::PipelineError error);

  fml::RefPtr<fml::TaskRunner> runner_;
  fml::RefPtr<fml::TaskRunner> platform_runner_;
  Puppet<Owner::kRaster, DisplayFrameService> clock_;
  display::CompositorFrameBuilder builder_;
  std::shared_ptr<display::FrameMailbox> mailbox_;
  std::shared_ptr<display::DisplayCompositor> compositor_;
  std::shared_ptr<const display::CompositorFrame> retained_frame_;
  display::OutputConfig config_;
  display::WindowKey window_;
  TargetFactory factory_;
  display::DisplayCompositor::Presented presented_;
  std::function<void()> request_retained_;
  Snapshot snapshot_;
  bool enabled_ = true;
  bool hybrid_composition_active_ = false;
  std::function<void(display::PipelineError)> error_;
  std::shared_ptr<std::atomic<uint64_t>> error_epoch_ =
      std::make_shared<std::atomic<uint64_t>>(0);
};
}  // namespace clay
#endif  // CLAY_SHELL_COMMON_SERVICES_DISPLAY_COMPOSITOR_SERVICE_H_
