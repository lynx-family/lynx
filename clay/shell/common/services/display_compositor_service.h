// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_DISPLAY_COMPOSITOR_SERVICE_H_
#define CLAY_SHELL_COMMON_SERVICES_DISPLAY_COMPOSITOR_SERVICE_H_
#include <memory>
#include <utility>

#include "clay/common/service/service.h"
#include "clay/flow/compositor/compositor_frame_builder.h"
#include "clay/shell/common/display_compositor/display_compositor.h"
#include "clay/shell/common/display_compositor/frame_mailbox.h"
#include "clay/shell/common/services/display_frame_service.h"
namespace clay {
class DisplayCompositorService
    : public Service<DisplayCompositorService, Owner::kPlatform> {
 public:
  static std::shared_ptr<DisplayCompositorService> Create();
  void SetBinding(display::RasterBindingKey binding,
                  std::shared_ptr<display::FrameMailbox> mailbox);
  void SetOutputConfig(display::OutputConfig config);
  void NotifyLayoutSubmitted(uint64_t version);
  void Attach(std::unique_ptr<display::DisplayTarget> target,
              display::WindowKey window,
              display::DisplayCompositor::Presented presented);
  void Detach();
  void Close();
  void SetEnabled(bool enabled);
  void SetErrorHandler(std::function<void(display::PipelineError)> handler) {
    error_ = std::move(handler);
  }
  std::function<void()> GetWakeCallback();
  std::shared_ptr<const display::CompositorFrame> CurrentFrame() const;

 private:
  void OnInit(ServiceManager&, const PlatformServiceContext&) override;
  void OnDestroy() override;
  void Drain();
  fml::RefPtr<fml::TaskRunner> runner_;
  Puppet<Owner::kPlatform, DisplayFrameService> clock_;
  display::CompositorFrameBuilder builder_;
  std::shared_ptr<display::FrameMailbox> mailbox_;
  std::shared_ptr<display::DisplayCompositor> compositor_;
  display::WindowKey window_;
  bool enabled_ = true;
  std::function<void(display::PipelineError)> error_;
};
}  // namespace clay
#endif  // CLAY_SHELL_COMMON_SERVICES_DISPLAY_COMPOSITOR_SERVICE_H_
