// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/shadow_node.h"

#include <cstddef>
#include <cstring>
#include <limits>
#include <optional>

#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/shadow/inline_image_shadow_node.h"
#include "clay/ui/shadow/inline_text_shadow_node.h"
#include "clay/ui/shadow/text_shadow_node.h"
#include "clay/ui/shadow/text_update_bundle.h"
#include "clay/ui/shadow/vertical_align_style.h"

namespace clay {

ShadowNode::ShadowNode(ShadowNodeOwner* owner, std::string tag, int id)
    : tag_(tag), id_(id), owner_(owner) {}

void ShadowNode::SetAttribute(const char* attr_c, const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  if (kw == KeywordID::kVerticalAlign) {
    SetVerticalAlign(value);
  } else if (kw == KeywordID::kIdselector) {
    id_selector_ = attribute_utils::GetCString(value);
  }
}

void ShadowNode::SetVerticalAlign(const clay::Value& value) {
  vertical_align_ = std::make_optional<VerticalAlign>();
  const auto& array = attribute_utils::GetArray(value);
  if (array.size() < 2) {
    vertical_align_->type = VerticalAlignType::kVerticalAlignDefault;
    vertical_align_->length = 0;
  } else {
    vertical_align_->type =
        static_cast<VerticalAlignType>(attribute_utils::GetInt(array[0]));
    vertical_align_->length = attribute_utils::GetDouble(array[1]);
  }
  MarkDirty();
  MarkNeedsUpdate(TextUpdateFlag::kUpdateFlagStyle);
}

void ShadowNode::AddChild(ShadowNode* node) { AddChild(node, ChildCount()); }

void ShadowNode::AddChild(ShadowNode* child, int index) {
  if (static_cast<size_t>(index) > ChildCount() || index < 0) {
    return;
  }

  ShadowNode* parent = child->parent_;
  if (parent) {
    parent->RemoveChild(child);
  }

  child->parent_ = this;
  children_.insert(children_.begin() + index, child);
}

void ShadowNode::RemoveChild(ShadowNode* child) {
  std::vector<ShadowNode*>::iterator iter(
      std::find(children_.begin(), children_.end(), child));
  if (iter == children_.end()) {
    return;
  }

  child->parent_ = nullptr;
  children_.erase(iter);
}

void ShadowNode::RemoveAllChildren() {
  for (auto* child : children_) {
    child->parent_ = nullptr;
  }
  children_.clear();
}

void ShadowNode::AddEventCallback(const char* event_c) {
  std::string event(event_c);
  if (!events_) {
    events_ = std::make_optional<std::vector<std::string>>();
  }
  events_->emplace_back(event);
}

bool ShadowNode::HasInlineObject() {
  for (ShadowNode* child : children_) {
    if (child->IsInlineImageShadowNode() || child->IsInlineTextShadowNode() ||
        child->IsInlineViewShadowNode() ||
        child->IsInlineTruncationShadowNode()) {
      return true;
    }
  }
  return false;
}

void ShadowNode::MarkNeedsUpdate(TextUpdateFlag flag) {
  auto node = this;
  while (node) {
    if (node->IsTextShadowNode()) {
      auto text_node = static_cast<TextShadowNode*>(node);
      text_node->AddUpdateFlag(flag);
      break;
    }
    node = node->Parent();
  }
}

ShadowNode* ShadowNode::FindNoneVirtualNode() {
  if (!IsVirtual()) {
    return this;
  }
  ShadowNode* temp = this->Parent();
  while (temp != nullptr && temp->IsVirtual()) {
    temp = temp->Parent();
  }
  return temp;
}

void ShadowNode::MarkDirty() {
  if (!IsVirtual()) {
    // mark lynx layout node dirty
    if (!is_dirty_) {
      is_dirty_ = true;
      owner_->MarkDirty(this);
    }
    return;
  }
  ShadowNode* visible_node = FindNoneVirtualNode();
  if (visible_node != nullptr) {
    visible_node->MarkDirty();
  }
}

void ShadowNode::UpdateLayoutStylesFromLynx() {
  if (!owner_) {
    return;
  }
  ClayLayoutStyles styles = owner_->GetLayoutStyles(this);
  styles_ = styles;
}

void ShadowNode::SetBaselineOffset(const FontMetrics& parent_metrics,
                                   double child_descent, double child_ascent) {
  SetBaselineOffset(
      CalculateBaselineOffset(parent_metrics, child_descent, child_ascent));
}

double ShadowNode::CalculateBaselineOffset(const FontMetrics& parent_metrics,
                                           double child_descent,
                                           double child_ascent) {
  auto vertical_align = GetVerticalAlign();
  if (!vertical_align.has_value()) {
    return 0.f;
  }
  double baseline_shift = 0.f;
  switch (vertical_align->type) {
    case kVerticalAlignDefault:
      baseline_shift = 0.f;
      break;
    case kVerticalAlignBaseline:
      break;
    case kVerticalAlignSub:
      baseline_shift =
          -(parent_metrics.glyph_bottom - parent_metrics.glyph_top) * 0.1f;
      break;
    case kVerticalAlignSuper:
      baseline_shift =
          (parent_metrics.glyph_bottom - parent_metrics.glyph_top) * 0.1f;
      break;
    case kVerticalAlignTop:
      baseline_shift = child_ascent - parent_metrics.ascent;
      break;
    case kVerticalAlignTextTop:
      baseline_shift =
          -parent_metrics.glyph_top - parent_metrics.ascent + child_ascent;
      break;
    case kVerticalAlignMiddle:
      baseline_shift =
          (parent_metrics.x_height + child_descent + child_ascent) * 0.5;
      break;
    case kVerticalAlignBottom:
      baseline_shift = child_descent - parent_metrics.descent;
      break;
    case kVerticalAlignTextBottom:
      baseline_shift =
          -parent_metrics.ascent - parent_metrics.glyph_bottom + child_descent;
      break;
    case VerticalAlignType::kVerticalAlignLength:
      baseline_shift = -vertical_align->length;
      break;
    case kVerticalAlignPercent:
      baseline_shift =
          vertical_align->length * parent_metrics.line_height / 100.f;
      break;
    case kVerticalAlignCenter:
      baseline_shift = (parent_metrics.bottom - parent_metrics.top +
                        child_descent - child_ascent) *
                           0.5 +
                       child_ascent - parent_metrics.descent;
      break;
  }
  return baseline_shift;
}

void ShadowNode::ResetEndIndex() {
  for (auto* child : GetChildren()) {
    if (child->IsRawTextShadowNode() || child->IsInlineImageShadowNode() ||
        child->IsInlineViewShadowNode()) {
      child->SetEndIndex(std::numeric_limits<size_t>::max());
    } else {
      child->ResetEndIndex();
    }
  }
}

void ShadowNode::CreateTextInfo(txt::Paragraph* paragraph) {
  auto bundle = FindTextBundle();
  if (!bundle) {
    return;
  }
  for (auto child : children_) {
    TextInfo info;
    info.id = child->id();
    info.parent_id = this->id();
    if (child->IsInlineTextShadowNode()) {
      info.range_ =
          static_cast<InlineTextShadowNode*>(child)->range_in_paragraph_;
      info.view_style = child->styles_;
      child->CreateTextInfo(paragraph);
    }
    if (child->IsInlineImageShadowNode()) {
      auto placeholder_index =
          static_cast<InlineImageShadowNode*>(child)->placeholder_index();
      if (placeholder_index < 0) {
        info.need_mount = false;
        return;
      }
      info.placeholder_index = placeholder_index;
      info.view_style = child->styles_;
      auto node = static_cast<InlineImageShadowNode*>(child);
      auto text_box =
          paragraph->GetRectsForRange(node->StartGlyph(), node->EndGlyph(),
                                      txt::Paragraph::RectHeightStyle::kTight,
                                      txt::Paragraph::RectWidthStyle::kTight);
      if (!text_box.empty()) {
        info.need_mount = true;
        if (text_box.back().direction == txt::TextDirection::rtl) {
          info.location =
              FloatPoint(text_box.back().rect.Left(),
                         text_box.back().rect.Top() + node->MarginTop());
        } else {
          info.location =
              FloatPoint(text_box.front().rect.Left(),
                         text_box.front().rect.Top() + node->MarginTop());
        }
      } else {
        info.need_mount = false;
        info.placeholder_index = -1;
      }
    }
    if (child->IsInlineViewShadowNode()) {
      info.placeholder_index =
          static_cast<InlineViewShadowNode*>(child)->placeholder_index();
      if (info.placeholder_index.value_or(-1) >= 0) {
        info.need_mount = true;
        child->CreateTextInfo(paragraph);
      } else {
        info.need_mount = false;
      }
    }
    if (child->IsInlineTruncationShadowNode()) {
      if (static_cast<InlineTruncationShadowNode*>(child)->IfNeedMount()) {
        info.view_style = child->styles_;
        info.need_mount = true;
      } else {
        info.need_mount = false;
      }
      child->CreateTextInfo(paragraph);
    }
    bundle->PushTextInfo(info);
  }
}

TextUpdateBundle* ShadowNode::FindTextBundle() {
  auto parent = this;
  while (!parent->IsTextShadowNode()) {
    if (parent->Parent()) {
      parent = parent->Parent();
    } else {
      return nullptr;
    }
  }
  return static_cast<TextShadowNode*>(parent)->GetTextBundle();
}

}  // namespace clay
