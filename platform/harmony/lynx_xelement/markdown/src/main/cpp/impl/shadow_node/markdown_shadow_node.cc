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
#include "markdown/platform/harmony/serval_markdown_view.h"
#include "markdown/view/markdown_view.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/impl/markdown_view_bundle.h"
#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/impl/resource/markdown_resource_loader_harmony.h"
#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/impl/utils/markdown_lepus_value.h"

namespace lynx {
namespace tasm {
namespace harmony {
namespace {

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
  markdown_view_ =
      std::make_shared<serval::markdown::NativeServalMarkdownView>();
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
  if (markdown_view_ && markdown_view_->GetMarkdownView()) {
    markdown_view_->GetMarkdownView()->SetResourceLoader(nullptr);
  }
  resource_loader_.reset();
  bundle_ = nullptr;
  markdown_view_.reset();
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
