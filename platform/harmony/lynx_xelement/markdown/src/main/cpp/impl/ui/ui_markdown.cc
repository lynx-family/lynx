// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/impl/ui/ui_markdown.h"

#include <algorithm>
#include <cmath>
#include <limits>
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

int32_t ReadIndex(const lepus::Value& value) {
  if (std::isnan(value.Number())) {
    return -1;
  }
  return static_cast<int32_t>(std::clamp(
      value.Number(), static_cast<double>(std::numeric_limits<int32_t>::min()),
      static_cast<double>(std::numeric_limits<int32_t>::max())));
}

serval::markdown::Range ReadRange(const lepus::Value& value) {
  serval::markdown::Range range{0, 0};
  if (value.IsArray()) {
    auto array = value.Array();
    if (array->size() > 0 && array->get(0).IsNumber()) {
      range.start_ = ReadIndex(array->get(0));
    }
    if (array->size() > 1 && array->get(1).IsNumber()) {
      range.end_ = ReadIndex(array->get(1));
    }
  } else if (value.IsTable()) {
    auto table = value.Table();
    const auto& start = table->GetValue("start");
    const auto& end = table->GetValue("end");
    const auto& selection_start = table->GetValue("selectionStart");
    const auto& selection_end = table->GetValue("selectionEnd");
    if (start.IsNumber()) {
      range.start_ = ReadIndex(start);
    } else if (selection_start.IsNumber()) {
      range.start_ = ReadIndex(selection_start);
    }
    if (end.IsNumber()) {
      range.end_ = ReadIndex(end);
    } else if (selection_end.IsNumber()) {
      range.end_ = ReadIndex(selection_end);
    }
  }
  return range;
}

lepus::Value RectToLepusValue(const serval::markdown::RectF& rect, float left,
                              float top, float density) {
  auto result = lepus::Dictionary::Create();
  result->SetValue("left", left + rect.GetLeft() / density);
  result->SetValue("top", top + rect.GetTop() / density);
  result->SetValue("right", left + rect.GetRight() / density);
  result->SetValue("bottom", top + rect.GetBottom() / density);
  result->SetValue("width", rect.GetWidth() / density);
  result->SetValue("height", rect.GetHeight() / density);
  return lepus::Value(result);
}

serval::markdown::MarkdownSelection::CharRangeType ReadSelectionType(
    const lepus::Value& value) {
  using Type = serval::markdown::MarkdownSelection::CharRangeType;
  if (value.IsString()) {
    const auto type = value.StdString();
    if (type == "word") return Type::kWord;
    if (type == "sentence") return Type::kSentence;
    if (type == "paragraph") return Type::kParagraph;
  }
  return Type::kChar;
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
  // The custom view should be sized to the content box, not the border box.
  // Otherwise ArkUI remeasures it with a width that includes padding/border,
  // which differs from the shadow node's content constraint and causes the
  // MarkdownViewMeasurer to re-layout on every animation step.
  const float content_width =
      std::max(0.f, width - padding_left_ - padding_right_ -
                        GetBorderLeftWidth() - GetBorderRightWidth());
  const float content_height =
      std::max(0.f, height - padding_top_ - padding_bottom_ -
                        GetBorderTopWidth() - GetBorderBottomWidth());
  markdown_view_->SetWidth(content_width);
  markdown_view_->SetHeight(content_height);
  // Persist the content origin so ArkUI does not center the drawing node
  // independently of the inline views positioned by Lynx layout.
  NodeManager::Instance().SetAttributeWithNumberValue(
      markdown_view_->GetHandle(), NODE_POSITION,
      padding_left_ + GetBorderLeftWidth(), padding_top_ + GetBorderTopWidth());
}

void UIMarkdown::OnPropUpdate(const std::string& name,
                              const lepus::Value& value) {
  UIBase::OnPropUpdate(name, value);
  if (markdown_view_) {
    RequestLayout();
    Invalidate();
  }
}

