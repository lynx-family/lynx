// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_LYNX_CLAY_PLATFORM_RENDERER_CLAY_H_
#define PLATFORM_LYNX_CLAY_PLATFORM_RENDERER_CLAY_H_

#include <functional>
#include <memory>
#include <utility>

#include "clay/gfx/gpu_object.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"

namespace clay {
class PendingContainerLayer;
}  // namespace clay

namespace lynx::tasm {

class ClayDisplayListRecording;
using RecordedClayDisplayList = std::shared_ptr<const ClayDisplayListRecording>;

// Produces CPU-only pictures on the same thread as the Fragment display list.
RecordedClayDisplayList RecordClayDisplayList(const DisplayList& list);

class PlatformRendererClay final : public PlatformRendererImpl {
 public:
  PlatformRendererClay(int id, PlatformRendererType type,
                       const base::String& tag,
                       const PlatformRendererInitConfig& config,
                       std::function<void()> request_paint);

  // Paired with the immediately following display-list update on the UI queue.
  void SetPendingRecording(RecordedClayDisplayList recording) {
    pending_recording_ = std::move(recording);
  }

  std::unique_ptr<::clay::PendingContainerLayer> BuildPendingLayers(
      const fml::RefPtr<::clay::GPUUnrefQueue>& unref_queue) const;

 protected:
  void OnUpdateDisplayList(DisplayList display_list) override;
  void OnUpdateAttributes(const fml::RefPtr<PropBundle>&) override {}
  void OnAddChild(PlatformRenderer*, int, bool) override {}
  void OnRemoveFromParent(bool) override;
  void OnUpdateSubtreeProperties(const DisplayList&) override {}

 private:
  const std::function<void()> request_paint_;
  RecordedClayDisplayList pending_recording_;
  RecordedClayDisplayList current_recording_;
};

class PlatformRendererClayFactory final : public PlatformRendererFactory {
 public:
  explicit PlatformRendererClayFactory(std::function<void()> request_paint)
      : request_paint_(std::move(request_paint)) {}

  fml::RefPtr<PlatformRenderer> CreateRenderer(
      int id, PlatformRendererType type,
      const fml::RefPtr<PropBundle>& init_data,
      const PlatformRendererInitConfig& config) override;
  fml::RefPtr<PlatformRenderer> CreateExtendedRenderer(
      int id, const base::String& tag, const fml::RefPtr<PropBundle>& init_data,
      const PlatformRendererInitConfig& config) override;

 private:
  const std::function<void()> request_paint_;
};

}  // namespace lynx::tasm

#endif  // PLATFORM_LYNX_CLAY_PLATFORM_RENDERER_CLAY_H_
