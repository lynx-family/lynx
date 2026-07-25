// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_frame.h"

#include <arkui/native_node_napi.h>

#include <algorithm>
#include <cmath>
#include <utility>

#include "base/include/float_comparison.h"
#include "base/include/log/logging.h"
#include "base/include/platform/harmony/napi_util.h"
#include "base/include/value/table.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/base/harmony/napi_convert_helper.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/dom/harmony/lynx_template_bundle_harmony.h"
#include "core/renderer/ui_wrapper/common/harmony/prop_bundle_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/frame_shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {
namespace tasm {
namespace harmony {
namespace {

constexpr int32_t kMeasureModeIndefinite = 0;
constexpr int32_t kMeasureModeDefinite = 1;
// Half-pixel dead-zone for intrinsic size changes. Deliberately coarser than
// base::EPSILON so sub-pixel jitter from the host does not trigger relayout.
constexpr float kIntrinsicSizeEpsilon = 0.5f;

struct NativeFrameBinding {
  std::weak_ptr<UIBase> frame;
};

bool ReadBool(const lepus::Value& value, bool fallback) {
  if (value.IsBool()) {
    return value.Bool();
  }
  if (value.IsString()) {
    return value.StdString() == "true";
  }
  if (value.IsNumber()) {
    return !base::FloatsEqual(static_cast<float>(value.Number()), 0.f);
  }
  return fallback;
}

bool IsValidMetaDataValue(const lepus::Value& value) {
  return value.IsObject();
}

bool IsNapiNumber(napi_env env, napi_value value) {
  if (!value) {
    return false;
  }
  napi_valuetype type;
  return napi_typeof(env, value, &type) == napi_ok && type == napi_number;
}

}  // namespace

UIFrame::UIFrame(LynxContext* context, int sign, const std::string& tag)
    : UIView(context, ARKUI_NODE_STACK, sign, tag),
      env_(context->GetNapiEnv()) {}

UIFrame::~UIFrame() { OnDestroy(); }

napi_value UIFrame::Init(napi_env env, napi_value exports) {
#define DECLARE_NAPI_FUNCTION(name, func) \
  {(name), nullptr, (func), nullptr, nullptr, nullptr, napi_default, nullptr}

  napi_property_descriptor properties[] = {
      DECLARE_NAPI_FUNCTION("onHostReady", OnHostReady),
      DECLARE_NAPI_FUNCTION("onIntrinsicSizeChanged", OnIntrinsicSizeChanged),
  };
#undef DECLARE_NAPI_FUNCTION

  napi_value cons;
  napi_define_class(env, "UIFrame", NAPI_AUTO_LENGTH, New, nullptr,
                    sizeof(properties) / sizeof(properties[0]), properties,
                    &cons);
  napi_set_named_property(env, exports, "UIFrame", cons);
  return exports;
}

napi_value UIFrame::New(napi_env env, napi_callback_info info) {
  napi_value js_this;
  napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
  return js_this;
}

napi_value UIFrame::OnHostReady(napi_env env, napi_callback_info info) {
  napi_value js_this;
  size_t argc = 0;
  napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);
  if (auto frame = GetFrameFromNapiThis(env, js_this)) {
    frame->HandleHostReady();
  }
  return nullptr;
}

napi_value UIFrame::OnIntrinsicSizeChanged(napi_env env,
                                           napi_callback_info info) {
  napi_value js_this;
  size_t argc = 2;
  napi_value argv[2] = {nullptr};
  if (napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr) != napi_ok ||
      argc < 2 || !IsNapiNumber(env, argv[0]) || !IsNapiNumber(env, argv[1])) {
    return nullptr;
  }
  auto frame = GetFrameFromNapiThis(env, js_this);
  if (!frame) {
    return nullptr;
  }
  double width = 0;
  double height = 0;
  if (napi_get_value_double(env, argv[0], &width) != napi_ok ||
      napi_get_value_double(env, argv[1], &height) != napi_ok) {
    return nullptr;
  }
  frame->HandleIntrinsicSizeChanged(static_cast<float>(width),
                                    static_cast<float>(height));
  return nullptr;
}