lepus::Value UIMarkdown::GetTextRectsResult(
    const std::vector<serval::markdown::RectF>& boxes, const lepus::Value& args,
    bool include_handles) {
  auto result = lepus::Dictionary::Create();
  auto box_values = lepus::CArray::Create();
  auto handles = lepus::CArray::Create();
  if (!boxes.empty()) {
    float rect[4] = {0, 0, width_, height_};
    GetBoundingClientRect(args, rect);
    const float left = rect[0] + padding_left_ + GetBorderLeftWidth();
    const float top = rect[1] + padding_top_ + GetBorderTopWidth();
    const float density = context_->ScaledDensity();
    auto bounds = boxes.front();
    for (const auto& box : boxes) {
      bounds.SetLeft(std::min(bounds.GetLeft(), box.GetLeft()));
      bounds.SetTop(std::min(bounds.GetTop(), box.GetTop()));
      bounds.SetRight(std::max(bounds.GetRight(), box.GetRight()));
      bounds.SetBottom(std::max(bounds.GetBottom(), box.GetBottom()));
      box_values->emplace_back(RectToLepusValue(box, left, top, density));
    }
    result->SetValue("boundingRect",
                     RectToLepusValue(bounds, left, top, density));
    if (include_handles) {
      const auto append_handle = [&](float x, float y) {
        auto handle = lepus::Dictionary::Create();
        handle->SetValue("x", left + x / density);
        handle->SetValue("y", top + y / density);
        handle->SetValue(
            "radius",
            markdown_view_->GetMarkdownView()->GetSelectionHandleRadius() /
                density);
        handles->emplace_back(std::move(handle));
      };
      append_handle(boxes.front().GetLeft(), boxes.front().GetBottom());
      append_handle(boxes.back().GetRight(), boxes.back().GetBottom());
    }
  }
  result->SetValue("boxes", std::move(box_values));
  if (include_handles) {
    result->SetValue("handles", std::move(handles));
  }
  return lepus::Value(std::move(result));
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
    if (!args.IsTable() && !args.IsArray()) {
      callback(LynxGetUIResult::PARAM_INVALID, lepus::Value());
      return;
    }
    const auto table =
        args.IsTable() ? args.Table() : lepus::Dictionary::Create();
    auto range = ReadRange(args);
    if (table->Contains("startX") || table->Contains("startY") ||
        table->Contains("endX") || table->Contains("endY")) {
      double coordinates[4] = {};
      const char* keys[] = {"startX", "startY", "endX", "endY"};
      bool clear = false;
      for (size_t i = 0; i < 4; ++i) {
        const auto& value = table->GetValue(keys[i]);
        if (!value.IsNil() && !value.IsNumber()) {
          callback(LynxGetUIResult::PARAM_INVALID, lepus::Value());
          return;
        }
        coordinates[i] = value.IsNumber() ? value.Number() : 0;
        if (std::isnan(coordinates[i])) {
          callback(LynxGetUIResult::PARAM_INVALID, lepus::Value());
          return;
        }
        clear |= coordinates[i] < 0;
      }
      if (clear) {
        range = {-1, -1};
      } else {
        const double density = context_->ScaledDensity();
        const float content_left = padding_left_ + GetBorderLeftWidth();
        const float content_top = padding_top_ + GetBorderTopWidth();
        const auto point = [&](size_t offset) {
          return serval::markdown::PointF{
              static_cast<float>(std::clamp(
                  (coordinates[offset] - content_left) * density, 0.0,
                  static_cast<double>(std::numeric_limits<float>::max()))),
              static_cast<float>(std::clamp(
                  (coordinates[offset + 1] - content_top) * density, 0.0,
                  static_cast<double>(std::numeric_limits<float>::max())))};
        };
        const auto start = point(0);
        const auto end = point(2);
        const auto type =
            start.x_ == end.x_ && start.y_ == end.y_
                ? ReadSelectionType(table->GetValue("selectionTextType"))
                : serval::markdown::MarkdownSelection::CharRangeType::kChar;
        const auto start_range = view->GetCharRangeByPosition(start, type);
        const auto end_range = view->GetCharRangeByPosition(end, type);
        range = {std::min(start_range.start_, end_range.start_),
                 std::max(start_range.end_, end_range.end_)};
      }
    }
    view->SetTextSelection(range);
    callback(
        LynxGetUIResult::SUCCESS,
        GetTextRectsResult(view->GetSelectedLineBoundingRect(), args, true));
  } else if (method == "getTextBoundingRect") {
    if (!args.IsTable() || !args.Table()->GetValue("start").IsNumber() ||
        !args.Table()->GetValue("end").IsNumber()) {
      callback(LynxGetUIResult::PARAM_INVALID, lepus::Value());
      return;
    }
    auto range = ReadRange(args);
    if (range.start_ < 0 || range.end_ < range.start_) {
      callback(LynxGetUIResult::PARAM_INVALID, lepus::Value());
      return;
    }
    const auto& index_type = args.Table()->GetValue("indexType");
    if (index_type.IsString() && index_type.StdString() == "source") {
      range = {view->SourceOffsetToCharOffset(range.start_),
               view->SourceOffsetToCharOffset(range.end_)};
    }
    const auto boxes = view->GetTextLineBoundingRect(range);
    if (boxes.empty()) {
      callback(LynxGetUIResult::UNKNOWN, lepus::Value());
      return;
    }
    callback(LynxGetUIResult::SUCCESS, GetTextRectsResult(boxes, args, false));
  } else {
    UIBase::InvokeMethod(method, args, std::move(callback));
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
