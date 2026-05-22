// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_DEVTOOL_PLATFORM_FACADE_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_DEVTOOL_PLATFORM_FACADE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "base/include/value/base_value.h"
#include "core/renderer/dom/element.h"
#include "devtool/lynx_devtool/base/mouse_event.h"
#include "devtool/lynx_devtool/base/screen_metadata.h"

namespace lynx {
namespace devtool {

class LynxDevToolMediator;
class InspectorUIExecutor;
class InspectorTasmExecutor;
class InspectorJavaScriptDebuggerImpl;
class InspectorLepusDebuggerImpl;

struct InspectorLayoutObjectInfo {
  // Element/layout object id used to resolve layout data across threads.
  int32_t id = 0;
  // Whether this object already carries a full layout snapshot.
  bool has_snapshot = false;
  double border_bound_width = 0;
  double border_bound_height = 0;
  double layout_padding_left = 0;
  double layout_padding_top = 0;
  double layout_padding_right = 0;
  double layout_padding_bottom = 0;
  double layout_border_left_width = 0;
  double layout_border_top_width = 0;
  double layout_border_right_width = 0;
  double layout_border_bottom_width = 0;
  double layout_margin_left = 0;
  double layout_margin_top = 0;
  double layout_margin_right = 0;
  double layout_margin_bottom = 0;
  double border_bound_left_from_parent_padding_bound = 0;
  double border_bound_top_from_parent_padding_bound = 0;
};

InspectorLayoutObjectInfo BuildLayoutObjectInfo(int32_t id, SLNode* layout_obj);

struct InspectorBoxModelQuery {
  // Layout info for the inspected element.
  InspectorLayoutObjectInfo layout_object;
  // The nearest node used as the transform anchor when the inspected element is
  // layout-only; otherwise it is usually the same as layout_object.
  InspectorLayoutObjectInfo transform_node;
  // Layout-only node chain from the inspected element up to the transform node,
  // used to accumulate offsets for elements without a UI primitive.
  std::vector<InspectorLayoutObjectInfo> layout_only_nodes;
  // Precomputed box model for overlay elements whose layout is handled
  // specially.
  std::vector<double> overlay_box_model;
  // Whether the inspected element owns a native UI primitive.
  bool has_ui_primitive = true;
  // Whether the inspected element is an overlay element.
  bool is_overlay = false;
};

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

  virtual std::string GetDebugInfoByUrl(const std::string& url) = 0;
  virtual void ScrollIntoView(int node_id) = 0;
  virtual void Focus(int node_id) {}
  virtual int FindNodeIdForLocation(float x, float y,
                                    std::string screen_shot_mode) = 0;
  virtual void StartScreenCast(ScreenshotRequest request) = 0;
  virtual void StopScreenCast() = 0;
  virtual void PageReload(bool ignore_cache,
                          const std::string& template_binary = "",
                          const std::string& reload_url = "",
                          bool from_template_fragments = false,
                          int32_t template_size = 0) = 0;
  virtual void Navigate(const std::string& url) = 0;
  virtual void OnAckReceived() = 0;
  virtual void GetLynxScreenShot() = 0;

  virtual void EmulateTouch(std::shared_ptr<lynx::devtool::MouseEvent>) = 0;
  virtual void InsertText(const std::string& text) {}

  virtual std::string GetUINodeInfo(int id) { return ""; }
  virtual std::string GetLynxUITree() { return ""; }
  virtual int SetUIStyle(int id, std::string name, std::string content) {
    return 0;
  }

  void SendPageScreencastFrameEvent(const std::string& data,
                                    std::shared_ptr<ScreenMetadata> metadata);
  void SendPageScreencastVisibilityChangedEvent(bool status);
  void SendLynxScreenshotCapturedEvent(const std::string& data);
  void SendConsoleEvent(const lynx::runtime::js::ConsoleMessage& message);
  void SendLayerTreeDidChangeEvent();
  void SendCDPEvent(const std::string& message);

  virtual std::vector<double> GetBoxModel(tasm::Element* element) {
    return std::vector<double>();
  }
  virtual std::vector<double> GetBoxModel(const InspectorBoxModelQuery& query);
  virtual std::vector<float> GetTransformValue(
      int identifier, const std::vector<float>& pad_border_margin_layout) {
    return std::vector<float>();
  }

  // Deprecated since 3.8
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
  std::string GetLepusDebugInfoUrl(const std::string& file_name);

 protected:
  // This function is shared across multiple platforms
  // and will be called in the GetBoxModel method of subclasses.
  // It is used to retrieve the box model information for an element.
  std::vector<double> GetBoxModelInGeneralPlatform(tasm::Element* element);
  // This function is shared across multiple platforms and retrieves box model
  // information from a TASM-thread snapshot plus UI-thread transform.
  std::vector<double> GetBoxModelInGeneralPlatform(
      const InspectorBoxModelQuery& query);

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
