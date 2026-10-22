// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/impl/shadow_node/markdown_shadow_node.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "base/include/log/logging.h"
#include "markdown/markdown_event_listener.h"
#include "markdown/platform/harmony/harmony_markdown_measurer.h"
#include "markdown/platform/harmony/serval_markdown_view.h"
#include "markdown/view/markdown_view.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/custom_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node_owner.h"
#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/impl/markdown_view_bundle.h"
#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/impl/resource/markdown_resource_loader_harmony.h"
#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/impl/utils/markdown_lepus_value.h"

namespace lynx {
namespace tasm {
namespace harmony {
namespace {

class MarkdownEventListenerHarmony final
    : public serval::markdown::MarkdownEventListener {
 public:
  MarkdownEventListenerHarmony(std::weak_ptr<LynxContext> context, int sign)
      : context_(std::move(context)), sign_(sign) {}

  void OnParseEnd() override {}
  void OnTextOverflow(serval::markdown::MarkdownTextOverflow) override {}
  void OnDrawStart() override { SendEvent("drawStart"); }
  void OnDrawEnd() override { SendEvent("drawEnd"); }
  void OnAnimationStep(int32_t, int32_t) override {}
  void OnLinkClicked(const char*, const char*) override {}
  void OnImageClicked(const char*) override {}
  void OnSelectionChanged(int32_t start, int32_t end,
                          serval::markdown::SelectionHandleType,
                          serval::markdown::SelectionState) override {
    auto detail = lepus::Dictionary::Create();
    detail->SetValue("start", start);
    detail->SetValue("end", end);
    const bool forward = last_start_ == -1
                             ? start < end
                             : last_start_ < start || last_end_ < end;
    detail->SetValue("direction", forward ? "forward" : "backward");
    last_start_ = start;
    last_end_ = end;
    SendEvent("selectionchange", lepus::Value(std::move(detail)));
  }

 private:
  void SendEvent(const char* name, lepus::Value detail = lepus::Value()) const {
    if (auto context = context_.lock()) {
      CustomEvent event{sign_, name, "detail", std::move(detail)};
      context->RunOnUIThread([weak_context = context_, event]() {
        if (auto context = weak_context.lock()) {
          context->SendEvent(event);
        }
      });
    }
  }

  std::weak_ptr<LynxContext> context_;
  int sign_;
  int32_t last_start_{-1};
  int32_t last_end_{-1};
};

struct MarkdownNativeState {
  MarkdownNativeState(std::weak_ptr<LynxContext> context, int sign)
      : listener(std::move(context), sign) {
    view.SetMeasurer(&measurer);
    view.GetMarkdownView()->SetEnableRegionView(false);
    view.GetMarkdownView()->SetEventListener(&listener);
  }

