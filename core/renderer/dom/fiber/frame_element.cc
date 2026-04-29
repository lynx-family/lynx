// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/frame_element.h"

#include <utility>

#include "base/include/log/logging.h"
#include "base/include/value/base_value.h"
#include "core/renderer/dom/element_manager_delegate.h"
#include "core/renderer/template_assembler.h"
#include "core/services/feature_count/global_feature_counter.h"

namespace lynx {
namespace tasm {

namespace {
constexpr char kDefaultFrameTag[] = "frame";
constexpr char kParamsName[] = "detail";
constexpr char kFrameLoaded[] = "frameLoaded";
constexpr char kFrameElement[] = "frameElement";
constexpr char kLoad[] = "load";
constexpr char kURL[] = "url";
constexpr char kStatusCode[] = "statusCode";
constexpr char kStatusMessage[] = "statusMessage";
constexpr char kFrameData[] = "data";
constexpr char kGlobalProps[] = "global-props";
}  // namespace

FrameElement::FrameElement(ElementManager* element_manager)
    : FiberElement(element_manager, BASE_STATIC_STRING(kDefaultFrameTag)) {}

void FrameElement::OnNodeAdded(FiberElement* child) {
  LOGE("frame element cannot adopt any child");
}

FrameElement::~FrameElement() {
  if (ShouldDestroy()) {
    element_manager()->element_manager_delegate()->OnFrameRemoved(this);
  }
}

void FrameElement::SetAttribute(const base::String& key,
                                const lepus::Value& value,
                                bool need_update_data_model) {
  OnSetSrc(key, value);
  if (key.IsEqual(kFrameData)) {
    data_ =
        value.IsEmpty()
            ? nullptr
            : std::make_unique<lepus::Value>(lepus::Value::ShallowCopy(value));
  } else if (key.IsEqual(kGlobalProps)) {
    global_props_ =
        value.IsEmpty()
            ? nullptr
            : std::make_unique<lepus::Value>(lepus::Value::ShallowCopy(value));
  }
  FiberElement::SetAttribute(key, value, need_update_data_model);
}

void FrameElement::ResetAttribute(const base::String& key) {
  if (key.IsEqual(kFrameData)) {
    data_ = nullptr;
  } else if (key.IsEqual(kGlobalProps)) {
    global_props_ = nullptr;
  }
  FiberElement::ResetAttribute(key);
}

void FrameElement::SetAttributeInternal(const base::String& key,
                                        const lepus::Value& value) {
  if (key.IsEqual(kFrameData)) {
    auto* transfer_value = data_ ? new lepus::Value(*data_) : nullptr;
    FiberElement::SetAttributeInternal(
        key, lepus::Value(reinterpret_cast<int64_t>(transfer_value)));
    return;
  }
  if (key.IsEqual(kGlobalProps)) {
    auto* transfer_value =
        global_props_ ? new lepus::Value(*global_props_) : nullptr;
    FiberElement::SetAttributeInternal(
        key, lepus::Value(reinterpret_cast<int64_t>(transfer_value)));
    return;
  }
  FiberElement::SetAttributeInternal(key, value);
}

void FrameElement::OnSetSrc(const base::String& key,
                            const lepus::Value& value) {
  BASE_STATIC_STRING_DECL(kSrc, "src");
  if (key == kSrc && value.IsString()) {
    std::string src = value.String().str();
    TRACE_EVENT(LYNX_TRACE_CATEGORY, FRAME_ELEMENT_ON_SET_SRC, "src", src);
    if (src != src_) {
      src_ = std::move(src);
      bundle_data_ = nullptr;
      element_manager()->element_manager_delegate()->LoadFrameBundle(src_,
                                                                     this);
    }
    report::GlobalFeatureCounter::Count(
        report::LynxFeature::CPP_USE_FRAME_ELEMENT,
        element_manager()->GetInstanceId());
  }
}

bool FrameElement::DidBundleLoaded(
    const std::shared_ptr<FrameElementData>& data) {
  if (src_ != data->src) {
    LOGE("bundle loaded with wrong src:" << data->src << " expect:" << src_);
    return false;
  }

  TRACE_EVENT(LYNX_TRACE_CATEGORY, FRAME_ELEMENT_DID_BUNDLED_LOADED, "src",
              src_);
  if (HasPaintingNode()) {
    if (data->error_code == error::E_SUCCESS && data->bundle) {
      element_container()->SetFrameAppBundle(data->bundle);
    } else {
      LOGE("load frame bundle failed:" << data->error_message);
    }

    SendLoadEvent(data);
    // TODO(yangguangzhao.solace): remove this when unified pipeline is ready
    element_container()->Flush();
  } else {
    bundle_data_ = data;
  }
  return true;
}

void FrameElement::FlushProps() {
  FiberElement::FlushProps();
  TRACE_EVENT(LYNX_TRACE_CATEGORY, FRAME_ELEMENT_FLUSH_PROPS, "src", src_);
  if (bundle_data_ && HasPaintingNode()) {
    if (bundle_data_->bundle) {
      element_container()->SetFrameAppBundle(bundle_data_->bundle);
    }
    SendLoadEvent(bundle_data_);
    bundle_data_ = nullptr;
  }
}

void FrameElement::SendLoadEvent(
    const std::shared_ptr<FrameElementData>& data) {
  auto detail = lepus::Dictionary::Create();
  detail->SetValue(BASE_STATIC_STRING(kURL), data->src);
  detail->SetValue(BASE_STATIC_STRING(kStatusCode), data->error_code);
  detail->SetValue(BASE_STATIC_STRING(kStatusMessage), data->error_message);

  if (data_model()->static_events().find(BASE_STATIC_STRING(kLoad)) !=
      data_model()->static_events().end()) {
    element_manager()->SendNativeCustomEvent(kLoad, impl_id(),
                                             lepus::Value(detail), kParamsName);
  } else {
    LOGI("bindload callback not found");
  }

  auto frame_loaded_info = lepus::Dictionary::Create();
  frame_loaded_info->SetValue(BASE_STATIC_STRING(kParamsName),
                              std::move(detail));
  frame_loaded_info->SetValue(BASE_STATIC_STRING(kFrameElement),
                              fml::RefPtr<FiberElement>(this));

  if (auto* delegate = element_manager()->element_manager_delegate()) {
    delegate->TriggerLepusGlobalEvent(
        kFrameLoaded, lepus::Value(std::move(frame_loaded_info)));
  }
}

const std::string& FrameElement::GetSrc() const { return src_; }

}  // namespace tasm
}  // namespace lynx
