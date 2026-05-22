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

InspectorLayoutObjectInfo BuildLayoutObjectInfo(int32_t id,
                                                SLNode* layout_obj) {
  InspectorLayoutObjectInfo info;
  info.id = id;
  if (layout_obj == nullptr) {
    return info;
  }
  info.has_snapshot = true;
  info.border_bound_width = layout_obj->GetBorderBoundWidth();
  info.border_bound_height = layout_obj->GetBorderBoundHeight();
  info.layout_padding_left = layout_obj->GetLayoutPaddingLeft();
  info.layout_padding_top = layout_obj->GetLayoutPaddingTop();
  info.layout_padding_right = layout_obj->GetLayoutPaddingRight();
  info.layout_padding_bottom = layout_obj->GetLayoutPaddingBottom();
  info.layout_border_left_width = layout_obj->GetLayoutBorderLeftWidth();
  info.layout_border_top_width = layout_obj->GetLayoutBorderTopWidth();
  info.layout_border_right_width = layout_obj->GetLayoutBorderRightWidth();
  info.layout_border_bottom_width = layout_obj->GetLayoutBorderBottomWidth();
  info.layout_margin_left = layout_obj->GetLayoutMarginLeft();
  info.layout_margin_top = layout_obj->GetLayoutMarginTop();
  info.layout_margin_right = layout_obj->GetLayoutMarginRight();
  info.layout_margin_bottom = layout_obj->GetLayoutMarginBottom();
  info.border_bound_left_from_parent_padding_bound =
      layout_obj->GetBorderBoundLeftFromParentPaddingBound();
  info.border_bound_top_from_parent_padding_bound =
      layout_obj->GetBorderBoundTopFromParentPaddingBound();
  return info;
}

namespace {

bool ResolveLayoutObjectInfo(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator,
    const InspectorLayoutObjectInfo& source,
    InspectorLayoutObjectInfo& target) {
  if (source.has_snapshot) {
    target = source;
    return true;
  }
  auto* layout_obj = devtool_mediator->GetLayoutObjectById(source.id);
  if (layout_obj == nullptr) {
    return false;
  }
  target = BuildLayoutObjectInfo(source.id, layout_obj);
  return true;
}

}  // namespace

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
    const lynx::runtime::js::ConsoleMessage& message) {
  auto devtool_mediator_ = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator_, "devtool_mediator_ is null");
  devtool_mediator_->SendLogEntryAddedEvent(message);
}

void DevToolPlatformFacade::SendLayerTreeDidChangeEvent() {
  auto devtool_mediator_ = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator_, "devtool_mediator_ is null");
  devtool_mediator_->SendLayerTreeDidChangeEvent();
}

void DevToolPlatformFacade::SendCDPEvent(const std::string& message) {
  auto devtool_mediator_ = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator_, "devtool_mediator_ is null");
  devtool_mediator_->SendCDPEvent(message);
}

std::vector<double> DevToolPlatformFacade::GetBoxModel(
    const InspectorBoxModelQuery& query) {
  if (query.is_overlay) {
    if (query.overlay_box_model.size() == 34) {
      return query.overlay_box_model;
    }
  }
  return GetBoxModelInGeneralPlatform(query);
}

