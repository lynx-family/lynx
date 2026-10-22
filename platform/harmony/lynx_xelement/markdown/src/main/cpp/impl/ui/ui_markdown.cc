// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/impl/ui/ui_markdown.h"

#include <utility>

#include "base/include/log/logging.h"
#include "base/include/platform/harmony/harmony_vsync_manager.h"
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

UIMarkdown::UIMarkdown(LynxContext* context, int sign, const std::string& tag)
    : UIBase(context, ARKUI_NODE_CUSTOM, sign, tag) {}

void UIMarkdown::DetachMarkdownView() {
  if (!markdown_view_) {
    return;
  }
  NodeManager::Instance().RemoveNode(Node(), markdown_view_->GetHandle());
  markdown_view_.reset();
  markdown_measurer_.reset();
}

void UIMarkdown::OnDestroy() { DetachMarkdownView(); }

void UIMarkdown::UpdateExtraData(
    const fml::RefPtr<fml::RefCountedThreadSafeStorage>& extra_data) {
  UIBase::UpdateExtraData(extra_data);
  std::shared_ptr<serval::markdown::NativeMarkdownMeasurer>
      new_markdown_measurer;
  measured_size_ = {};
  if (extra_data) {
    auto* bundle = static_cast<MarkdownViewBundle*>(extra_data.get());
    new_markdown_measurer = bundle->GetMarkdownMeasurer();
    measured_size_ = bundle->GetMeasuredSize();
  }
  if (new_markdown_measurer != markdown_measurer_) {
    DetachMarkdownView();
    if (new_markdown_measurer) {
      auto view =
          std::make_unique<serval::markdown::NativeServalMarkdownView>(false);
      view->DisableInternalVSync(true);
      view->SetExternalMeasure(true);
      const bool attached = view->SetMeasurer(new_markdown_measurer.get());
      DCHECK(attached);
      markdown_measurer_ = std::move(new_markdown_measurer);
      markdown_view_ = std::move(view);
      NodeManager::Instance().InsertNode(Node(), markdown_view_->GetHandle(),
                                         0);
      RequestRendererFrame();
    }
  }
  LayoutMarkdownView();
  RequestLayout();
  Invalidate();
}

void UIMarkdown::UpdateLayout(float left, float top, float width, float height,
                              const float* paddings, const float* margins,
                              const float* sticky, float max_height,
                              uint32_t node_index) {
  UIBase::UpdateLayout(left, top, width, height, paddings, margins, sticky,
                       max_height, node_index);
  LayoutMarkdownView();
}

void UIMarkdown::LayoutMarkdownView() {
  if (!markdown_view_) {
    return;
  }
  markdown_view_->SetExternalMeasuredSize(measured_size_);
  const float density = context_->ScaledDensity();
  markdown_view_->SetWidth(measured_size_.width_ / density);
  markdown_view_->SetHeight(measured_size_.height_ / density);
  // Persist the content origin so ArkUI does not center the drawing node
  // independently of the inline views positioned by Lynx layout.
  NodeManager::Instance().SetAttributeWithNumberValue(
      markdown_view_->GetHandle(), NODE_POSITION,
      padding_left_ + GetBorderLeftWidth(), padding_top_ + GetBorderTopWidth());
}

void UIMarkdown::RequestRendererFrame() {
  if (renderer_frame_requested_) {
    return;
  }
  renderer_frame_requested_ = true;
  base::HarmonyVsyncManager::GetInstance().RequestVSync(
      [weak_self = weak_from_this(),
       runner = context_->GetUITaskRunner()](long long frame_time) {
        runner->PostTask([weak_self, frame_time] {
          auto self = std::static_pointer_cast<UIMarkdown>(weak_self.lock());
          if (!self) {
            return;
          }
          self->renderer_frame_requested_ = false;
          if (!self->markdown_view_) {
            return;
          }
          self->markdown_view_->OnRendererFrame(frame_time);
          self->RequestRendererFrame();
        });
      });
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
  } else if (method == "pauseAnimation") {
    view->PauseRenderUpdate();
    auto result = lepus::Dictionary::Create();
    result->SetValue("animationStep", view->GetRenderedAnimationStep());
    context_->RunOnLayoutThread(
        [weak_measurer = std::weak_ptr(markdown_measurer_)] {
          if (auto measurer = weak_measurer.lock()) {
            measurer->PauseAnimation();
          }
        });
    callback(LynxGetUIResult::SUCCESS, lepus::Value(result));
  } else if (method == "resumeAnimation") {
    int32_t animation_step = -1;
    if (args.IsTable()) {
      const auto& step = args.Table()->GetValue("animationStep");
      if (step.IsNumber()) {
        animation_step = static_cast<int32_t>(step.Number());
      }
    }
    context_->RunOnLayoutThread(
        [weak_measurer = std::weak_ptr(markdown_measurer_), animation_step] {
          if (auto measurer = weak_measurer.lock()) {
            measurer->ResumeAnimation(animation_step);
          }
        });
    view->ResumeRenderUpdate();
    callback(LynxGetUIResult::SUCCESS, lepus::Value());
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