std::shared_ptr<UIFrame> UIFrame::GetFrameFromNapiThis(napi_env env,
                                                       napi_value js_this) {
  NativeFrameBinding* binding = nullptr;
  if (napi_unwrap(env, js_this, reinterpret_cast<void**>(&binding)) !=
          napi_ok ||
      !binding) {
    return nullptr;
  }
  auto frame = binding->frame.lock();
  if (!frame || frame->Tag() != kTag) {
    return nullptr;
  }
  return std::static_pointer_cast<UIFrame>(frame);
}

void UIFrame::SetFrameAppBundle(
    std::shared_ptr<tasm::LynxTemplateBundle> bundle) {
  if (!bundle) {
    return;
  }
  pending_bundle_ = std::move(bundle);
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, UI_FRAME_SET_FRAME_APP_BUNDLE, "url",
                      src_, "loaded", loaded_, "props_updated", props_updated_,
                      "child_ready", child_context_ready_, "has_src",
                      !src_.empty());
  // Keep the current loaded state until the matching src prop is updated.
  // This allows the bundle to arrive before props without loading it with the
  // previous src.
  TryLoadBundle();
}

void UIFrame::UpdateProps(PropBundleHarmony* props) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_FRAME_UPDATE_PROPS, "url", src_);
  CreateFrameHost();
  UIView::UpdateProps(props);
  props_updated_ = true;
  UpdateHostAutoSize();
  UpdateHostMetaDataIfNeeded();
  TryLoadBundle();
}

void UIFrame::UpdateLayout(float left, float top, float width, float height,
                           const float* paddings, const float* margins,
                           const float* sticky, float max_height,
                           uint32_t node_index) {
  UIView::UpdateLayout(left, top, width, height, paddings, margins, sticky,
                       max_height, node_index);
  const float horizontal_padding = padding_left_ + padding_right_;
  const float vertical_padding = padding_top_ + padding_bottom_;
  const float horizontal_border = GetBorderLeftWidth() + GetBorderRightWidth();
  const float vertical_border = GetBorderTopWidth() + GetBorderBottomWidth();
  content_width_ =
      std::max(0.f, width - horizontal_padding - horizontal_border);
  content_height_ = std::max(0.f, height - vertical_padding - vertical_border);
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_FRAME_VIEW_UPDATE_LAYOUT, "url", src_,
              "width", content_width_, "height", content_height_);
  UpdateHostViewport();
}

void UIFrame::OnDestroy() {
  if (destroyed_) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_FRAME_VIEW_DESTROY, "url", src_);
  destroyed_ = true;
  DisposeFrameHost();
  pending_bundle_.reset();
}

void UIFrame::HandleHostReady() {
  if (destroyed_) {
    return;
  }
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, UI_FRAME_HOST_READY, "url", src_,
                      "loaded", loaded_, "props_updated", props_updated_,
                      "has_bundle", static_cast<bool>(pending_bundle_));
  child_context_ready_ = true;
  UpdateHostAutoSize();
  UpdateHostViewport();
  TryLoadBundle();
}

void UIFrame::HandleIntrinsicSizeChanged(float width, float height) {
  if (destroyed_ || !std::isfinite(width) || !std::isfinite(height)) {
    return;
  }
  width = std::max(0.f, width);
  height = std::max(0.f, height);
  if (std::abs(intrinsic_width_ - width) < kIntrinsicSizeEpsilon &&
      std::abs(intrinsic_height_ - height) < kIntrinsicSizeEpsilon) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_FRAME_VIEW_SET_INTRINSIC_CONTENT_SIZE,
              "url", src_, "old_width", intrinsic_width_, "old_height",
              intrinsic_height_, "width", width, "height", height);
  intrinsic_width_ = width;
  intrinsic_height_ = height;

  if (context_) {
    context_->FindShadowNodeAndRunTask(
        Sign(), [width, height](ShadowNode* shadow_node) {
          if (shadow_node && std::string(shadow_node->Tag()) == kTag) {
            static_cast<FrameShadowNode*>(shadow_node)
                ->UpdateIntrinsicSize(width, height);
          }
        });
  }
}