  // The view must be destroyed before the measurer it references.
  MarkdownEventListenerHarmony listener;
  serval::markdown::NativeMarkdownMeasurer measurer;
  serval::markdown::NativeServalMarkdownView view{false};
};

tttext::LayoutMode ToMarkdownLayoutMode(MeasureMode mode) {
  switch (mode) {
    case MeasureMode::Definite:
      return tttext::LayoutMode::kDefinite;
    case MeasureMode::AtMost:
      return tttext::LayoutMode::kAtMost;
    case MeasureMode::Indefinite:
    default:
      return tttext::LayoutMode::kIndefinite;
  }
}

float ToMarkdownMeasureValue(float value, MeasureMode mode, float density) {
  if (mode == MeasureMode::Indefinite || !std::isfinite(value)) {
    return serval::markdown::MeasureSpec::LAYOUT_MAX_SIZE;
  }
  if (value <= 0.f) {
    return 0.f;
  }
  return std::min(value * density,
                  serval::markdown::MeasureSpec::LAYOUT_MAX_SIZE);
}

serval::markdown::Range ReadRange(const lepus::Value& value) {
  serval::markdown::Range range{0, std::numeric_limits<int32_t>::max()};
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
    if (start.IsNumber()) {
      range.start_ = static_cast<int32_t>(start.Number());
    }
    if (end.IsNumber()) {
      range.end_ = static_cast<int32_t>(end.Number());
    }
  }
  return range;
}

serval::markdown::MarkdownAnimationType ReadAnimationType(
    const lepus::Value& value) {
  if (!value.IsString()) {
    return serval::markdown::MarkdownAnimationType::kNone;
  }
  const auto& type = value.StdString();
  if (type == "typewriter") {
    return serval::markdown::MarkdownAnimationType::kTypewriter;
  }
  if (type == "line-expand") {
    return serval::markdown::MarkdownAnimationType::kLineExpand;
  }
  return serval::markdown::MarkdownAnimationType::kNone;
}

void InitMarkdownEnv(LynxContext* context) {
  if (!context) {
    return;
  }
  static std::once_flag once_flag;
  std::call_once(once_flag, [context] {
    serval::markdown::NativeServalMarkdownView::InitEnv(context->GetNapiEnv());
  });
}

}  // namespace

MarkdownShadowNode::MarkdownShadowNode(int sign, const std::string& tag)
    : ShadowNode(sign, tag) {
  SetCustomMeasureFunc(this);
}

MarkdownShadowNode::~MarkdownShadowNode() = default;

void MarkdownShadowNode::OnContextReady() { EnsureMarkdownView(); }

std::shared_ptr<serval::markdown::NativeServalMarkdownView>
MarkdownShadowNode::EnsureMarkdownView() {
  if (markdown_view_ || !context_) {
    return markdown_view_;
  }
  InitMarkdownEnv(context_);
  auto state = std::make_shared<MarkdownNativeState>(owner_->GetWeakContext(),
                                                     Signature());
  // Share ownership so a view retained by the UI keeps its measurer alive.
  markdown_measurer_ =
      std::shared_ptr<serval::markdown::NativeMarkdownMeasurer>(
          state, &state->measurer);
  markdown_view_ = std::shared_ptr<serval::markdown::NativeServalMarkdownView>(
      state, &state->view);
  markdown_measurer_->SetRequestMeasureCallback([this]() { MarkDirty(); });
  resource_loader_ = std::make_shared<MarkdownResourceLoaderHarmony>(
      context_, this, markdown_view_.get());
  markdown_view_->GetMarkdownView()->SetResourceLoader(resource_loader_.get());
  bundle_ = MarkdownViewBundle::Create(markdown_view_);
  return markdown_view_;
}

serval::markdown::MarkdownView* MarkdownShadowNode::MarkdownView() {
  auto markdown_view = EnsureMarkdownView();
  return markdown_view ? markdown_view->GetMarkdownView() : nullptr;
}

fml::RefPtr<fml::RefCountedThreadSafeStorage>
MarkdownShadowNode::getExtraBundle() {
  EnsureMarkdownView();
  return bundle_;
}

void MarkdownShadowNode::Destroy() {
  if (markdown_measurer_) {
    markdown_measurer_->SetRequestMeasureCallback(nullptr);
  }
  if (markdown_view_ && markdown_view_->GetMarkdownView()) {
    markdown_view_->GetMarkdownView()->SetResourceLoader(nullptr);
  }
  resource_loader_.reset();
  bundle_ = nullptr;
  markdown_view_.reset();
  markdown_measurer_.reset();
}

void MarkdownShadowNode::OnPropsUpdate(const std::string& name,
                                       const lepus::Value& value) {
  auto markdown_view = EnsureMarkdownView();
  if (!markdown_view) {
    return;
  }
  auto* view = markdown_view->GetMarkdownView();
  if (name == "content") {
    if (value.IsString()) {
      markdown_view->SetContent(value.StdString());
    } else {
      LOGE("markdown: content prop is not string");
    }
  } else if (name == "markdown-style") {
    markdown_view->SetStyle(LepusValueToMarkdownValueMap(value));
  } else {
    ApplyConfig(name, value);
  }
  view->NeedsMeasure();
  MarkDirty();
}

void MarkdownShadowNode::ApplyConfig(const std::string& name,
                                     const lepus::Value& value) {
  auto markdown_view = EnsureMarkdownView();
  if (!markdown_view) {
    return;
  }
  auto* view = markdown_view->GetMarkdownView();
  if (name == "markdown-config") {
    markdown_view->SetConfig(LepusValueToMarkdownValueMap(value));
  } else if (name == "enable-gif") {
    resource_loader_->SetEnableGif(value.IsBool() && value.Bool());
  } else if (name == "text-mark-attachments") {
    view->SetTextAttachments(LepusValueToMarkdownValue(value));
  } else if (name == "markdown-effect") {
    auto effect = LepusValueToMarkdownValueMap(value);
    view->SetMapProp(serval::markdown::MarkdownProps::kMarkdownEffect, effect);
  } else if (name == "content-complete") {
    if (value.IsBool()) {
      view->SetContentComplete(value.Bool());
    }
  } else if (name == "content-range") {
    view->SetContentRange(ReadRange(value));
  } else if (name == "text-maxline" || name == "text-max-lines") {
    if (value.IsNumber()) {
      view->SetTextMaxLines(static_cast<int32_t>(value.Number()));
    }
  } else if (name == "animation-type") {
    view->SetAnimationType(ReadAnimationType(value));
  } else if (name == "animation-velocity") {
    if (value.IsNumber()) {
      view->SetAnimationVelocity(static_cast<float>(value.Number()));
    }
  } else if (name == "initial-animation-step") {
    if (value.IsNumber()) {
      view->SetInitialAnimationStep(static_cast<int32_t>(value.Number()));
    }
  } else if (name == "animation-step") {
    if (value.IsNumber()) {
      view->SetAnimationStep(static_cast<int32_t>(value.Number()));
    }
  } else if (name == "text-selection" || name == "enable-selection") {
    if (value.IsBool()) {
      view->SetEnableSelection(value.Bool());
    }
  } else if (name == "allow-break-around-punctuation" ||
             name == "enable-break-around-punctuation") {
    if (value.IsBool()) {
      view->SetEnableBreakAroundPunctuation(value.Bool());
    }
  } else if (name == "trim-last-paragraph-space") {
    if (value.IsBool()) {
      view->SetTrimParagraphSpaces(value.Bool());
    }
  } else if (name == "parser-type") {
    if (value.IsString()) {
      view->SetParserType(value.StdString());
    }
  } else if (name == "source-type") {
    if (value.IsString() && value.StdString() == "plain-text") {
      view->SetSourceType(serval::markdown::SourceType::kPlainText);
    } else {
      view->SetSourceType(serval::markdown::SourceType::kMarkdown);
    }
  }
}

LayoutResult MarkdownShadowNode::Measure(float width, MeasureMode width_mode,
                                         float height, MeasureMode height_mode,
                                         bool final_measure) {
  (void)final_measure;
  auto* view = MarkdownView();
  if (!view) {
    return {0, 0};
  }
  const float density = context_ ? context_->ScaledDensity() : 1.f;
  if (resource_loader_) {
    resource_loader_->SetMeasureContext(width, width_mode, height, height_mode,
                                        final_measure, density);
  }
  serval::markdown::MeasureSpec spec;
  spec.width_ = ToMarkdownMeasureValue(width, width_mode, density);
  spec.width_mode_ = ToMarkdownLayoutMode(width_mode);
  spec.height_ = ToMarkdownMeasureValue(height, height_mode, density);
  spec.height_mode_ = ToMarkdownLayoutMode(height_mode);
  auto size = view->Measure(spec);
  return {size.width_ / density, size.height_ / density};
}

void MarkdownShadowNode::Align() {
  auto* view = MarkdownView();
  if (!view) {
    return;
  }
  view->Align(0, 0);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
