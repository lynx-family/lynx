// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/editable_shadow_node.h"

#include <algorithm>
#include <string>

#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/common/measure_constraint.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/editable/editable_view.h"
#include "clay/ui/component/editable/textarea_ng_view.h"
#include "clay/ui/component/editable/textarea_view.h"
#include "clay/ui/component/view_context.h"
#include "clay/ui/shadow/shadow_node.h"

namespace clay {

namespace {

constexpr float kDefaultLineHeightFactor = 1.2f;

bool MeasureMountedEditableView(BaseView* view,
                                const MeasureConstraint& constraint,
                                MeasureResult& result) {
  if (!view) {
    return false;
  }
  if (view->Is<TextAreaView>()) {
    static_cast<TextAreaView*>(view)->Measure(constraint, result);
    return true;
  }
  if (view->Is<TextAreaNGView>()) {
    static_cast<TextAreaNGView*>(view)->Measure(constraint, result);
    return true;
  }
  if (view->Is<EditableView>()) {
    static_cast<EditableView*>(view)->Measure(constraint, result);
    return true;
  }
  return false;
}

}  // namespace

EditableShadowNode::EditableShadowNode(ShadowNodeOwner* owner, std::string tag,
                                       int id)
    : ShadowNode(owner, tag, id) {
  font_size_ =
      owner->GetViewContext()->GetPageView()->ConvertFrom<kPixelTypeLogical>(
          kDefaultFontSizeInDip);
}

void EditableShadowNode::SetAttribute(const char* attr_c,
                                      const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  if (kw == KeywordID::kFontSize) {
    double font_size = 0.0;
    if (attribute_utils::TryGetNum(value, font_size)) {
      font_size_ = font_size;
    }
  } else if (kw == KeywordID::kLineHeight) {
    double line_height = 0.0;
    if (attribute_utils::TryGetNum(value, line_height)) {
      line_height_ = line_height;
    }
  } else if (kw == KeywordID::kMaxlines) {
    double max_lines = std::numeric_limits<uint32_t>::max();
    if (attribute_utils::TryGetNum(value, max_lines)) {
      max_lines_ = max_lines;
    }
  } else if (kw == KeywordID::kMinHeight) {
    std::string min_height;
    attribute_utils::TryGetString(value, min_height);
    min_height_ = attribute_utils::ToPxWithDisplayMetrics(
        min_height, owner_->GetViewContext()->GetPageView());
    MarkDirty();
  } else if (kw == KeywordID::kMaxHeight) {
    std::string max_height;
    attribute_utils::TryGetString(value, max_height);
    max_height_ = attribute_utils::ToPxWithDisplayMetrics(
        max_height, owner_->GetViewContext()->GetPageView());
    MarkDirty();
  }
}

void EditableShadowNode::SetTextHeight(float text_height) {
  if (text_height_ != text_height) {
    text_height_ = text_height;
    MarkDirty();
  }
}

void EditableShadowNode::Measure(const MeasureConstraint& constraint,
                                 MeasureResult& result) {
  bool infinite = max_lines_ == std::numeric_limits<uint32_t>::max();
  auto estimated_height = [this, infinite]() {
    float line_height = line_height_.has_value()
                            ? std::max(kDefaultLineHeightFactor * font_size_,
                                       line_height_.value())
                            : kDefaultLineHeightFactor * font_size_;
    return line_height * (infinite ? 1 : max_lines_);
  };

  if (constraint.height_mode == MeasureMode::kDefinite) {
    result.height = *constraint.height;
  } else {
    // Currently the autoheight attribute does not support asynchronous
    if (min_height_ >= 0 && max_height_ >= 0 && max_height_ >= min_height_) {
      if (owner_->GetUITaskRunner()->RunsTasksOnCurrentThread()) {
        if (MeasureMountedEditableView(owner_->FindViewByViewId(id()),
                                       constraint, result)) {
          result.height = std::clamp(result.height, min_height_, max_height_);
          return;
        }
      } else {
        FML_DCHECK(false);
      }
      result.height = text_height_ > 0 ? text_height_ : estimated_height();
      result.height = std::clamp(result.height, min_height_, max_height_);
    } else {
      result.height = estimated_height();
    }
  }
  if (constraint.width_mode == MeasureMode::kIndefinite ||
      !constraint.width.has_value()) {
    result.width = std::numeric_limits<float>::infinity();
  } else {
    result.width = *constraint.width;
  }

  if (constraint.width_mode == MeasureMode::kAtMost) {
    result.width = std::min(result.width, constraint.width.value_or(0));
  }
  if (constraint.height_mode == MeasureMode::kAtMost) {
    result.height = std::min(result.height, constraint.height.value_or(0));
  }
}

}  // namespace clay
