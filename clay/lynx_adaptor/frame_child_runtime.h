// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_FRAME_CHILD_RUNTIME_H_
#define CLAY_LYNX_ADAPTOR_FRAME_CHILD_RUNTIME_H_

#include <memory>
#include <optional>
#include <string>

#include "clay/ui/component/frame_child_data.h"

namespace lynx {
namespace pub {
class LynxResourceLoader;
}  // namespace pub
namespace tasm {

class LynxTemplateBundle;
class UIDelegateClay;

struct FrameChildRuntimeOptions {
  std::shared_ptr<pub::LynxResourceLoader> resource_loader;
  float device_pixel_ratio = 1.f;
  float viewport_width = 0.f;
  float viewport_height = 0.f;
  int embedded_mode = 0;
  std::optional<bool> enable_multi_async_thread;
};

// Owns the child Lynx runtime while Clay owns its PageView and compositor
// surface. Platform integrations can use their native Lynx runtime without
// changing the Clay frame rendering path.
class FrameChildRuntime {
 public:
  virtual ~FrameChildRuntime() = default;

  virtual bool LoadBundle(
      const std::string& url, const LynxTemplateBundle& bundle,
      const std::shared_ptr<FrameChildData>& data,
      const std::shared_ptr<FrameChildData>& global_props) = 0;
  virtual bool UpdateMetaData(
      const std::shared_ptr<FrameChildData>& data,
      const std::shared_ptr<FrameChildData>& global_props) = 0;
  virtual void UpdateViewport(float width, int width_mode, float height,
                              int height_mode, bool need_layout) = 0;
};

class FrameChildRuntimeFactory {
 public:
  virtual ~FrameChildRuntimeFactory() = default;

  virtual std::unique_ptr<FrameChildRuntime> CreateRuntime(
      UIDelegateClay* ui_delegate, const FrameChildRuntimeOptions& options) = 0;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_FRAME_CHILD_RUNTIME_H_
