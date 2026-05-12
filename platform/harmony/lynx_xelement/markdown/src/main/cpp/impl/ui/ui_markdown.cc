// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/impl/ui/ui_markdown.h"

#include <mutex>
#include <utility>

#include "base/include/value/table.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "markdown/view/markdown_view.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/impl/markdown_view_bundle.h"

namespace lynx {
namespace tasm {
namespace harmony {
namespace {

serval::markdown::Range ReadRange(const lepus::Value& value) {
  serval::markdown::Range range{0, 0};
  if (value.IsArray()) {
    auto array = value.Array();
    if (array->size() > 0 && array->get(0).IsNumber()) {
      range.start_ = static_cast<int32_t>(array->get(0).Number());
    }
    if (array->size() > 1 && array->get(1).IsNumber()) {
      range.end_ = static_cast<int32_t>(array->get(1).Number());
    }
  } else if (value.IsTable()) {
    auto table = value.Table();
    const auto& start = table->GetValue("start");
    const auto& end = table->GetValue("end");
    const auto& selection_start = table->GetValue("selectionStart");
    const auto& selection_end = table->GetValue("selectionEnd");
    if (start.IsNumber()) {
      range.start_ = static_cast<int32_t>(start.Number());
    } else if (selection_start.IsNumber()) {
      range.start_ = static_cast<int32_t>(selection_start.Number());
    }
    if (end.IsNumber()) {
      range.end_ = static_cast<int32_t>(end.Number());
    } else if (selection_end.IsNumber()) {
      range.end_ = static_cast<int32_t>(selection_end.Number());
    }
  }
  return range;
}

lepus::Value RectToLepusValue(const serval::markdown::RectF& rect) {
  auto result = lepus::Dictionary::Create();
  result->SetValue("left", rect.GetLeft());
  result->SetValue("top", rect.GetTop());
  result->SetValue("right", rect.GetRight());
  result->SetValue("bottom", rect.GetBottom());
  result->SetValue("width", rect.GetWidth());
  result->SetValue("height", rect.GetHeight());
  return lepus::Value(result);
}

}  // namespace

void UIMarkdown::InitMarkdownEnv(LynxContext* context) {
  if (!context) {
    return;
  }
  static std::once_flag once_flag;
  std::call_once(once_flag, [context] {
    serval::markdown::NativeServalMarkdownView::InitEnv(context->GetNapiEnv());
  });
}

UIMarkdown::UIMarkdown(LynxContext* context, int sign, const std::string& tag)
    : UIBase(context, ARKUI_NODE_CUSTOM, sign, tag) {}

void UIMarkdown::DetachMarkdownView() {
  if (!markdown_view_) {
    return;
  }
  NodeManager::Instance().RemoveNode(Node(), markdown_view_->GetHandle());
}

void UIMarkdown::OnDestroy() {
  DetachMarkdownView();
  markdown_view_.reset();
}

void UIMarkdown::UpdateExtraData(
    const fml::RefPtr<fml::RefCountedThreadSafeStorage>& extra_data) {
  UIBase::UpdateExtraData(extra_data);
  std::shared_ptr<serval::markdown::NativeServalMarkdownView> new_markdown_view;
  if (extra_data) {
    auto* bundle = static_cast<MarkdownViewBundle*>(extra_data.get());
    new_markdown_view = bundle->GetMarkdownView();
  }
  if (new_markdown_view == markdown_view_) {
    return;
  }
  DetachMarkdownView();
  markdown_view_ = std::move(new_markdown_view);
  if (!markdown_view_) {
    return;
  }
  NodeManager::Instance().InsertNode(Node(), markdown_view_->GetHandle(), 0);
  RequestLayout();
  Invalidate();
}

void UIMarkdown::UpdateLayout(float left, float top, float width, float height,
                              const float* paddings, const float* margins,
                              const float* sticky, float max_height,
                              uint32_t node_index) {
  UIBase::UpdateLayout(left, top, width, height, paddings, margins, sticky,
                       max_height, node_index);
  if (!markdown_view_) {
    return;
  }
  markdown_view_->SetWidth(width);
  markdown_view_->SetHeight(height);
  markdown_view_->Layout(0, 0);
}

void UIMarkdown::OnPropUpdate(const std::string& name,
                              const lepus::Value& value) {
  UIBase::OnPropUpdate(name, value);
  if (markdown_view_) {
    RequestLayout();
    Invalidate();
  }
}

void UIMarkdown::InvokeMethod(
    const std::string& method, const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (!markdown_view_) {
    callback(LynxGetUIResult::NODE_NOT_FOUND, lepus::Value());
    return;
  }
  if (method == "getParseResult") {
    UIBase::InvokeMethod(method, args, std::move(callback));
    return;
  }
  auto* view = markdown_view_->GetMarkdownView();
  if (method == "getContent") {
    auto result = lepus::Dictionary::Create();
    result->SetValue("content", view->GetContent());
    callback(LynxGetUIResult::SUCCESS, lepus::Value(result));
  } else if (method == "getSelectedText") {
    auto result = lepus::Dictionary::Create();
    result->SetValue("selectedText", view->GetSelectedText());
    callback(LynxGetUIResult::SUCCESS, lepus::Value(result));
  } else if (method == "setTextSelection") {
    view->SetTextSelection(ReadRange(args));
    callback(LynxGetUIResult::SUCCESS, lepus::Value());
  } else if (method == "getTextBoundingRect") {
    callback(LynxGetUIResult::SUCCESS,
             RectToLepusValue(view->GetTextBoundingRect(ReadRange(args))));
  } else {
    UIBase::InvokeMethod(method, args, std::move(callback));
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
