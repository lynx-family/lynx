// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"

#include <utility>

#include "base/include/closure.h"
#include "base/include/fml/task_runner.h"
#include "core/resource/lynx_resource_loader_harmony.h"
#include "core/shell/harmony/embedder_platform_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/custom_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_target.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/gesture_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/lynx_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/touch_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/image_shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/inline_placeholder_shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/inline_text_shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/inline_truncation_shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/js_shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/raw_text_shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/text_shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_image.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_list.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_root.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_scroll.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_text.h"

namespace lynx {
namespace tasm {
namespace harmony {

std::unordered_map<std::string, LynxContext::NodeInfo>&
LynxContext::GetCAPINodeInfoMap() {
  static base::NoDestructor<
      std::unordered_map<std::string, LynxContext::NodeInfo>>
      kNodeTypeInfoMap{{
          {"text",
           {UIText::Make, TextShadowNode::Make, LayoutNodeType::CUSTOM}},
          {"x-text",
           {UIText::Make, TextShadowNode::Make, LayoutNodeType::CUSTOM}},
          {"raw-text",
           {nullptr, RawTextShadowNode::Make,
            LayoutNodeType::CUSTOM | LayoutNodeType::VIRTUAL}},
          {"inline-truncation",
           {nullptr, InlineTruncationShadowNode::Make,
            LayoutNodeType::CUSTOM | LayoutNodeType::VIRTUAL}},
          {"inline-text",
           {nullptr, InlineTextShadowNode::Make,
            LayoutNodeType::CUSTOM | LayoutNodeType::VIRTUAL}},
          {"x-inline-truncation",
           {nullptr, InlineTruncationShadowNode::Make,
            LayoutNodeType::CUSTOM | LayoutNodeType::VIRTUAL}},
          {"x-inline-text",
           {nullptr, InlineTextShadowNode::Make,
            LayoutNodeType::CUSTOM | LayoutNodeType::VIRTUAL}},
          {"view", {UIView::Make}},
          {"image",
           {UIImage::Make, ImageShadowNode::Make, LayoutNodeType::CUSTOM}},
          {"filter-image",
           {UIImage::Make, ImageShadowNode::Make, LayoutNodeType::CUSTOM}},
          // Node type is common because the layout logic of inline image and
          // inline view is the same.
          {"inline-image",
           {UIImage::Make, InlinePlaceholderShadowNode::Make,
            LayoutNodeType::COMMON}},
          {"x-inline-image",
           {UIImage::Make, InlinePlaceholderShadowNode::Make,
            LayoutNodeType::COMMON}},

          {"component", {UIComponent::Make}},
          {"list", {UIList::Make}},
          {"list-container", {UIList::Make}},
          {"list-item", {UIComponent::Make}},
          {"scroll-view", {UIScroll::Make}},
      }};
  return *kNodeTypeInfoMap;
}

LynxContext::~LynxContext() {
  DCHECK(!embedder_);
  DCHECK(!ui_owner_);
  DCHECK(!node_owner_);
  LOGD("~LynxContext");
}

void LynxContext::SetTapSlop(const std::string& tap_slop) {
  if (!ui_owner_) {
    return;
  }
  ui_owner_->SetTapSlop(tap_slop);
}

void LynxContext::SetHasTouchPseudo(bool has_touch_pseudo) {
  if (!ui_owner_) {
    return;
  }
  ui_owner_->SetHasTouchPseudo(has_touch_pseudo);
}

void LynxContext::SetLongPressDuration(int32_t long_press_duration) {
  if (!ui_owner_) {
    return;
  }
  ui_owner_->SetLongPressDuration(long_press_duration);
}

void LynxContext::SendEvent(const LynxEvent& event) const {
  if (!ui_owner_) {
    return;
  }
  ui_owner_->SendEvent(event);
}

void LynxContext::HandleTouchEvent(const TouchEvent& touch_event) const {
  float target_point[2] = {0}, page_point[2] = {0}, client_point[2] = {0};
  touch_event.GetTargetPoint(target_point);
  touch_event.GetPagePoint(page_point);
  touch_event.GetClientPoint(client_point);
  if (delegate_ && delegate_->touch_event_callback) {
    delegate_->touch_event_callback(
        touch_event.Name(), touch_event.ID(), target_point[0], target_point[1],
        client_point[0], client_point[1], page_point[0], page_point[1],
        delegate_->data);
    return;
  }
  if (!engine_proxy_) {
    return;
  }
  engine_proxy_->SendTouchEvent(
      touch_event.Name(), touch_event.ID(), target_point[0], target_point[1],
      client_point[0], client_point[1], page_point[0], page_point[1]);
}

void LynxContext::HandleMultiTouchEvent(const TouchEvent& touch_event) const {
  if (delegate_ && delegate_->multi_touch_event_callback) {
    float target_point[2] = {0}, page_point[2] = {0}, client_point[2] = {0};
    touch_event.GetTargetPoint(target_point);
    touch_event.GetPagePoint(page_point);
    touch_event.GetClientPoint(client_point);
    delegate_->multi_touch_event_callback(
        touch_event.Name(), touch_event.ID(), target_point[0], target_point[1],
        client_point[0], client_point[1], page_point[0], page_point[1],
        delegate_->data);
    return;
  }
  if (!engine_proxy_) {
    return;
  }
  engine_proxy_->SendTouchEvent(touch_event.Name(),
                                PubLepusValue(touch_event.UITouchMap()));
}

void LynxContext::HandleCustomEvent(const CustomEvent& custom_event) const {
  if (delegate_ && delegate_->custom_event_callback) {
    delegate_->custom_event_callback(custom_event.Name(), custom_event.ID(),
                                     PubLepusValue(custom_event.ParamValue()),
                                     custom_event.ParamName(), delegate_->data);
    return;
  }
  if (!engine_proxy_) {
    return;
  }
  engine_proxy_->SendCustomEvent(custom_event.Name(), custom_event.ID(),
                                 PubLepusValue(custom_event.ParamValue()),
                                 custom_event.ParamName());
}

void LynxContext::OnPseudoStatusChanged(int id, PseudoStatus pre_status,
                                        PseudoStatus current_status) const {
  if (!engine_proxy_) {
    return;
  }
  engine_proxy_->OnPseudoStatusChanged(id, static_cast<int32_t>(pre_status),
                                       static_cast<int32_t>(current_status));
}

void LynxContext::HandleGestureEvent(const GestureEvent& gesture_event) const {
  if (!engine_proxy_) {
    return;
  }
  engine_proxy_->SendGestureEvent(gesture_event.ID(), gesture_event.GestureId(),
                                  gesture_event.Name(),
                                  PubLepusValue(gesture_event.ParamValue()));
}

void LynxContext::SendGlobalEvent(lepus::Value params) const {
  if (delegate_ && delegate_->global_event_callback) {
    delegate_->global_event_callback(PubLepusValue(params), delegate_->data);
    return;
  }
  CallJSFunction("GlobalEventEmitter", "emit", std::move(params));
}

void LynxContext::CallJSApiCallbackWithValue(int32_t callback_id,
                                             const lepus::Value& params) const {
  if (!runtime_proxy_) {
    return;
  }
  runtime_proxy_->CallJSApiCallbackWithValue(
      callback_id,
      std::make_unique<pub::ValueImplLepus>(lepus::Value::ShallowCopy(params)));
}

void LynxContext::CallJSFunction(const std::string& module_id,
                                 const std::string& method_id,
                                 lepus::Value&& params) const {
  if (!runtime_proxy_) {
    return;
  }
  runtime_proxy_->CallJSFunction(
      module_id, method_id,
      std::make_unique<pub::ValueImplLepus>(
          lepus::Value::ShallowCopy(std::move(params))));
}

void LynxContext::CallJSIntersectionObserver(int32_t observer_id,
                                             int32_t callback_id,
                                             lepus::Value params) const {
  if (!runtime_proxy_) {
    return;
  }
  runtime_proxy_->CallJSIntersectionObserver(
      observer_id, callback_id,
      std::make_unique<pub::ValueImplLepus>(
          lepus::Value::ShallowCopy(std::move(params))));
}

float LynxContext::ScaledDensity() const {
  if (delegate_) {
    return delegate_->device_pixel_ratio;
  }
  std::shared_lock<std::shared_mutex> guard(embedder_shared_mutex_);
  if (!embedder_) {
    return 1.f;
  }
  return embedder_->DevicePixelRatio();
}

UIBase* LynxContext::FindUIBySign(int sign) const {
  if (!ui_owner_) {
    return nullptr;
  }
  return ui_owner_->FindUIBySign(sign);
}

UIBase* LynxContext::FindUIByIdSelector(const std::string& id_selector) const {
  if (!ui_owner_) {
    return nullptr;
  }
  return ui_owner_->FindUIByIdSelector(id_selector);
}

void LynxContext::InvokeUIMethod(
    const std::string& component_id, const std::string& node,
    const std::string& method, const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (!ui_owner_) {
    return;
  }
  ui_owner_->InvokeUIMethod(component_id, node, method, args,
                            std::move(callback));
}

ShadowNode* LynxContext::FindShadowNodeBySign(int sign) const {
  if (!node_owner_) {
    return nullptr;
  }
  return node_owner_->FindShadowNodeBySign(sign);
}

void LynxContext::FindShadowNodeAndRunTask(
    int sign, base::MoveOnlyClosure<void, ShadowNode*> task) const {
  if (!node_owner_) {
    return;
  }
  node_owner_->FindShadowNodeAndRunTask(sign, std::move(task));
}

void LynxContext::ScrollByListContainer(int32_t tag, float x, float y,
                                        float original_x, float original_y) {
  if (!engine_proxy_) {
    return;
  }
  engine_proxy_->ScrollByListContainer(tag, x, y, original_x, original_y);
}

void LynxContext::ScrollToPosition(int32_t tag, int index, float offset,
                                   int align, bool smooth) {
  if (!engine_proxy_) {
    return;
  }
  engine_proxy_->ScrollToPosition(tag, index, offset, align, smooth);
}

void LynxContext::ScrollStopped(int32_t tag) {
  if (!engine_proxy_) {
    return;
  }
  engine_proxy_->ScrollStopped(tag);
}

void LynxContext::AddUIToExposedMap(UIBase* ui, std::string unique_id,
                                    lepus::Value extra_data,
                                    bool is_custom_event) {
  if (!ui_owner_ || (unique_id.empty() && ui->ExposureID().empty())) {
    return;
  }
  ui_owner_->AddUIToExposedMap(ui, std::move(unique_id), std::move(extra_data),
                               is_custom_event);
}

void LynxContext::RemoveUIFromExposedMap(UIBase* ui, std::string unique_id) {
  if (!ui_owner_ || (unique_id.empty() && ui->ExposureID().empty())) {
    return;
  }
  ui_owner_->RemoveUIFromExposedMap(ui, std::move(unique_id));
}

void LynxContext::TriggerExposureCheck() {
  if (!ui_owner_) {
    return;
  }
  ui_owner_->TriggerExposureCheck();
}

void LynxContext::StopExposure(const lepus::Value& options) {
  if (!ui_owner_) {
    return;
  }
  ui_owner_->StopExposure(options);
}

void LynxContext::ResumeExposure() {
  if (!ui_owner_) {
    return;
  }
  ui_owner_->ResumeExposure();
}

void LynxContext::SetObserverFrameRate(const lepus::Value& options) {
  if (!ui_owner_) {
    return;
  }
  ui_owner_->SetObserverFrameRate(options);
}

void LynxContext::CreateUIIntersectionObserver(
    int intersection_observer_id, const std::string& js_component_id,
    const lepus::Value& options) {
  if (!ui_owner_) {
    return;
  }
  ui_owner_->CreateUIIntersectionObserver(intersection_observer_id,
                                          js_component_id, options);
}

UIIntersectionObserver* LynxContext::GetUIIntersectionObserver(
    int intersection_observer_id) {
  if (!ui_owner_) {
    return nullptr;
  }
  return ui_owner_->GetUIIntersectionObserver(intersection_observer_id);
}

void LynxContext::NotifyUIScroll() {
  if (!ui_owner_) {
    return;
  }
  ui_owner_->NotifyUIScroll();
}

void LynxContext::ScreenSize(float size[2]) const {
  if (delegate_) {
    size[0] = delegate_->screen_width;
    size[1] = delegate_->screen_height;
    return;
  }
  std::shared_lock<std::shared_mutex> guard(embedder_shared_mutex_);
  if (!embedder_) {
    return;
  }
  embedder_->ScreenSize(size);
}

void LynxContext::OnTouchEvent(const ArkUI_UIInputEvent* event, UIBase* root,
                               bool from_overlay) {
  if (!ui_owner_) {
    return;
  }
  ui_owner_->OnTouchEvent(event, root, from_overlay);
}

bool LynxContext::EventThrough() {
  if (!ui_owner_) {
    return false;
  }
  return ui_owner_->EventThrough();
}

bool LynxContext::ShouldBlockNativeEvent() {
  if (!ui_owner_) {
    return false;
  }
  return ui_owner_->ShouldBlockNativeEvent();
}

void LynxContext::AttachGesturesToRoot(UIBase* root) {
  if (!ui_owner_) {
    return;
  }
  ui_owner_->AttachGesturesToRoot(root);
}

void LynxContext::OnGestureRecognized(UIBase* ui) {
  if (!ui_owner_) {
    return;
  }
  ui_owner_->OnGestureRecognized(ui);
}

void LynxContext::OnGestureRecognizedWithSign(int sign) {
  if (!ui_owner_) {
    return;
  }
  ui_owner_->OnGestureRecognizedWithSign(sign);
}

void LynxContext::SetFocusedTarget(
    const std::weak_ptr<EventTarget>& focused_target) {
  if (!ui_owner_) {
    return;
  }
  ui_owner_->SetFocusedTarget(focused_target);
}

void LynxContext::UnsetFocusedTarget(
    const std::weak_ptr<EventTarget>& focused_target) {
  if (!ui_owner_) {
    return;
  }
  ui_owner_->UnsetFocusedTarget(focused_target);
}

void LynxContext::StartFluencyTrace(int sign, const std::string& scene,
                                    const std::string& tag) {
  if (!ui_owner_) {
    return;
  }
  ui_owner_->StartFluencyTrace(sign, scene, tag);
}

void LynxContext::StopFluencyTrace(int sign) {
  if (!ui_owner_) {
    return;
  }
  ui_owner_->StopFluencyTrace(sign);
}

int32_t LynxContext::GetInstanceId() const {
  return embedder_->GetInstanceId();
}

UIRoot* LynxContext::Root() {
  if (!ui_owner_) {
    return nullptr;
  }
  return ui_owner_->Root();
}

const std::string& LynxContext::OwnerId() {
  if (!ui_owner_) {
    static const base::NoDestructor<std::string> kEmpty;
    return *kEmpty;
  }
  return ui_owner_->Id();
}

void LynxContext::CreateNodeContent(UIBase* ui) const {
  if (!ui_owner_) {
    return;
  }
  return ui_owner_->CreateNodeContent(ui);
}

const std::shared_ptr<base::VSyncMonitor> LynxContext::VSyncMonitor() {
  if (ui_owner_) {
    return ui_owner_->VSyncMonitor();
  }
  return nullptr;
}

TextMeasureCache* LynxContext::GetTextMeasureCache() const {
  if (!node_owner_) {
    return nullptr;
  }
  return node_owner_->GetTextMeasureCache();
}

std::shared_ptr<FontFaceManager> LynxContext::GetFontFaceManager() const {
  std::shared_lock<std::shared_mutex> guard(node_owner_shared_mutex_);
  if (!node_owner_) {
    return nullptr;
  }
  return node_owner_->GetFontFaceManager();
}

void LynxContext::OnLynxCreate(
    const std::shared_ptr<shell::LynxEngineProxy>& engine_proxy,
    const std::shared_ptr<shell::LynxRuntimeProxy>& runtime_proxy,
    const std::shared_ptr<shell::PerfControllerProxy>& perf_controller_proxy,
    const std::shared_ptr<pub::LynxResourceLoader>& resource_loader,
    const fml::RefPtr<fml::TaskRunner>& ui_task_runner,
    const fml::RefPtr<fml::TaskRunner>& layout_task_runner) {
  engine_proxy_ = engine_proxy;
  runtime_proxy_ = runtime_proxy;
  perf_controller_proxy_ = perf_controller_proxy;
  resource_loader_ = resource_loader;
  ui_task_runner_ = ui_task_runner;
  layout_task_runner_ = layout_task_runner;
  static_cast<lynx::harmony::LynxResourceLoaderHarmony*>(resource_loader_.get())
      ->SetUITaskRunner(ui_task_runner_);
}

void LynxContext::PostTaskOnUIThread(base::closure task) const {
  if (!ui_task_runner_) {
    return;
  }
  ui_task_runner_->PostTask(std::move(task));
}

void LynxContext::RunOnUIThread(base::closure task) const {
  if (!ui_task_runner_) {
    return;
  }
  fml::TaskRunner::RunNowOrPostTask(ui_task_runner_, std::move(task));
}

void LynxContext::RunOnLayoutThread(base::closure task) const {
  if (!layout_task_runner_) {
    return;
  }
  fml::TaskRunner::RunNowOrPostTask(layout_task_runner_, std::move(task));
}

void LynxContext::PostSyncTaskOnUIThread(base::closure task) const {
  if (!ui_task_runner_) {
    return;
  }
  ui_task_runner_->PostSyncTask(std::move(task));
}

const LynxContext::NodeInfo* LynxContext::GetNodeInfo(
    const std::string& node_name) {
  auto& static_map = GetCAPINodeInfoMap();
  if (auto iter = static_map.find(node_name); iter != static_map.end()) {
    return &iter->second;
  }

  if (auto iter = dynamic_node_info_map_.find(node_name);
      iter != dynamic_node_info_map_.end()) {
    return &iter->second;
  }

  return nullptr;
}

void LynxContext::RegisterNodeInfo(const std::string& node_name,
                                   NodeInfo node_info) {
  if (dynamic_node_info_map_.find(node_name) == dynamic_node_info_map_.end()) {
    dynamic_node_info_map_[node_name] = node_info;
  }
}

fluency::harmony::FluencyTraceHelperHarmony&
LynxContext::GetFluencyTraceHelper() {
  return fluency_trace_helper_;
}

napi_env LynxContext::GetNapiEnv() const {
  if (embedder_) {
    return embedder_->GetNapiEnv();
  }
  return nullptr;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