std::vector<double> DevToolPlatformFacade::GetBoxModelInGeneralPlatform(
    tasm::Element* element) {
  std::vector<double> res;

  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", res);

  auto devtool_mediator_ = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN_VALUE(devtool_mediator_,
                                  "devtool_mediator_ is null", res);

  auto layout_obj = devtool_mediator_->GetLayoutObjectForElement(element);
  if (element->is_virtual() ||
      (static_cast<lynx::tasm::FiberElement*>(element)->is_wrapper())) {
    auto temp_parent = element->parent();
    while (
        temp_parent &&
        (temp_parent->is_virtual() ||
         (static_cast<lynx::tasm::FiberElement*>(temp_parent)->is_wrapper()))) {
      temp_parent = temp_parent->parent();
    }
    if (temp_parent) {
      res = GetBoxModel(temp_parent);
    }
  } else if (layout_obj != nullptr) {
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
    if (!element->HasUIPrimitive()) {
      auto current = element;
      float layout_only_x = 0;
      float layout_only_y = 0;
      while (current != nullptr && !current->HasUIPrimitive()) {
        auto current_layout_obj =
            devtool_mediator_->GetLayoutObjectForElement(current);
        if (current_layout_obj != nullptr) {
          layout_only_x +=
              current_layout_obj->GetBorderBoundLeftFromParentPaddingBound();
          layout_only_y +=
              current_layout_obj->GetBorderBoundTopFromParentPaddingBound();
        }
        do {
          current = current->parent();
        } while (current != nullptr &&
                 static_cast<lynx::tasm::FiberElement*>(current)->is_wrapper());
      }
      if (current != nullptr) {
        auto current_layout_obj =
            devtool_mediator_->GetLayoutObjectForElement(current);
        if (current_layout_obj != nullptr) {
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

std::vector<double> DevToolPlatformFacade::GetBoxModelInGeneralPlatform(
    const InspectorBoxModelQuery& query) {
  std::vector<double> res;

  auto devtool_mediator_ = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN_VALUE(devtool_mediator_,
                                  "devtool_mediator_ is null", res);

  InspectorLayoutObjectInfo layout_obj;
  if (!ResolveLayoutObjectInfo(devtool_mediator_, query.layout_object,
                               layout_obj)) {
    return res;
  }

  res.push_back(layout_obj.border_bound_width - layout_obj.layout_padding_left -
                layout_obj.layout_padding_right -
                layout_obj.layout_border_left_width -
                layout_obj.layout_border_right_width);
  res.push_back(layout_obj.border_bound_height - layout_obj.layout_padding_top -
                layout_obj.layout_padding_bottom -
                layout_obj.layout_border_top_width -
                layout_obj.layout_border_bottom_width);

  std::vector<float> pad_border_margin_layout = {
      static_cast<float>(layout_obj.layout_padding_left),
      static_cast<float>(layout_obj.layout_padding_top),
      static_cast<float>(layout_obj.layout_padding_right),
      static_cast<float>(layout_obj.layout_padding_bottom),
      static_cast<float>(layout_obj.layout_border_left_width),
      static_cast<float>(layout_obj.layout_border_top_width),
      static_cast<float>(layout_obj.layout_border_right_width),
      static_cast<float>(layout_obj.layout_border_bottom_width),
      static_cast<float>(layout_obj.layout_margin_left),
      static_cast<float>(layout_obj.layout_margin_top),
      static_cast<float>(layout_obj.layout_margin_right),
      static_cast<float>(layout_obj.layout_margin_bottom),
      0,
      0,
      0,
      0};
  std::vector<float> trans;
  if (!query.has_ui_primitive) {
    float layout_only_x = 0;
    float layout_only_y = 0;
    // Layout-only nodes do not have their own UI primitive, so their offsets
    // need to be accumulated before applying the transform of the nearest UI
    // node.
    for (const auto& info : query.layout_only_nodes) {
      InspectorLayoutObjectInfo current_layout_obj;
      if (ResolveLayoutObjectInfo(devtool_mediator_, info,
                                  current_layout_obj)) {
        layout_only_x += static_cast<float>(
            current_layout_obj.border_bound_left_from_parent_padding_bound);
        layout_only_y += static_cast<float>(
            current_layout_obj.border_bound_top_from_parent_padding_bound);
      }
    }
    InspectorLayoutObjectInfo transform_layout_obj;
    if (ResolveLayoutObjectInfo(devtool_mediator_, query.transform_node,
                                transform_layout_obj)) {
      layout_only_x +=
          static_cast<float>(transform_layout_obj.layout_border_left_width);
      layout_only_y +=
          static_cast<float>(transform_layout_obj.layout_border_top_width);
      pad_border_margin_layout[12] = layout_only_x;
      pad_border_margin_layout[13] = layout_only_y;
      pad_border_margin_layout[14] =
          static_cast<float>(transform_layout_obj.border_bound_width) -
          layout_only_x - static_cast<float>(layout_obj.border_bound_width);
      pad_border_margin_layout[15] =
          static_cast<float>(transform_layout_obj.border_bound_height) -
          layout_only_y - static_cast<float>(layout_obj.border_bound_height);
      trans =
          GetTransformValue(query.transform_node.id, pad_border_margin_layout);
    }
  } else {
    trans =
        GetTransformValue(query.transform_node.id, pad_border_margin_layout);
  }
  for (float t : trans) {
    res.push_back(t);
  }
  return res;
}

std::string DevToolPlatformFacade::GetLepusDebugInfoUrl(
    const std::string& file_name) {
  auto debugger = lepus_debugger_wp_.lock();
  if (debugger != nullptr) {
    return debugger->GetDebugInfoUrl(file_name);
  }
  return "";
}

}  // namespace devtool
}  // namespace lynx
