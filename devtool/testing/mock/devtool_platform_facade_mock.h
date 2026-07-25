// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_TESTING_MOCK_DEVTOOL_PLATFORM_FACADE_MOCK_H_
#define DEVTOOL_TESTING_MOCK_DEVTOOL_PLATFORM_FACADE_MOCK_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "devtool/base_devtool/native/public/devtool_status.h"
#include "devtool/lynx_devtool/agent/devtool_platform_facade.h"
#include "devtool/lynx_devtool/agent/input/input_event_target.h"
#include "devtool/lynx_devtool/base/mouse_event.h"

namespace lynx {
namespace testing {

class InputEventTargetMock : public input::InputEventTarget {
 public:
  input::PointerCapabilities GetPointerCapabilities() const override {
    std::lock_guard<std::mutex> lock(mutex_);
    return capabilities_;
  }

  bool InjectPointerEvent(const input::PointerEvent& event) override {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.push_back(event);
    return injection_result_;
  }

  void WaitForInputProcessed(std::function<void(bool)> callback) override {
    bool result = true;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      result = processing_result_;
    }
    callback(result);
  }

  void SetCapabilities(const input::PointerCapabilities& capabilities) {
    std::lock_guard<std::mutex> lock(mutex_);
    capabilities_ = capabilities;
  }

  void SetInjectionResult(bool result) {
    std::lock_guard<std::mutex> lock(mutex_);
    injection_result_ = result;
  }

  void SetProcessingResult(bool result) {
    std::lock_guard<std::mutex> lock(mutex_);
    processing_result_ = result;
  }

  std::vector<input::PointerEvent> Events() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return events_;
  }

 private:
  mutable std::mutex mutex_;
  input::PointerCapabilities capabilities_{input::PointerSourceType::kTouch,
                                           true, false};
  bool injection_result_ = true;
  bool processing_result_ = true;
  std::vector<input::PointerEvent> events_;
};

class DevToolPlatformFacadeMock : public lynx::devtool::DevToolPlatformFacade {
 public:
  DevToolPlatformFacadeMock()
      : input_event_target_(std::make_shared<InputEventTargetMock>()) {}
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

  std::vector<double> GetBoxModel(
      const devtool::InspectorBoxModelQuery& query) override;
  std::vector<float> GetTransformValue(
      int identifier,
      const std::vector<float>& pad_border_margin_layout) override;

  void StartScreenCast(devtool::ScreenshotRequest request) override;
  void StopScreenCast() override;
  void OnAckReceived() override;
  void GetLynxScreenShot() override;
  void EmulateTouch(std::shared_ptr<lynx::devtool::MouseEvent> input) override {
  }
  void InsertText(const std::string& text) override { inserted_text_ = text; }
  std::shared_ptr<input::InputEventTarget> GetInputEventTarget() override {
    return input_event_target_;
  }

  std::string GetDebugInfoByUrl(const std::string& url) override {
    return devtool::DevToolStatus::NO_DEBUG_INFO_FOUND_BY_URL;
  }

  void PageReload(bool ignore_cache, const std::string& template_binary = "",
                  const std::string& reload_url = "",
                  bool from_template_fragments = false,
                  int32_t template_size = 0) override {}

  void Navigate(const std::string& url) override {}

  lynx::lepus::Value* GetLepusValueFromTemplateData() override;
  std::string GetTemplateJsInfo(int32_t offset, int32_t size) override;

  std::string GetLepusDebugInfo(const std::string& url) override {
    return "test GetLepusDebugInfo";
  }

  std::unordered_map<std::string, bool> devtools_switch_;
  std::string inserted_text_;
  std::vector<devtool::InspectorBoxModelQuery> box_model_queries_;
  std::vector<devtool::ScreenshotRequest> screen_cast_requests_;
  std::vector<double> box_model_response_;
  std::vector<int> transform_value_ids_;
  std::vector<std::vector<float>> transform_value_inputs_;
  std::vector<float> transform_value_response_;
  bool supports_overlay_box_model_ = false;
  std::shared_ptr<InputEventTargetMock> input_event_target_;

 protected:
  bool SupportsOverlayBoxModel() const override {
    return supports_overlay_box_model_;
  }
};

}  // namespace testing
}  // namespace lynx

#endif  // DEVTOOL_TESTING_MOCK_DEVTOOL_PLATFORM_FACADE_MOCK_H_
