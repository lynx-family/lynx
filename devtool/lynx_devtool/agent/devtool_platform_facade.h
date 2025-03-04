// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DEVTOOL_PLATFORM_FACADE_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DEVTOOL_PLATFORM_FACADE_H_

#include <memory>
#include <string>
#include <vector>

#include "core/renderer/dom/element.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "devtool/lynx_devtool/base/mouse_event.h"
#include "devtool/lynx_devtool/base/screen_metadata.h"

namespace lynx {
namespace devtool {

class LynxDevToolMediator;
class InspectorUIExecutor;
class InspectorTasmExecutor;
class InspectorJavaScriptDebuggerImpl;
class InspectorLepusDebuggerImpl;

class DevToolPlatformFacade
    : public std::enable_shared_from_this<DevToolPlatformFacade> {
 public:
  DevToolPlatformFacade() = default;
  virtual ~DevToolPlatformFacade();

  void InitWithDevToolMediator(
      std::shared_ptr<LynxDevToolMediator> devtool_mediator);

  const std::weak_ptr<InspectorJavaScriptDebuggerImpl>& GetJSDebugger() {
    return js_debugger_wp_;
  }

  virtual lynx::lepus::Value* GetLepusValueFromTemplateData() = 0;
  virtual std::string GetTemplateJsInfo(int32_t offset, int32_t size) = 0;

  virtual void ScrollIntoView(int node_id) = 0;
  virtual int FindNodeIdForLocation(float x, float y,
                                    std::string screen_shot_mode) = 0;
  virtual void StartScreenCast(ScreenshotRequest request) = 0;
  virtual void StopScreenCast() = 0;
  virtual void PageReload(bool ignore_cache, std::string template_binary = "",
                          bool from_template_fragments = false,
                          int32_t template_size = 0) = 0;
  virtual void Navigate(const std::string& url) = 0;
  virtual void OnAckReceived() = 0;
  virtual void GetLynxScreenShot() = 0;

  virtual void EmulateTouch(std::shared_ptr<lynx::devtool::MouseEvent>) = 0;

  virtual std::string GetUINodeInfo(int id) { return ""; }
  virtual std::string GetLynxUITree() { return ""; }
  virtual int SetUIStyle(int id, std::string name, std::string content) {
    return 0;
  }

  void SendPageScreencastFrameEvent(const std::string& data,
                                    std::shared_ptr<ScreenMetadata> metadata);
  void SendPageScreencastVisibilityChangedEvent(bool status);
  void SendLynxScreenshotCapturedEvent(const std::string& data);
  void SendPageFrameNavigatedEvent(const std::string& url);
  void SendConsoleEvent(const lynx::piper::ConsoleMessage& message);
  void SendLayerTreeDidChangeEvent();

  virtual std::vector<double> GetBoxModel(tasm::Element* element) {
    return std::vector<double>();
  }
  virtual std::vector<float> GetTransformValue(
      int identifier, const std::vector<float>& pad_border_margin_layout) {
    return std::vector<float>();
  }

  virtual void SetDevToolSwitch(const std::string& key, bool value) = 0;

  virtual std::vector<float> GetRectToWindow() const = 0;

  virtual std::string GetLynxVersion() const = 0;

  virtual void OnReceiveTemplateFragment(const std::string& data, bool eof) = 0;

  virtual std::vector<int32_t> GetViewLocationOnScreen() const = 0;

  virtual void SendEventToVM(const std::string& vm_type,
                             const std::string& event_name,
                             const std::string& data) = 0;

  // The following functions are used for console delegate and only work on
  // Android/iOS.
  virtual void OnConsoleMessage(const std::string& message) {}
  virtual void OnConsoleObject(const std::string& detail, int callback_id) {}

  virtual std::string GetLepusDebugInfo(const std::string& url) { return ""; }
  virtual void SetLepusDebugInfoUrl(const std::string& url) {}

 protected:
  // This function is shared across multiple platforms
  // and will be called in the GetBoxModel method of subclasses.
  // It is used to retrieve the box model information for an element.
  std::vector<double> GetBoxModelInGeneralPlatform(tasm::Element* element);

 private:
  std::weak_ptr<InspectorUIExecutor> inspector_ui_executor_wp_;
  // std::weak_ptr<InspectorTasmExecutor> inspector_element_executor_wp_;
  std::weak_ptr<InspectorJavaScriptDebuggerImpl> js_debugger_wp_;
  std::weak_ptr<LynxDevToolMediator> devtool_mediator_wp_;
  std::weak_ptr<InspectorLepusDebuggerImpl> lepus_debugger_wp_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_DEVTOOL_PLATFORM_FACADE_H_