void UIFrame::OnPropUpdate(const std::string& name, const lepus::Value& value) {
  UIView::OnPropUpdate(name, value);
  if (name == "src") {
    const std::string next_src = value.IsString() ? value.StdString() : "";
    if (next_src != src_) {
      TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, UI_FRAME_SET_SRC, "url",
                          next_src, "has_src", !next_src.empty());
      props_updated_ = false;
      src_ = next_src;
      ResetLoadedState();
    }
  } else if (name == "data") {
    TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, LYNX_FRAME_VIEW_SET_INIT_DATA,
                        "url", src_, "valid", IsValidMetaDataValue(value));
    has_frame_data_ = IsValidMetaDataValue(value);
    frame_data_ = has_frame_data_ ? value : lepus::Value();
    frame_data_dirty_ = true;
  } else if (name == "global-props") {
    TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, LYNX_FRAME_VIEW_SET_GLOBAL_PROPS,
                        "url", src_, "valid", IsValidMetaDataValue(value));
    has_global_props_ = IsValidMetaDataValue(value);
    global_props_ = has_global_props_ ? value : lepus::Value();
    global_props_dirty_ = true;
  } else if (name == "auto-width") {
    auto_width_ = ReadBool(value, auto_width_);
  } else if (name == "auto-height") {
    auto_height_ = ReadBool(value, auto_height_);
  }
}

void UIFrame::CreateFrameHost() {
  if (!context_ || !context_->GetUIOwner() || frame_host_ref_) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_FRAME_CREATE_FRAME_HOST, "url", src_);
  auto host = CreateFrameHostFromCreator();
  frame_host_ref_ = host.host_ref;
  frame_host_node_ = host.node;
  if (!frame_host_ref_ || !frame_host_node_) {
    LOGE("UIFrame failed to create frame host, url: "
         << src_ << ", has_host: " << static_cast<bool>(frame_host_ref_)
         << ", has_node: " << static_cast<bool>(frame_host_node_));
  }
  if (frame_host_node_) {
    NodeManager::Instance().InsertNode(Node(), frame_host_node_, 0);
  }
}

void UIFrame::DisposeFrameHost() {
  if (frame_host_node_) {
    NodeManager::Instance().RemoveNode(Node(), frame_host_node_);
    frame_host_node_ = nullptr;
  }
  if (frame_host_ref_ && env_) {
    base::NapiHandleScope scope(env_);
    CallHostMethod("dispose", 0, nullptr);
    napi_delete_reference(env_, frame_host_ref_);
    frame_host_ref_ = nullptr;
  }
  child_context_ready_ = false;
}

UIFrame::FrameHostCreationResult UIFrame::CreateFrameHostFromCreator() {
  FrameHostCreationResult host;
  auto* owner = context_->GetUIOwner();
  if (!owner) {
    return host;
  }

  base::NapiHandleScope scope(env_);
  napi_value native_frame = CreateNativeFrameValue();
  if (!native_frame) {
    return host;
  }
  napi_value js_host = owner->CreateFrameHost(native_frame, Sign());
  if (!js_host) {
    return host;
  }
  napi_valuetype host_type;
  if (napi_typeof(env_, js_host, &host_type) != napi_ok ||
      host_type != napi_object) {
    return host;
  }

  napi_value get_frame_node;
  if (napi_get_named_property(env_, js_host, "getFrameNode", &get_frame_node) !=
      napi_ok) {
    return host;
  }
  napi_valuetype method_type;
  if (napi_typeof(env_, get_frame_node, &method_type) != napi_ok ||
      method_type != napi_function) {
    return host;
  }
  napi_value frame_node;
  if (napi_call_function(env_, js_host, get_frame_node, 0, nullptr,
                         &frame_node) != napi_ok) {
    return host;
  }
  napi_valuetype node_type;
  if (napi_typeof(env_, frame_node, &node_type) == napi_ok &&
      node_type != napi_null && node_type != napi_undefined) {
    OH_ArkUI_GetNodeHandleFromNapiValue(env_, frame_node, &host.node);
  }
  if (napi_create_reference(env_, js_host, 1, &host.host_ref) != napi_ok) {
    host.node = nullptr;
    return FrameHostCreationResult{};
  }
  return host;
}

