// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/devtool_platform_facade.h"

#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/lynx_devtool/js_debug/js/inspector_java_script_debugger_impl.h"
#include "devtool/lynx_devtool/js_debug/lepus/inspector_lepus_debugger_impl.h"

namespace lynx {
namespace devtool {

void DevToolPlatformFacade::InitWithDevToolMediator(
    std::shared_ptr<LynxDevToolMediator> devtool_mediator) {
  devtool_mediator_wp_ = devtool_mediator;
  inspector_ui_executor_wp_ = devtool_mediator->GetUIExecutor();
  js_debugger_wp_ = devtool_mediator->GetJSDebugger();
  lepus_debugger_wp_ = devtool_mediator->GetLepusDebugger();
}

DevToolPlatformFacade::~DevToolPlatformFacade() {
  LOGI("~DevToolPlatformFacade this: " << this);
}

void DevToolPlatformFacade::SendPageScreencastFrameEvent(
    const std::string& data, std::shared_ptr<ScreenMetadata> metadata) {
  auto ui_executor = inspector_ui_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(ui_executor, "ui_executor is null");
  ui_executor->SendPageScreencastFrameEvent(data, metadata);
}

void DevToolPlatformFacade::SendPageScreencastVisibilityChangedEvent(
    bool status) {
  auto ui_executor = inspector_ui_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(ui_executor, "ui_executor is null");
  ui_executor->SendPageScreencastVisibilityChangedEvent(status);
}

void DevToolPlatformFacade::SendLynxScreenshotCapturedEvent(
    const std::string& data) {
  auto ui_executor = inspector_ui_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(ui_executor, "ui_executor is null");
  ui_executor->SendLynxScreenshotCapturedEvent(data);
}

void DevToolPlatformFacade::SendConsoleEvent(
    const lynx::piper::ConsoleMessage& message) {
  auto devtool_mediator_ = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator_, "devtool_mediator_ is null");
  devtool_mediator_->SendLogEntryAddedEvent(message);
}

void DevToolPlatformFacade::SendLayerTreeDidChangeEvent() {
  auto devtool_mediator_ = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator_, "devtool_mediator_ is null");
  devtool_mediator_->LayerTreeDidChange();
}

std::vector<double> DevToolPlatformFacade::GetBoxModelInGeneralPlatform(
    tasm::Element* element) {
  std::vector<double> res;

  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", res);

  auto devtool_mediator_ = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN_VALUE(devtool_mediator_,
                                  "devtool_mediator_ is null", res);

  auto layout_node = devtool_mediator_->GetLayoutNodeForElement(element);
  if (element->is_virtual() ||
      (element->is_fiber_element() &&
       static_cast<lynx::tasm::FiberElement*>(element)->is_wrapper())) {
    auto temp_parent = element->parent();
    while (
        temp_parent &&
        (temp_parent->is_virtual() ||
         (temp_parent->is_fiber_element() &&
          static_cast<lynx::tasm::FiberElement*>(temp_parent)->is_wrapper()))) {
      temp_parent = temp_parent->parent();
    }
    if (temp_parent) {
      res = GetBoxModel(temp_parent);
    }
  } else if (layout_node != nullptr && layout_node->slnode() != nullptr) {
    auto layout_obj = layout_node->slnode();
    res.push_back(layout_obj->GetBorderBoundWidth() -
                  layout_obj->GetLayoutPaddingLeft() -
                  layout_obj->GetLayoutPaddingRight() -
                  layout_obj->GetLayoutBorderLeftWidth() -
                  layout_obj->GetLayoutBorderRightWidth());
    res.push_back(layout_obj->GetBorderBoundHeight() -
                  layout_obj->GetLayoutPaddingTop() -
                  layout_obj->GetLayoutPaddingBottom() -
                  layout_obj->GetLayoutBorderTopWidth() -
                  layout_obj->GetLayoutBorderBottomWidth());

    std::vector<float> pad_border_margin_layout = {
        layout_obj->GetLayoutPaddingLeft(),
        layout_obj->GetLayoutPaddingTop(),
        layout_obj->GetLayoutPaddingRight(),
        layout_obj->GetLayoutPaddingBottom(),
        layout_obj->GetLayoutBorderLeftWidth(),
        layout_obj->GetLayoutBorderTopWidth(),
        layout_obj->GetLayoutBorderRightWidth(),
        layout_obj->GetLayoutBorderBottomWidth(),
        layout_obj->GetLayoutMarginLeft(),
        layout_obj->GetLayoutMarginTop(),
        layout_obj->GetLayoutMarginRight(),
        layout_obj->GetLayoutMarginBottom(),
        0,
        0,
        0,
        0};
    std::vector<float> trans;
    if (element->CanBeLayoutOnly()) {
      auto current = element;
      float layout_only_x = 0;
      float layout_only_y = 0;
      while (current != nullptr && current->CanBeLayoutOnly()) {
        auto current_layout_node =
            devtool_mediator_->GetLayoutNodeForElement(current);
        if (current_layout_node != nullptr &&
            current_layout_node->slnode() != nullptr) {
          auto current_layout_obj = current_layout_node->slnode();
          layout_only_x +=
              current_layout_obj->GetBorderBoundLeftFromParentPaddingBound();
          layout_only_y +=
              current_layout_obj->GetBorderBoundTopFromParentPaddingBound();
        }
        do {
          current = current->parent();
        } while (current != nullptr && current->is_fiber_element() &&
                 static_cast<lynx::tasm::FiberElement*>(current)->is_wrapper());
      }
      if (current != nullptr) {
        auto current_layout_node =
            devtool_mediator_->GetLayoutNodeForElement(current);
        if (current_layout_node != nullptr &&
            current_layout_node->slnode() != nullptr) {
          auto current_layout_obj = current_layout_node->slnode();
          layout_only_x += current_layout_obj->GetLayoutBorderLeftWidth();
          layout_only_y += current_layout_obj->GetLayoutBorderTopWidth();
          pad_border_margin_layout[12] = layout_only_x;
          pad_border_margin_layout[13] = layout_only_y;
          pad_border_margin_layout[14] =
              current_layout_obj->GetBorderBoundWidth() - layout_only_x -
              layout_obj->GetBorderBoundWidth();
          pad_border_margin_layout[15] =
              current_layout_obj->GetBorderBoundHeight() - layout_only_y -
              layout_obj->GetBorderBoundHeight();
        }
        trans = GetTransformValue(current->impl_id(), pad_border_margin_layout);
      }
    } else {
      trans = GetTransformValue(element->impl_id(), pad_border_margin_layout);
    }
    for (float t : trans) {
      res.push_back(t);
    }
    return res;
  }
  return res;
}

void DevToolPlatformFacade::SendPageFrameNavigatedEvent(
    const std::string& url) {
  auto ui_executor = inspector_ui_executor_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(ui_executor, "ui_executor is null");
  ui_executor->SendPageFrameNavigatedEvent(url);
}

}  // namespace devtool
}  // namespace lynx
