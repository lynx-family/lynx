// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_DEVTOOL_LYNX_DEVTOOL_PROXY_H_
#define CORE_PUBLIC_DEVTOOL_LYNX_DEVTOOL_PROXY_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "core/public/devtool/lynx_inspector_owner.h"
#include "core/public/ui_delegate.h"

namespace lynx {

namespace tasm {
class TemplateData;
}  // namespace tasm

namespace devtool {
class LynxDevToolProxy {
 public:
  virtual void ReloadTemplate(
      const std::string& url, const std::vector<uint8_t>& source,
      const std::shared_ptr<tasm::TemplateData>& data) = 0;
  virtual void LoadTemplateFromURL(
      const std::string& url,
      const std::shared_ptr<tasm::TemplateData> data = nullptr) = 0;
  virtual double GetScreenScaleFactor() = 0;
  virtual void TakeSnapshot(
      size_t max_width, size_t max_height, int quality,
      float screen_scale_factor,
      const lynx::fml::RefPtr<lynx::fml::TaskRunner>& screenshot_runner,
      tasm::TakeSnapshotCompletedCallback callback) = 0;
  virtual int GetNodeForLocation(int x, int y) = 0;
  virtual std::vector<float> GetTransformValue(
      int id, const std::vector<float>& pad_border_margin_layout) = 0;
  virtual void SetInspectorOwner(LynxInspectorOwner* owner) = 0;
  virtual void EmulateTouch(const std::string& event_type, int x, int y,
                            const std::string& button, float delta_x,
                            float delta_y, int modifiers, int click_count) = 0;
};
}  // namespace devtool
}  // namespace lynx
#endif  // CORE_PUBLIC_DEVTOOL_LYNX_DEVTOOL_PROXY_H_