napi_value UIFrame::CreateNativeFrameValue() {
  napi_value native_frame;
  napi_status status = napi_create_object(env_, &native_frame);
  if (status != napi_ok) {
    LOGE("UIFrame failed to create native frame object, url: "
         << src_ << ", status: " << base::NapiUtil::StatusToString(status));
    return nullptr;
  }

  auto* binding = new NativeFrameBinding{weak_from_this()};
  status = napi_wrap(
      env_, native_frame, binding,
      [](napi_env env, void* data, void* hint) {
        delete static_cast<NativeFrameBinding*>(data);
      },
      nullptr, nullptr);
  if (status != napi_ok) {
    LOGE("UIFrame failed to wrap native frame object, url: "
         << src_ << ", status: " << base::NapiUtil::StatusToString(status));
    delete binding;
    return nullptr;
  }

#define DECLARE_NAPI_FUNCTION(name, func) \
  {(name), nullptr, (func), nullptr, nullptr, nullptr, napi_default, nullptr}

  napi_property_descriptor properties[] = {
      DECLARE_NAPI_FUNCTION("onHostReady", OnHostReady),
      DECLARE_NAPI_FUNCTION("onIntrinsicSizeChanged", OnIntrinsicSizeChanged),
  };
#undef DECLARE_NAPI_FUNCTION

  if (napi_define_properties(env_, native_frame,
                             sizeof(properties) / sizeof(properties[0]),
                             properties) != napi_ok) {
    void* wrapped = nullptr;
    napi_remove_wrap(env_, native_frame, &wrapped);
    delete static_cast<NativeFrameBinding*>(wrapped);
    return nullptr;
  }
  return native_frame;
}

void UIFrame::TryLoadBundle() {
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, UI_FRAME_TRY_LOAD_BUNDLE, "url",
                      src_, "loaded", loaded_, "props_updated", props_updated_,
                      "child_ready", child_context_ready_, "has_bundle",
                      static_cast<bool>(pending_bundle_), "has_src",
                      !src_.empty());
  if (loaded_ || !props_updated_ || !child_context_ready_ || !pending_bundle_ ||
      src_.empty()) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_FRAME_LOAD_BUNDLE, "url", src_);
  base::NapiHandleScope scope(env_);
  napi_value src = nullptr;
  napi_status status =
      napi_create_string_utf8(env_, src_.c_str(), src_.length(), &src);
  if (status != napi_ok) {
    LOGE("UIFrame failed to create src value, url: "
         << src_ << ", status: " << base::NapiUtil::StatusToString(status));
    return;
  }
  napi_value bundle =
      lynx::harmony::LynxTemplateBundleHarmony::CreateFromNative(
          env_, *pending_bundle_);
  if (!bundle) {
    LOGE("UIFrame failed to create template bundle value, url: " << src_);
    return;
  }
  napi_value argv[4] = {src, bundle,
                        CreateMetaDataValue(has_frame_data_, frame_data_),
                        CreateMetaDataValue(has_global_props_, global_props_)};
  if (CallHostMethod("loadTemplate", 4, argv)) {
    loaded_ = true;
    pending_bundle_.reset();
    frame_data_dirty_ = false;
    global_props_dirty_ = false;
    UpdateHostViewport();
  }
}

void UIFrame::UpdateHostAutoSize() {
  if (!frame_host_ref_) {
    return;
  }
  base::NapiHandleScope scope(env_);
  napi_value auto_width;
  napi_value auto_height;
  napi_get_boolean(env_, auto_width_, &auto_width);
  napi_get_boolean(env_, auto_height_, &auto_height);
  napi_value argv[2] = {auto_width, auto_height};
  CallHostMethod("updateAutoSize", 2, argv);
}

