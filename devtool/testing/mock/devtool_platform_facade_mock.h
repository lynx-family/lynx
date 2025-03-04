// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_TESTING_MOCK_DEVTOOL_PLATFORM_FACADE_MOCK_H_
#define DEVTOOL_TESTING_MOCK_DEVTOOL_PLATFORM_FACADE_MOCK_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "devtool/lynx_devtool/agent/devtool_platform_facade.h"
#include "devtool/lynx_devtool/base/mouse_event.h"

namespace lynx {
namespace testing {

class DevToolPlatformFacadeMock : public lynx::devtool::DevToolPlatformFacade {
 public:
  DevToolPlatformFacadeMock() = default;
  ~DevToolPlatformFacadeMock() override = default;

  int FindNodeIdForLocation(float x, float y,
                            std::string screen_shot_mode) override;
  void ScrollIntoView(int node_id) override {}

  void SetDevToolSwitch(const std::string& key, bool value) override;

  std::string GetLynxVersion() const override;

  void OnReceiveTemplateFragment(const std::string& data, bool eof) override;

  std::vector<int32_t> GetViewLocationOnScreen() const override;

  void SendEventToVM(const std::string& vm_type, const std::string& event_name,
                     const std::string& data) override;
  std::vector<float> GetRectToWindow() const override;

  void StartScreenCast(devtool::ScreenshotRequest request) override;
  void StopScreenCast() override;
  void OnAckReceived() override;
  void GetLynxScreenShot() override;
  void EmulateTouch(std::shared_ptr<lynx::devtool::MouseEvent> input) override {
  }

  void PageReload(bool ignore_cache, std::string template_binary = "",
                  bool from_template_fragments = false,
                  int32_t template_size = 0) override {}
  void Navigate(const std::string& url) override {}

  lynx::lepus::Value* GetLepusValueFromTemplateData() override;
  std::string GetTemplateJsInfo(int32_t offset, int32_t size) override;

  std::string GetLepusDebugInfo(const std::string& url) override {
    return "test GetLepusDebugInfo";
  }
  void SetLepusDebugInfoUrl(const std::string& url) override {
    debug_info_url_ = url;
  }

  std::unordered_map<std::string, bool> devtools_switch_;
  std::string debug_info_url_;
};

}  // namespace testing
}  // namespace lynx

#endif  // DEVTOOL_TESTING_MOCK_DEVTOOL_PLATFORM_FACADE_MOCK_H_
