// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/frame_view.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#include "clay/gfx/geometry/float_size.h"
#include "clay/lynx_adaptor/clay_value.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/frame_child_surface_host.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/event/key_event.h"
#include "clay/ui/rendering/render_frame.h"
#include "clay/ui/shadow/frame_shadow_node.h"
#include "core/renderer/data/template_data.h"
#include "core/value_wrapper/value_wrapper_utils.h"

namespace clay {
namespace {

constexpr const char* kSrc = "src";
constexpr const char* kData = "data";
constexpr const char* kGlobalProps = "global-props";
constexpr const char* kEmbeddedMode = "embedded-mode";
constexpr const char* kAutoWidth = "auto-width";
constexpr const char* kAutoHeight = "auto-height";
constexpr const char* kPresetWidth = "preset-width";
constexpr const char* kPresetHeight = "preset-height";
constexpr const char* kEnableMultiAsyncThread = "enable-multi-async-thread";

}  // namespace

FrameView::FrameView(int id, PageView* page_view)
    : WithTypeInfo(id, "frame", std::make_unique<RenderFrame>(), page_view),
      child_surface_host_(std::make_unique<FrameChildSurfaceHost>(
          render_object()->element_id(), page_view)) {
#if defined(ENABLE_MOUSE_TRACKING)
  if (auto* mouse_region_manager = page_view->mouse_region_manager()) {
    mouse_region_manager->RegisterEnterCallback(
        this, [this](const PointerEvent& event) {
          BaseView::OnMouseEnter(event);
          ForwardMouseRegionEvent(event);
        });
    mouse_region_manager->RegisterLeaveCallback(
        this, [this](const PointerEvent& event) {
          BaseView::OnMouseLeave(event);
          ForwardMouseRegionEvent(event);
        });
    mouse_region_manager->RegisterHoverTrackingCallback(
        this,
        [this](const PointerEvent& event) { ForwardMouseRegionEvent(event); });
  }
#endif
}

FrameView::~FrameView() = default;

void FrameView::SetAttribute(const char* attr, const clay::Value& value) {
  const std::string name(attr);
  if (name == kSrc) {
    if (value.IsString() && url_ != value.GetString()) {
      url_ = value.GetString();
      is_url_changed_ = true;
      has_attached_bundle_ = false;
      pending_bundle_.reset();
      current_surface_id_.reset();
      ResetIntrinsicContentSize();
      if (auto* frame = static_cast<RenderFrame*>(render_object())) {
        frame->ClearFrameSurfaceId();
      }
      ClearEventForwarder();
      if (child_surface_host_) {
        child_surface_host_->Clear();
      }
    }
  } else if (name == kData) {
    pending_data_ = ToTemplateData(value);
    is_props_updated_ = true;
  } else if (name == kGlobalProps) {
    pending_global_props_ = ToTemplateData(value);
    is_props_updated_ = true;
  } else if (name == kEmbeddedMode) {
    if (auto number = ToNumber(value)) {
      embedded_mode_ = static_cast<int>(*number);
    }
  } else if (name == kAutoWidth) {
    const bool auto_width = ToBool(value).value_or(false);
    if (auto_width_ != auto_width) {
      auto_width_ = auto_width;
      ApplyIntrinsicContentSize(true);
    }
  } else if (name == kAutoHeight) {
    const bool auto_height = ToBool(value).value_or(false);
    if (auto_height_ != auto_height) {
      auto_height_ = auto_height;
      ApplyIntrinsicContentSize(true);
    }
  } else if (name == kPresetWidth) {
    preset_width_ = ToPresetLength(value);
  } else if (name == kPresetHeight) {
    preset_height_ = ToPresetLength(value);
  } else if (name == kEnableMultiAsyncThread) {
    enable_multi_async_thread_ = ToBool(value);
  }

  BaseView::SetAttribute(attr, value);
}

void FrameView::OnLayoutUpdated() {
  content_frame_initialized_ = true;
  content_frame_ = FloatRect(0.f, 0.f, std::max(ContentWidth(), 0.f),
                             std::max(ContentHeight(), 0.f));
  if (child_surface_host_) {
    child_surface_host_->SetSurfaceSize(
        FloatSize(content_frame_.width(), content_frame_.height()));
  }
}

FloatSize FrameView::child_viewport_size() const {
  float width = std::max(content_frame_.width(), 0.f);
  float height = std::max(content_frame_.height(), 0.f);
  // An auto-sized axis needs the preset as its bootstrap constraint until the
  // child reports an intrinsic size, even if the parent has already laid out.
  if (!content_frame_initialized_ ||
      (auto_width_ && !intrinsic_content_size_)) {
    width = preset_width_.value_or(width);
  }
  if (!content_frame_initialized_ ||
      (auto_height_ && !intrinsic_content_size_)) {
    height = preset_height_.value_or(height);
  }
  return FloatSize(width, height);
}

void FrameView::HandleEvent(const PointerEvent& event) {
  if (pointer_event_forwarder_ && pointer_event_forwarder_(event)) {
    return;
  }
  BaseView::HandleEvent(event);
}

void FrameView::ForwardMouseRegionEvent(const PointerEvent& event) {
  // Hover is maintained by MouseRegionManager and does not reach HandleEvent.
  // Keep down/up/wheel on the normal hit-test path to avoid duplicate dispatch.
  if (event.type != PointerEvent::EventType::kHoverEvent &&
      event.type != PointerEvent::EventType::kCancel) {
    return;
  }
  if (pointer_event_forwarder_) {
    pointer_event_forwarder_(event);
  }
}

bool FrameView::OnKeyEvent(const KeyEvent* event) {
  if (key_event_forwarder_ && key_event_forwarder_(event)) {
    return true;
  }
  return BaseView::OnKeyEvent(event);
}

void FrameView::OnReceiveAppBundle(
    const std::shared_ptr<lynx::tasm::LynxTemplateBundle>& bundle) {
  pending_bundle_ = bundle;
}

void FrameView::MarkPropsUpdated() { is_props_updated_ = true; }

bool FrameView::SubmitChildLayerTree(std::shared_ptr<LayerTree> layer_tree,
                                     std::optional<skity::Rect> damage_rect) {
  if (!child_surface_host_) {
    return false;
  }
  const auto pending_surface_id = child_surface_host_->SubmitLayerTree(
      std::move(layer_tree), child_viewport_size(), damage_rect);
  if (!pending_surface_id) {
    return false;
  }
  current_surface_id_ = pending_surface_id;
  if (auto* frame = static_cast<RenderFrame*>(render_object())) {
    frame->SetFrameSurfaceId(*current_surface_id_);
    frame->MarkNeedsPaint(true);
  }
  page_view()->RequestPaintBase();
  return true;
}

void FrameView::SetIntrinsicContentSize(float width, float height) {
  if (!std::isfinite(width) || !std::isfinite(height)) {
    return;
  }
  const FloatSize intrinsic_size(std::max(width, 0.f), std::max(height, 0.f));
  if (intrinsic_content_size_ && *intrinsic_content_size_ == intrinsic_size) {
    return;
  }
  intrinsic_content_size_ = intrinsic_size;
  ApplyIntrinsicContentSize();
}

void FrameView::ApplyIntrinsicContentSize(bool force_layout) {
  auto* shadow_node = page_view()->GetShadowNodeById(id());
  if (!shadow_node || !shadow_node->IsFrameShadowNode()) {
    return;
  }
  auto* frame_shadow_node = static_cast<FrameShadowNode*>(shadow_node);
  frame_shadow_node->SetIntrinsicContentSize(
      auto_width_ && intrinsic_content_size_
          ? std::make_optional(intrinsic_content_size_->width())
          : std::nullopt,
      auto_height_ && intrinsic_content_size_
          ? std::make_optional(intrinsic_content_size_->height())
          : std::nullopt);
  if (force_layout) {
    frame_shadow_node->MarkDirty();
  }
}

void FrameView::ResetIntrinsicContentSize() {
  if (!intrinsic_content_size_) {
    return;
  }
  intrinsic_content_size_.reset();
  ApplyIntrinsicContentSize();
}

void FrameView::SetEventForwarder(
    std::function<bool(const PointerEvent&)> pointer,
    std::function<bool(const KeyEvent*)> key) {
  pointer_event_forwarder_ = std::move(pointer);
  key_event_forwarder_ = std::move(key);
}

void FrameView::ClearEventForwarder() {
  pointer_event_forwarder_ = nullptr;
  key_event_forwarder_ = nullptr;
}

void FrameView::DestroyChildContent() {
  ClearEventForwarder();
  pending_bundle_.reset();
  pending_data_.reset();
  pending_global_props_.reset();
  is_url_changed_ = false;
  is_props_updated_ = false;
  has_attached_bundle_ = false;
  current_surface_id_.reset();
  ResetIntrinsicContentSize();
  if (auto* frame = static_cast<RenderFrame*>(render_object())) {
    frame->ClearFrameSurfaceId();
  }
  if (child_surface_host_) {
    child_surface_host_->Clear();
  }
}

bool FrameView::HasPendingBundleReady() const { return CanLoadPendingBundle(); }

bool FrameView::HasPendingMetadataUpdate() const {
  return has_attached_bundle_ && is_props_updated_ && !is_url_changed_ &&
         (pending_data_ || pending_global_props_);
}

void FrameView::CommitPendingBundleAttached() {
  has_attached_bundle_ = true;
  is_url_changed_ = false;
  is_props_updated_ = false;
  pending_bundle_.reset();
  pending_data_.reset();
  pending_global_props_.reset();
  render_object()->MarkNeedsPaint();
  page_view()->RequestPaintBase();
}

void FrameView::CommitPendingMetadataUpdated() {
  is_props_updated_ = false;
  pending_data_.reset();
  pending_global_props_.reset();
}

std::optional<float> FrameView::ToNumber(const clay::Value& value) {
  if (value.IsInt()) {
    return static_cast<float>(value.GetInt());
  }
  if (value.IsUint()) {
    return static_cast<float>(value.GetUint());
  }
  if (value.IsLong()) {
    return static_cast<float>(value.GetLong());
  }
  if (value.IsFloat()) {
    return value.GetFloat();
  }
  if (value.IsDouble()) {
    return static_cast<float>(value.GetDouble());
  }
  return std::nullopt;
}

std::optional<float> FrameView::ToPresetLength(const clay::Value& value) const {
  if (value.IsNone() || value.IsNull()) {
    return std::nullopt;
  }
  attribute_utils::Length length;
  if (!attribute_utils::TryGetLength(value, length) ||
      length.unit == attribute_utils::Unit::kPercent) {
    return std::nullopt;
  }
  const float result =
      attribute_utils::ToPxWithDisplayMetrics(length, page_view());
  return std::isfinite(result) && result >= 0.f ? std::make_optional(result)
                                                : std::nullopt;
}

std::optional<bool> FrameView::ToBool(const clay::Value& value) {
  if (value.IsBool()) {
    return value.GetBool();
  }
  if (auto number = ToNumber(value)) {
    return *number != 0.f;
  }
  return std::nullopt;
}

std::shared_ptr<lynx::tasm::TemplateData> FrameView::ToTemplateData(
    const clay::Value& value) {
  if (value.IsNone() || value.IsNull()) {
    return nullptr;
  }

  if (value.IsLong()) {
    auto* lepus_value = reinterpret_cast<lynx::lepus::Value*>(value.GetLong());
    if (!lepus_value) {
      return nullptr;
    }
    auto data = std::make_shared<lynx::tasm::TemplateData>(*lepus_value, false);
    delete lepus_value;
    return data;
  }

  lynx::ClayValueWrapper wrapper(value);
  auto lepus_value = lynx::pub::ValueUtils::ConvertValueToLepusValue(wrapper);
  return std::make_shared<lynx::tasm::TemplateData>(lepus_value, false);
}

bool FrameView::CanLoadPendingBundle() const {
  return pending_bundle_ && is_url_changed_ && is_props_updated_;
}

}  // namespace clay