void UIFrame::UpdateHostViewport() {
  if (!child_context_ready_) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_FRAME_VIEW_SET_LAYOUT_MODE, "url", src_,
              "width", content_width_, "width_mode",
              auto_width_ ? kMeasureModeIndefinite : kMeasureModeDefinite,
              "height", content_height_, "height_mode",
              auto_height_ ? kMeasureModeIndefinite : kMeasureModeDefinite);
  base::NapiHandleScope scope(env_);
  napi_value width;
  napi_value width_mode;
  napi_value height;
  napi_value height_mode;
  napi_create_double(env_, content_width_, &width);
  napi_create_int32(env_,
                    auto_width_ ? kMeasureModeIndefinite : kMeasureModeDefinite,
                    &width_mode);
  napi_create_double(env_, content_height_, &height);
  napi_create_int32(
      env_, auto_height_ ? kMeasureModeIndefinite : kMeasureModeDefinite,
      &height_mode);
  napi_value argv[4] = {width, width_mode, height, height_mode};
  CallHostMethod("updateViewport", 4, argv);
}

void UIFrame::UpdateHostMetaDataIfNeeded() {
  if (!loaded_ || !child_context_ready_ ||
      (!frame_data_dirty_ && !global_props_dirty_)) {
    return;
  }
  base::NapiHandleScope scope(env_);
  napi_value argv[2] = {CreateMetaDataValue(has_frame_data_, frame_data_),
                        CreateMetaDataValue(has_global_props_, global_props_)};
  if (CallHostMethod("updateMetaData", 2, argv)) {
    frame_data_dirty_ = false;
    global_props_dirty_ = false;
  }
}

bool UIFrame::CallHostMethod(const char* method, size_t argc, napi_value* argv,
                             napi_value* result) {
  if (!env_ || !frame_host_ref_) {
    LOGE("UIFrame cannot call host method, url: "
         << src_ << ", method: " << method
         << ", has_env: " << static_cast<bool>(env_)
         << ", has_host: " << static_cast<bool>(frame_host_ref_));
    return false;
  }
  napi_value host =
      base::NapiUtil::GetReferenceNapiValue(env_, frame_host_ref_);
  if (!host) {
    LOGE("UIFrame failed to get frame host, url: " << src_
                                                   << ", method: " << method);
    return false;
  }
  napi_value func;
  napi_status status = napi_get_named_property(env_, host, method, &func);
  if (status != napi_ok) {
    LOGE("UIFrame failed to get host method, url: "
         << src_ << ", method: " << method
         << ", status: " << base::NapiUtil::StatusToString(status));
    return false;
  }
  napi_valuetype type = napi_undefined;
  status = napi_typeof(env_, func, &type);
  if (status != napi_ok || type != napi_function) {
    LOGE("UIFrame host property is not callable, url: "
         << src_ << ", method: " << method << ", status: "
         << base::NapiUtil::StatusToString(status) << ", type: " << type);
    return false;
  }
  napi_value local_result;
  status = napi_call_function(env_, host, func, argc, argv,
                              result ? result : &local_result);
  if (status != napi_ok) {
    LOGE("UIFrame failed to call host method, url: "
         << src_ << ", method: " << method
         << ", status: " << base::NapiUtil::StatusToString(status));
    return false;
  }
  return true;
}

napi_value UIFrame::CreateMetaDataValue(bool has_value,
                                        const lepus::Value& value) {
  if (!has_value) {
    napi_value undefined;
    napi_get_undefined(env_, &undefined);
    return undefined;
  }
  return base::NapiConvertHelper::CreateNapiValue(env_, value);
}

void UIFrame::ResetLoadedState() {
  loaded_ = false;
  child_context_ready_ = child_context_ready_ && frame_host_ref_;
  // The bundle may arrive before src. Keep it pending so UpdateProps() can
  // consume it after all props for the new src have been applied.
  intrinsic_width_ = 0.f;
  intrinsic_height_ = 0.f;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
