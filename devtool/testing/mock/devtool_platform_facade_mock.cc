// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/testing/mock/devtool_platform_facade_mock.h"

namespace lynx {
namespace testing {
int DevToolPlatformFacadeMock::FindNodeIdForLocation(
    float x, float y, std::string screen_shot_mode) {
  return -1;
}

void DevToolPlatformFacadeMock::SetDevToolSwitch(const std::string& key,
                                                 bool value) {
  devtools_switch_.emplace(key, value);
}

std::string DevToolPlatformFacadeMock::GetLynxVersion() const {
  return "1.1.5";
}

void DevToolPlatformFacadeMock::OnReceiveTemplateFragment(
    const std::string& data, bool eof) {}

std::vector<int32_t> DevToolPlatformFacadeMock::GetViewLocationOnScreen()
    const {
  return {1, 1};
}

void DevToolPlatformFacadeMock::SendEventToVM(const std::string& vm_type,
                                              const std::string& event_name,
                                              const std::string& data) {}

std::vector<float> DevToolPlatformFacadeMock::GetRectToWindow() const {
  return {1, 1, 1, 1};
}

void DevToolPlatformFacadeMock::StartScreenCast(
    devtool::ScreenshotRequest request) {}
void DevToolPlatformFacadeMock::StopScreenCast() {}
void DevToolPlatformFacadeMock::OnAckReceived() {}
void DevToolPlatformFacadeMock::GetLynxScreenShot() {}

lynx::lepus::Value* DevToolPlatformFacadeMock::GetLepusValueFromTemplateData() {
  return nullptr;
}
std::string DevToolPlatformFacadeMock::GetTemplateJsInfo(int32_t offset,
                                                         int32_t size) {
  return "";
}

}  // namespace testing
}  // namespace lynx
