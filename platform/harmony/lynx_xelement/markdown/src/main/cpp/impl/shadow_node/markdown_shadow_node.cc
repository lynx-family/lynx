// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_xelement/markdown/src/main/cpp/impl/shadow_node/markdown_shadow_node.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <string>

#include "base/include/log/logging.h"
#include "base/include/platform/harmony/harmony_vsync_manager.h"
#include "markdown/platform/harmony/harmony_markdown_measurer.h"
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

}  // namespace

MarkdownShadowNode::MarkdownShadowNode(int sign, const std::string& tag)
    : ShadowNode(sign, tag) {
  SetCustomMeasureFunc(this);
}

MarkdownShadowNode::~MarkdownShadowNode() = default;

void MarkdownShadowNode::OnContextReady() {
  EnsureMarkdownMeasurer();
  RequestLayoutFrame();
}

void MarkdownShadowNode::RequestLayoutFrame() {
  base::HarmonyVsyncManager::GetInstance().RequestVSync(
      [self = fml::RefPtr<MarkdownShadowNode>(this),
       runner = context_->GetLayoutTaskRunner()](long long frame_time) {
        runner->PostTask([self, frame_time] {
          if (self->IsDestroyed()) {
            return;
          }
          self->markdown_measurer_->OnLayoutFrame(frame_time);
          self->RequestLayoutFrame();
        });
      });
}

std::shared_ptr<serval::markdown::NativeMarkdownMeasurer>
MarkdownShadowNode::EnsureMarkdownMeasurer() {
  if (markdown_measurer_ || !context_) {
    return markdown_measurer_;
  }
  markdown_measurer_ =
      std::make_shared<serval::markdown::NativeMarkdownMeasurer>();
  markdown_measurer_->GetMarkdownView()->SetEnableRegionView(false);
  markdown_measurer_->SetNumberProp(
      serval::markdown::MarkdownProps::kAnimationFrameRate, 15);
  resource_loader_ = std::make_shared<MarkdownResourceLoaderHarmony>(
      context_, this, markdown_measurer_);
  markdown_measurer_->GetMarkdownView()->SetResourceLoader(
      resource_loader_.get());
  markdown_measurer_->SetRequestMeasureCallback(
      [weak_loader =
           std::weak_ptr<MarkdownResourceLoaderHarmony>(resource_loader_)] {
        if (auto loader = weak_loader.lock()) {
          loader->RequestMeasure();
        }
      });
  return markdown_measurer_;
}

fml::RefPtr<fml::RefCountedThreadSafeStorage>
MarkdownShadowNode::getExtraBundle() {
  return MarkdownViewBundle::Create(EnsureMarkdownMeasurer(), measured_size_);
}

void MarkdownShadowNode::Destroy() {
  if (markdown_measurer_) {
    markdown_measurer_->SetRequestMeasureCallback(nullptr);
    markdown_measurer_->GetMarkdownView()->SetResourceLoader(nullptr);
  }
  if (resource_loader_) {
    resource_loader_->Destroy();
  }
  resource_loader_.reset();
  markdown_measurer_.reset();
}

void MarkdownShadowNode::OnPropsUpdate(const std::string& name,
                                       const lepus::Value& value) {
  auto markdown_measurer = EnsureMarkdownMeasurer();
  if (!markdown_measurer) {
    return;
  }
  if (name == "content") {
    if (value.IsString()) {
      markdown_measurer->SetContent(value.StdString());
    } else {
      LOGE("markdown: content prop is not string");
    }
  } else if (name == "markdown-style") {
    markdown_measurer->SetStyle(LepusValueToMarkdownValueMap(value));
  } else {
    ApplyConfig(name, value);
  }
  markdown_measurer->MarkDirty();
}

void MarkdownShadowNode::ApplyConfig(const std::string& name,
                                     const lepus::Value& value) {
  auto markdown_measurer = EnsureMarkdownMeasurer();
  if (!markdown_measurer) {
    return;
  }
  auto* view = markdown_measurer->GetMarkdownView();
  if (name == "markdown-config") {
    markdown_measurer->SetConfig(LepusValueToMarkdownValueMap(value));
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
  } else if (name == "animation-frame-rate") {
    if (value.IsNumber()) {
      markdown_measurer->SetNumberProp(
          serval::markdown::MarkdownProps::kAnimationFrameRate, value.Number());
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
  auto markdown_measurer = EnsureMarkdownMeasurer();
  if (!markdown_measurer) {
    measured_size_ = {};
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
  measured_size_ = markdown_measurer->Measure(spec);
  return {measured_size_.width_ / density, measured_size_.height_ / density,
          measured_size_.baseline_ / density};
}

void MarkdownShadowNode::Align() {
  if (auto markdown_measurer = EnsureMarkdownMeasurer()) {
    markdown_measurer->Align(0, 0);
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
