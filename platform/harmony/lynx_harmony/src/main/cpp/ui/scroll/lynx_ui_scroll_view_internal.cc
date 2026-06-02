// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/scroll/lynx_ui_scroll_view_internal.h"

#include <algorithm>
#include <utility>

#include "core/renderer/dom/lynx_get_ui_result.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_scroll.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_ui_screenshot_helper.h"

namespace lynx {
namespace tasm {
namespace harmony {

LynxUIScrollViewInternal::LynxUIScrollViewInternal(LynxContext* context,
                                                   int sign,
                                                   const std::string& tag)
    : UIView(context, ARKUI_NODE_CUSTOM, sign, tag) {
  scroll_view_ = new LynxBaseScrollView(this);
  NodeManager::Instance().InsertNode(node_, scroll_view_->node_, 0);
  first_render_block_array_ = new std::vector<
      base::MoveOnlyClosure<void, LynxUIScrollViewInternal*>*>();
  auto_scroller_ = new LynxBaseScrollViewAutoScroller(scroll_view_);
}

LynxUIScrollViewInternal::~LynxUIScrollViewInternal() {
  delete scroll_view_;
  delete auto_scroller_;
  if (first_render_block_array_ != nullptr) {
    delete first_render_block_array_;
  }
}

void LynxUIScrollViewInternal::GetPositionOf(float result[2], UIBase* target,
                                             LynxUIScrollViewInternal* scroll,
                                             bool is_rtl) {
  result[0] = 0;
  result[1] = 0;
  if (scroll != nullptr && target != nullptr) {
    if (is_rtl) {
      float scroll_range[] = {0, 0, 0, 0};
      scroll->scroll_view_->GetScrollRange(scroll_range);
      result[0] = scroll_range[1] - target->left_ - target->width_;
      result[1] = target->top_;
    } else {
      result[0] = target->left_;
      result[1] = target->top_;
    }
  }
}

void LynxUIScrollViewInternal::AddOffset(float offset[2], float delta[2],
                                         bool is_rtl) {
  offset[0] += is_rtl ? -delta[0] : delta[0];
  offset[1] += delta[1];
}

void LynxUIScrollViewInternal::FormatOffset(float offset[2],
                                            LynxUIScrollViewInternal* scroll,
                                            bool isRTL) {
  if (isRTL) {
    float scroll_range[] = {0, 0, 0, 0};
    scroll->scroll_view_->GetScrollRange(scroll_range);
    offset[0] = scroll_range[1] - offset[0];
  }
}

void LynxUIScrollViewInternal::OnScrollStateChanged(
    LynxBaseScrollViewScrollState from, LynxBaseScrollViewScrollState to) {
  switch (to) {
    case LynxBaseScrollViewScrollState::LynxBaseScrollViewScrollStateIdle:
      SendScrollEvent("scrollend", lepus_value());
      break;
    case LynxBaseScrollViewScrollState::LynxBaseScrollViewScrollStateDragging:
      SendScrollEvent("scrollstart", lepus_value());
      break;
    case LynxBaseScrollViewScrollState::LynxBaseScrollViewScrollStateAnimating:
      if (from ==
          LynxBaseScrollViewScrollState::LynxBaseScrollViewScrollStateIdle) {
        SendScrollEvent("scrollstart", lepus_value());
      }
      break;
    case LynxBaseScrollViewScrollState::LynxBaseScrollViewScrollStateFling:
      break;
    default:
      break;
  }

  auto params = lepus::Dictionary::Create();
  params->SetValue("previousState", static_cast<int>(from));
  params->SetValue("currentState", static_cast<int>(to));
  SendScrollEvent("scrollstatechange", lepus_value(params));
}

void LynxUIScrollViewInternal::ScrollViewDidScroll() {
  if (!is_first_render_) {
    context_->NotifyUIScroll();
    TryToSendScrollEvent();
    UpdateScrollPosition();
  }
  UpdateStickyItems();
}

void LynxUIScrollViewInternal::UpdateStickyItems() {
  float scroll_offset[2] = {0, 0};
  scroll_view_->GetScrollOffset(scroll_offset);
  for (const auto child : children_) {
    if (child) {
      child->CheckStickyOnParentScroll(scroll_offset[0], scroll_offset[1]);
      if (child->sticky_info_) {
        NodeManager::Instance().SetAttributeWithNumberValue(
            child->DrawNode(), NODE_TRANSLATE, child->sticky_info_->translate_x,
            child->sticky_info_->translate_y, 0);
        NodeManager::Instance().SetAttributeWithNumberValue(child->DrawNode(),
                                                            NODE_Z_INDEX, 1);
      }
    }
  }
}

void LynxUIScrollViewInternal::TryToSendScrollEvent() {
  if (throttle_ != 0.f) {
    auto current_time = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(current_time -
                                                              last_update_time_)
            .count() >= throttle_) {
      float scroll_offset[2] = {0, 0};
      scroll_view_->GetScrollOffset(scroll_offset);
      auto params = lepus::Dictionary::Create();
      params->SetValue("deltaX", scroll_offset[0] - last_content_offset_[0]);
      params->SetValue("deltaY", scroll_offset[1] - last_content_offset_[1]);
      SendScrollEvent("scroll", lepus_value(params));
      last_content_offset_[0] = scroll_offset[0];
      last_content_offset_[1] = scroll_offset[1];
      last_update_time_ = current_time;
    }
  } else {
    float scroll_offset[2] = {0, 0};
    scroll_view_->GetScrollOffset(scroll_offset);
    auto params = lepus::Dictionary::Create();
    params->SetValue("deltaX", scroll_offset[0] - last_content_offset_[0]);
    params->SetValue("deltaY", scroll_offset[1] - last_content_offset_[1]);
    SendScrollEvent("scroll", lepus_value(params));
    last_content_offset_[0] = scroll_offset[0];
    last_content_offset_[1] = scroll_offset[1];
  }
}

void LynxUIScrollViewInternal::UpdateScrollPosition() {
  float scroll_offset = scroll_view_->Vertical()
                            ? scroll_view_->GetScrollOffsetVertically()
                            : scroll_view_->GetScrollOffsetHorizontally();
  float scroll_range[2] = {0, 0};
  scroll_view_->Vertical()
      ? scroll_view_->GetScrollRangeVertically(scroll_range)
      : scroll_view_->GetScrollRangeHorizontally(scroll_range);

  bool at_upper = scroll_offset <= scroll_range[0] + upper_threshold_;
  bool at_lower = scroll_offset >= scroll_range[1] - lower_threshold_;

  if (at_upper && !at_upper_) {
    SendScrollEvent("scrolltoupper", lepus_value());
  }
  if (at_lower && !at_lower_) {
    SendScrollEvent("scrolltolower", lepus_value());
  }

  at_upper_ = at_upper;
  at_lower_ = at_lower;
}

void LynxUIScrollViewInternal::InsertNode(UIBase* child, int index) {
  NodeManager::Instance().InsertNode(scroll_view_->scroll_content_,
                                     child->DrawNode(), index);
}

void LynxUIScrollViewInternal::RemoveNode(UIBase* child) {
  NodeManager::Instance().RemoveNode(scroll_view_->scroll_content_,
                                     child->DrawNode());
}

void LynxUIScrollViewInternal::UpdateLayout(float left, float top, float width,
                                            float height, const float* paddings,
                                            const float* margins,
                                            const float* sticky,
                                            float max_height,
                                            uint32_t node_index) {
  UIView::UpdateLayout(left, top, width, height, paddings, margins, sticky,
                       max_height, node_index);
  NodeManager::Instance().SetAttributeWithNumberValue(scroll_view_->node_,
                                                      NODE_WIDTH, width_);
  NodeManager::Instance().SetAttributeWithNumberValue(scroll_view_->node_,
                                                      NODE_HEIGHT, height_);
}

void LynxUIScrollViewInternal::FinishLayoutOperation() {
  UIView::FinishLayoutOperation();
  float content_size[2] = {0, 0};
  for (auto child : children_) {
    content_size[0] +=
        child->width_ + child->margin_left_ + child->margin_right_;
    content_size[1] +=
        child->height_ + child->margin_top_ + child->margin_bottom_;
  }
  scroll_view_->SetScrollContentSize(content_size);
  FlushFirstRenderOperations();
  UpdateScrollPosition();
}

bool LynxUIScrollViewInternal::IsScrollable() { return true; }

float LynxUIScrollViewInternal::ScrollX() {
  return scroll_view_->GetScrollOffsetHorizontally();
}

float LynxUIScrollViewInternal::ScrollY() {
  return scroll_view_->GetScrollOffsetVertically();
}

void LynxUIScrollViewInternal::FlushFirstRenderOperations() {
  if (is_first_render_) {
    for (auto blk : *first_render_block_array_) {
      (*blk)(this);
    }
    delete first_render_block_array_;
    first_render_block_array_ = nullptr;
    is_first_render_ = false;
  }
}

void LynxUIScrollViewInternal::SendScrollEvent(const char* name,
                                               lepus::Value params) {
  auto scroll_event_detail = lepus::Dictionary::Create();
  // scroll_event->SetValue("direction", IsHorizontal() ? "left" : "bottom");
  scroll_event_detail->SetValue("scrollLeft",
                                scroll_view_->GetScrollOffsetHorizontally());
  scroll_event_detail->SetValue("scrollTop",
                                scroll_view_->GetScrollOffsetVertically());
  float scroll_range[4]{0.f};
  scroll_view_->GetScrollRange(scroll_range);
  scroll_event_detail->SetValue("scrollHeight", scroll_range[3]);
  scroll_event_detail->SetValue("scrollWidth", scroll_range[1]);
  scroll_event_detail->SetValue("isDragging", scroll_view_->Dragging());
  scroll_event_detail->SetValue("scrollState",
                                scroll_view_->CurrentScrollState());
  ForEachLepusValue(params, [&scroll_event_detail](const lepus::Value& key,
                                                   const lepus::Value& value) {
    scroll_event_detail->SetValue(key.String(), value);
  });

  CustomEvent scroll_event{Sign(), name, "detail",
                           lepus_value(scroll_event_detail)};
  context_->SendEvent(scroll_event);
}

void LynxUIScrollViewInternal::SetScrollOrientation(const lepus::Value& value) {
  scroll_view_->SetVertical(value.String() == "vertical");
}
void LynxUIScrollViewInternal::SetEnableScrollBar(const lepus::Value& value) {
  scroll_view_->EnableScrollBar(value.Bool());
}
void LynxUIScrollViewInternal::SetEnableScroll(const lepus::Value& value) {
  scroll_view_->EnableScroll(value.Bool());
}
void LynxUIScrollViewInternal::SetBounces(const lepus::Value& value) {
  scroll_view_->SetBounces(value.Bool());
}
void LynxUIScrollViewInternal::SetForwardsNestedScroll(
    const lepus::Value& value) {
  scroll_view_->SetForwardsNestedScrollMode(
      static_cast<NestedScrollMode>(value.Int32()));
}
void LynxUIScrollViewInternal::SetBackwardsNestedScroll(
    const lepus::Value& value) {
  scroll_view_->SetBackwardsNestedScrollMode(
      static_cast<NestedScrollMode>(value.Int32()));
}
void LynxUIScrollViewInternal::SetInitialScrollIndex(
    const lepus::Value& value) {
  if (first_render_block_array_ != nullptr) {
    first_render_block_array_->push_back(
        new base::MoveOnlyClosure<void, LynxUIScrollViewInternal*>(
            [value](LynxUIScrollViewInternal* ui) {
              int index = value.Int32();
              if (index >= 0 && index < ui->Children().size()) {
                float content_offset[2] = {0.f, 0.f};
                GetPositionOf(content_offset, ui->Children()[index], ui, false);
                ui->scroll_view_->ScrollTo(content_offset);
              }
            }));
  }
}
void LynxUIScrollViewInternal::SetInitialScrollOffset(
    const lepus::Value& value) {
  if (first_render_block_array_ != nullptr) {
    first_render_block_array_->push_back(
        new base::MoveOnlyClosure<void, LynxUIScrollViewInternal*>(
            [value](LynxUIScrollViewInternal* ui) {
              float offset =
                  ui->ToVPFromUnitValue(value.StdString()).GetValue(0);
              float content_offset[2] = {offset, offset};
              FormatOffset(content_offset, ui, false);
              ui->scroll_view_->ScrollTo(content_offset);
            }));
  }
}
void LynxUIScrollViewInternal::SetUpperThreshold(const lepus::Value& value) {
  upper_threshold_ = ToVPFromUnitValue(value.StdString()).GetValue(0);
}
void LynxUIScrollViewInternal::SetLowerThreshold(const lepus::Value& value) {
  lower_threshold_ = ToVPFromUnitValue(value.StdString()).GetValue(0);
}
void LynxUIScrollViewInternal::SetScrollEventThreshold(
    const lepus::Value& value) {
  throttle_ = value.Number();
}

void LynxUIScrollViewInternal::UIScrollTo(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  const auto& params = args.Table();
  float offset = 0.f;
  bool animated = true;
  int index = 0;
  if (params->Contains("offset")) {
    offset =
        ToVPFromUnitValue(params->GetValue("offset").StdString()).GetValue(0);
  }
  if (params->Contains("index")) {
    index = static_cast<int32_t>(params->GetValue("index").Number());
  }
  if (params->Contains("animated")) {
    animated = params->GetValue("animated").Bool();
  }
  if (index >= 0 && index < children_.size()) {
    UIBase* child = children_[index];
    float content_offset[2] = {0.f, 0.f};
    GetPositionOf(content_offset, child, this, false);
    float delta[2] = {offset, offset};
    AddOffset(content_offset, delta, false);
    if (animated) {
      scroll_view_->AnimatedScrollTo(content_offset);
    } else {
      scroll_view_->ScrollTo(content_offset);
    }
    callback(LynxGetUIResult::SUCCESS, lepus_value());
  } else {
    std::stringstream msg;
    msg << "scrollTo index: " << index << " is out of range[0, "
        << children_.size() << ").";
    callback(LynxGetUIResult::PARAM_INVALID, lepus_value(msg.str()));
  }
}

void LynxUIScrollViewInternal::UIScrollBy(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  const auto& params = args.Table();
  float offset = 0.f;
  if (params->Contains("offset")) {
    offset =
        ToVPFromUnitValue(params->GetValue("offset").StdString()).GetValue(0);
  }
  float delta[2] = {offset, offset};
  scroll_view_->ScrollBy(delta);

  callback(LynxGetUIResult::SUCCESS, lepus_value());
}
void LynxUIScrollViewInternal::UIAutoScroll(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  const auto& params = args.Table();
  bool start = false;
  if (params->Contains("start")) {
    start = params->GetValue("start").Bool();
  }
  if (start) {
    float rate = 0.f;
    if (params->Contains("rate")) {
      rate =
          ToVPFromUnitValue(params->GetValue("rate").StdString()).GetValue(0);
    }
    rate = rate / 60.0f;
    if (abs(rate) <= 1.0 / context_->DevicePixelRatio()) {
      callback(LynxGetUIResult::PARAM_INVALID,
               lepus_value("rate is too small to scroll"));
      return;
    }

    auto_scroller_->StartAutoScroll(rate, true);
  } else {
    auto_scroller_->StopAutoScroll();
  }

  callback(LynxGetUIResult::SUCCESS, lepus_value());
}

void LynxUIScrollViewInternal::UIGetScrollInfo(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  auto info = lepus::Dictionary::Create();
  info->SetValue("scrollLeft", scroll_view_->GetScrollOffsetHorizontally());
  info->SetValue("scrollTop", scroll_view_->GetScrollOffsetVertically());
  float scroll_range[4]{0.f};
  scroll_view_->GetScrollRange(scroll_range);
  info->SetValue("scrollWidth", scroll_range[1]);
  info->SetValue("scrollHeight", scroll_range[3]);
  callback(LynxGetUIResult::SUCCESS, lepus_value(info));
}

void LynxUIScrollViewInternal::UITakeContentScreenshot(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  LynxUIScreenshotHelper::TakeContentScreenshotForNode(
      context_->GetNapiEnv(), scroll_view_->scroll_content_, args,
      std::move(callback));
}

void LynxUIScrollViewInternal::ScrollIntoView(bool smooth, const UIBase* target,
                                              const std::string& block,
                                              const std::string& inline_value) {
  const std::string& type = scroll_view_->Vertical() ? block : inline_value;
  if (!target) {
    return;
  }

  float scroll_target[2] = {0, 0};
  if (scroll::kCenter == type) {
    scroll_target[1] -= (height_ - target->height_) / 2;
    scroll_target[0] -= (width_ - target->width_) / 2;
  } else if (scroll::kEnd == inline_value) {
    scroll_target[1] -= (height_ - target->height_);
    scroll_target[0] -= (width_ - target->width_);
  }

  while (target && target != this) {
    scroll_target[1] += target->top_;
    scroll_target[0] += target->left_;
    target = target->Parent();
  }

  if (smooth) {
    scroll_view_->AnimatedScrollTo(scroll_target);
  } else {
    scroll_view_->ScrollTo(scroll_target);
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
