// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

#include <arkui/native_node_napi.h>

#include <memory>
#include <string>
#include <utility>

#include "base/include/platform/harmony/napi_util.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/base/harmony/napi_convert_helper.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "core/services/fluency/fluency_tracer.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/touch_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/arena/gesture_arena_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/native_node_content.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/js_ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_flatten_image.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_image.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_list.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_root.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_scroll.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_text.h"

namespace lynx {
namespace tasm {
namespace harmony {

napi_value UIOwner::Init(napi_env env, napi_value exports) {
#define DECLARE_NAPI_FUNCTION(name, func) \
  {(name), nullptr, (func), nullptr, nullptr, nullptr, napi_default, nullptr}

  napi_property_descriptor properties[] = {
      DECLARE_NAPI_FUNCTION("attachPageRoot", AttachPageRoot),
      DECLARE_NAPI_FUNCTION("getId", GetId),
      DECLARE_NAPI_FUNCTION("destroy", Destroy),
      DECLARE_NAPI_FUNCTION("requestLayout", RequestLayout),
      DECLARE_NAPI_FUNCTION("keyboardStatusChanged", KeyboardStatusChanged),

      DECLARE_NAPI_FUNCTION("canConsumeTouchEvent", CanConsumeTouchEvent),
  };
#undef DECLARE_NAPI_FUNCTION

  napi_value cons;
  napi_define_class(env, "UIOwner", NAPI_AUTO_LENGTH, Constructor, nullptr,
                    sizeof(properties) / sizeof(properties[0]), properties,
                    &cons);

  napi_set_named_property(env, exports, "UIOwner", cons);
  return exports;
}

UIBase* UIOwner::CreateJSUI(int sign, const std::string& tag) {
  base::NapiHandleScope scope(env_);
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OWNER_CREATE_JS_UI, "tag", tag);
  napi_value js_recv = base::NapiUtil::GetReferenceNapiValue(env_, js_this_);
  napi_value create = base::NapiUtil::GetReferenceNapiValue(env_, js_create_);
  if (!js_recv || !create) {
    return nullptr;
  }
  size_t argc = 3;
  /**
   * 0 - context ptr array
   * 1 - tag string
   * 2 - sign int
   */
  napi_value argv[argc];
  argv[0] = base::NapiUtil::CreatePtrArray(
      env_, reinterpret_cast<uintptr_t>(context_.get()));

  napi_create_string_latin1(env_, tag.data(), NAPI_AUTO_LENGTH, &argv[1]);
  napi_create_int32(env_, sign, &argv[2]);

  napi_value result;
  napi_call_function(env_, js_recv, create, argc, argv, &result);
  napi_valuetype type;
  napi_typeof(env_, result, &type);
  if (type == napi_undefined) {
    return nullptr;
  }
  JSUIBase* node{nullptr};
  napi_unwrap(env_, result, reinterpret_cast<void**>(&node));
  return node;
}

void UIOwner::CreateUI(int sign, const std::string& tag,
                       PropBundleHarmony* painting_data, uint32_t node_index) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OWNER_CREATE_UI + tag);
  UIBase* ui = nullptr;
  if ((tag == "image") && (painting_data->Contains("autoplay") ||
                           painting_data->Contains("loop-count"))) {
    ui = UIFlattenImage::Make(context_.get(), sign, tag);
  } else if (auto* node_info = context_->GetNodeInfo(tag);
             node_info && node_info->ui_creator) {
    ui = node_info->ui_creator(context_.get(), sign, tag);
  } else if (tag == "page") {
    ui = Root();
  } else {
    ui = CreateJSUI(sign, tag);
  }

  if (ui == nullptr) {
    ui = UIView::Make(context_.get(), sign, tag);
  }

  ui_holder_[sign] = ui->IsRoot() ? root_ : std::shared_ptr<UIBase>(ui);
  UpdateComponentIdMap(ui, painting_data);
  ui->UpdateProps(painting_data);
  const auto& events = painting_data->GetEvents();
  if (events) {
    ui->SetEvents(events.value());
  }

  const auto& gestures = painting_data->GetGestureDetectors();
  if (gestures) {
    ui->SetGestureDetectors(gestures.value());
  }
  MarkHasUIOperationsBottomUp(ui);

  // XXX: Move the tag to builder's map to static map or gperf.
}

void UIOwner::InsertUI(int parent, int child, int index) {
  const auto& parent_it = ui_holder_.find(parent);
  if (parent_it == ui_holder_.end()) {
    return;
  }
  const auto& child_it = ui_holder_.find(child);
  if (child_it == ui_holder_.end()) {
    return;
  }
  const auto& parent_ui = parent_it->second;
  const auto& child_ui = child_it->second;
  MarkHasUIOperations(parent_ui.get());
  parent_ui->AddChild(child_ui.get(), index);
}

void UIOwner::RemoveUI(int parent, int child, int index, bool is_move) {
  const auto& child_it = ui_holder_.find(child);
  if (child_it == ui_holder_.end()) {
    return;
  }
  MarkHasUIOperationsBottomUp(child_it->second.get());
  child_it->second->RemoveFromParent();
}

void UIOwner::UpdateUI(int sign, PropBundleHarmony* props) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OWNER_UPDATE_UI);
  if (const auto& it = ui_holder_.find(sign); it != ui_holder_.end()) {
    UIBase* ui = it->second.get();
    UpdateComponentIdMap(ui, props);

    ui->UpdateProps(props);
    MarkHasUIOperations(ui);
    const auto& events = props->GetEvents();
    if (events) {
      ui->SetEvents(events.value());
    }
    const auto& gestures = props->GetGestureDetectors();
    if (gestures) {
      ui->SetGestureDetectors(gestures.value());
    }

    ui_observer_->NotifyUIPropsChange();
  }
}

void UIOwner::DestroyUI(int parent, int child, int index) {
  const auto& child_it = ui_holder_.find(child);
  if (child_it == ui_holder_.end()) {
    return;
  }
  UIBase* ui = child_it->second.get();
  MarkHasUIOperationsBottomUp(ui);
  DestroySubTree(ui);
}

void UIOwner::OnNodeReady(int sign) {
  if (auto it = ui_holder_.find(sign); it != ui_holder_.end()) {
    auto* ui = it->second.get();
    TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OWNER_ON_NODE_READY,
                [ui](lynx::perfetto::EventContext ctx) {
                  ctx.event()->add_debug_annotations("tag", ui->Tag());
                });
    ui->OnNodeReady();
  }
}

void UIOwner::DestroySubTree(UIBase* root) {
  for (auto* child : root->Children()) {
    DestroySubTree(child);
  }
  root->OnDestroy();
  root->RemoveFromParent();
  AddOrRemoveUIFromExclusiveSet(root->Sign(), false);
  keyboard_event_observers_.erase(root->Sign());
  ui_holder_.erase(root->Sign());
}

UIRoot* UIOwner::Root() {
  if (!root_) {
    root_ = std::shared_ptr<UIBase>(UIRoot::Make(context_.get(), 10, "page"));
  }
  return reinterpret_cast<UIRoot*>(root_.get());
}

void UIOwner::CreateNodeContent(UIBase* ui) const {
  base::NapiHandleScope scope(env_);
  napi_value js_recv = base::NapiUtil::GetReferenceNapiValue(env_, js_this_);
  napi_value create =
      base::NapiUtil::GetReferenceNapiValue(env_, js_create_node_content_);
  if (!js_recv || !create) {
    return;
  }
  size_t argc = 0;

  napi_value node_content{nullptr};
  napi_call_function(env_, js_recv, create, argc, nullptr, &node_content);
  NativeNodeContent* node;
  napi_unwrap(env_, node_content, reinterpret_cast<void**>(&node));
  ui->AttachToNodeContent(node);
}

void UIOwner::UpdateLayout(int sign, float left, float top, float width,
                           float height, const float* paddings,
                           const float* margins, const float* sticky,
                           float max_height, uint32_t node_index) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OWNER_UPDATE_LAYOUT);
  if (const auto& it = ui_holder_.find(sign); it != ui_holder_.end()) {
    UIBase* ui = it->second.get();
    ui->UpdateLayout(left, top, width, height, paddings, margins, sticky,
                     max_height, node_index);
    ui->UpdateSticky(sticky);
    ui_observer_->NotifyUILayout();
    MarkHasUIOperationsBottomUp(ui);
  }
}

void UIOwner::OnLayoutFinish(int32_t component_id, int64_t operation_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OWNER_ON_LAYOUT_FINISH);

  for (const auto& layout_changed_node : layout_changed_nodes_) {
    const auto ui = layout_changed_node.second.lock();
    if (ui != nullptr) {
      ui->FinishLayoutOperation();
    }
  }
  layout_changed_nodes_.clear();

  // For `<list>`
  if (operation_id == 0) {
    return;
  }
  int list_id = static_cast<int32_t>(operation_id >> 32);

  auto list_iterator = ui_holder_.find(list_id);
  auto child_iterator = ui_holder_.find(component_id);
  if (list_iterator == ui_holder_.end() || child_iterator == ui_holder_.end()) {
    return;
  }
  UIBase* ui_base = list_iterator->second.get();
  if (ui_base && ui_base->IsList()) {
    UIList* list_view = static_cast<UIList*>(ui_base);
    UIBase* child = child_iterator->second.get();
    list_view->OnLayoutFinish(child, operation_id);
  }
}

void UIOwner::UpdateContentOffsetForListContainer(int32_t list_sign,
                                                  float content_size,
                                                  float delta_x, float delta_y,
                                                  bool is_init_scroll_offset,
                                                  bool from_layout) {
  const auto it = ui_holder_.find(list_sign);
  if (it != ui_holder_.end()) {
    UIBase* ui_list = it->second.get();
    if (ui_list && ui_list->IsList()) {
      UIList* list_view = static_cast<UIList*>(ui_list);
      list_view->UpdateContentSizeAndOffset(content_size, delta_x, delta_y,
                                            is_init_scroll_offset, from_layout);
    }
  }
}

void UIOwner::UpdateScrollInfo(int32_t list_sign, bool smooth,
                               float estimated_offset, bool scrolling) {
  const auto it = ui_holder_.find(list_sign);
  if (it != ui_holder_.end()) {
    UIBase* ui_base = it->second.get();
    if (ui_base && ui_base->IsList()) {
      UIList* list_view = static_cast<UIList*>(ui_base);
      list_view->UpdateScrollInfo(smooth, estimated_offset, scrolling);
    }
  }
}

void UIOwner::InsertListItemPaintingNode(int list_sign, int child_sign) {
  const auto list_iterator = ui_holder_.find(list_sign);
  const auto child_iterator = ui_holder_.find(child_sign);

  if (list_iterator == ui_holder_.end() || child_iterator == ui_holder_.end()) {
    return;
  }
  UIBase* ui_base = list_iterator->second.get();
  if (ui_base && ui_base->IsList()) {
    UIList* ui_list = static_cast<UIList*>(ui_base);
    UIComponent* child =
        static_cast<UIComponent*>(child_iterator->second.get());
    ui_list->InsertListItemNode(child);
  }
}

void UIOwner::RemoveListItemPaintingNode(int list_sign, int child_sign) {
  const auto list_iterator = ui_holder_.find(list_sign);
  const auto child_iterator = ui_holder_.find(child_sign);

  if (list_iterator == ui_holder_.end() || child_iterator == ui_holder_.end()) {
    return;
  }
  UIBase* ui_base = list_iterator->second.get();
  if (ui_base && ui_base->IsList()) {
    UIList* list_view = static_cast<UIList*>(ui_base);
    UIComponent* child =
        static_cast<UIComponent*>(child_iterator->second.get());
    list_view->RemoveListItemNode(child);
  }
}

UIOwner::~UIOwner() = default;

void UIOwner::AttachPageRoot(NativeNodeContent* content) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OWNER_ATTACH_PAGE_ROOT);
  Root()->AttachToNodeContent(content);
}

void UIOwner::UpdateExtraData(
    int sign, const fml::RefPtr<fml::RefCountedThreadSafeStorage>& extra_data) {
  if (auto it = ui_holder_.find(sign); it != ui_holder_.end()) {
    it->second->UpdateExtraData(extra_data);
  }
}

int32_t UIOwner::GetTagInfo(const std::string& tag) const {
  int node_type;

  auto* node_info = context_->GetNodeInfo(tag);
  if (node_info) {
    node_type = node_info->node_type;
  } else {
    auto ret = GetJSNodeType(0, tag);
    node_type = ret != 0 ? ret : LayoutNodeType::COMMON;
  }
  return node_type;
}

void UIOwner::SetGestureDetectorState(int64_t id, int32_t gesture_id,
                                      int32_t state) const {
  const auto it = ui_holder_.find(id);
  if (it != ui_holder_.end()) {
    UIBase* ui_base = it->second.get();
    ui_base->SetGestureDetectorState(gesture_id, state);
  }
}

UIBase* UIOwner::FindUIBySign(int sign) const {
  if (const auto it = ui_holder_.find(sign); it != ui_holder_.end()) {
    return it->second.get();
  }
  return nullptr;
}

UIBase* UIOwner::FindUIByIdSelector(const std::string& id_selector) const {
  if (id_selector.empty()) {
    return nullptr;
  }
  for (const auto it : ui_holder_) {
    if (it.second->IdSelector() == id_selector) {
      return it.second.get();
    }
  }
  return nullptr;
}

UIOwner::UIOwner() {
  id_ = "lynx-" + std::to_string(reinterpret_cast<uintptr_t>(this)) + "-";
}

napi_value UIOwner::Constructor(napi_env env, napi_callback_info info) {
  napi_value js_object;
  UIOwner* owner = new UIOwner();
  /**
   * 0 - js object ref
   * 1 - create function ref
   * 2 - uiContext ref
   * 3 - create NodeContent ref
   * 4 - startFluencyTrace function ref
   * 5 - stopFluencyTrace function ref
   * 6 - getTagInfo ref
   * 7 - postDrawEndTimingFrameCallback function ref
   * 8 - onAvoidKeyboardCallback function ref
   */
  size_t argc = 9;
  napi_value argv[argc];
  owner->env_ = env;
  napi_get_cb_info(env, info, &argc, argv, &js_object, nullptr);
  napi_create_reference(env, argv[0], 0, &owner->js_this_);
  napi_create_reference(env, argv[1], 0, &owner->js_create_);
  OH_ArkUI_GetContextFromNapiValue(env, argv[2], &owner->ark_ui_context_);
  napi_create_reference(env, argv[3], 0, &owner->js_create_node_content_);
  napi_create_reference(env, argv[4], 0, &owner->js_start_fluency_trace_);
  napi_create_reference(env, argv[5], 0, &owner->js_stop_fluency_trace_);
  napi_create_reference(env, argv[6], 0, &owner->js_get_node_type_);
  napi_create_reference(env, argv[7], 0,
                        &owner->post_draw_end_timing_frame_callback_);
  napi_create_reference(env, argv[8], 0, &owner->on_avoid_keyboard_callback_);
  napi_wrap(
      env, js_object, owner,
      [](napi_env env, void* data, void* hint) -> void {}, nullptr, nullptr);
  return js_object;
}

napi_value UIOwner::AttachPageRoot(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[argc];
  napi_value js_this;
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  UIOwner* owner = nullptr;
  napi_unwrap(env, js_this, reinterpret_cast<void**>(&owner));
  if (!owner) {
    return nullptr;
  }
  NativeNodeContent* native_node_content;
  napi_unwrap(env, argv[0], reinterpret_cast<void**>(&native_node_content));
  owner->AttachPageRoot(native_node_content);
  return nullptr;
}

napi_value UIOwner::GetId(napi_env env, napi_callback_info info) {
  UIOwner* owner = nullptr;
  napi_value js_this = nullptr;
  size_t argc = 0;
  napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);
  napi_unwrap(env, js_this, reinterpret_cast<void**>(&owner));
  if (!owner) {
    return nullptr;
  }
  napi_value id;
  napi_create_string_latin1(env, owner->Id().data(), owner->Id().length(), &id);
  return id;
}

napi_value UIOwner::Destroy(napi_env env, napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value argv[argc];
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);

  UIOwner* obj = nullptr;
  napi_status status =
      napi_remove_wrap(env, js_this, reinterpret_cast<void**>(&obj));
  NAPI_THROW_IF_FAILED_NULL(env, status, "UIOwner napi_remove_wrap failed!");
  obj->context_->ResetUIOwner();
  obj->accessibility_exclusive_.clear();
  obj->ui_holder_.clear();
  obj->layout_changed_nodes_.clear();
  obj->keyboard_event_observers_.clear();
  obj->root_ = nullptr;
  napi_delete_reference(env, obj->js_create_);
  napi_delete_reference(env, obj->js_create_node_content_);
  napi_delete_reference(env, obj->js_get_node_type_);
  napi_delete_reference(env, obj->post_draw_end_timing_frame_callback_);
  napi_delete_reference(env, obj->on_avoid_keyboard_callback_);
  napi_delete_reference(env, obj->js_this_);
  obj->js_create_ = nullptr;
  obj->js_create_node_content_ = nullptr;
  obj->js_get_node_type_ = nullptr;
  obj->post_draw_end_timing_frame_callback_ = nullptr;
  obj->on_avoid_keyboard_callback_ = nullptr;
  obj->js_this_ = nullptr;
  obj->env_ = nullptr;
  obj->destroyed_ = true;
  return nullptr;
}

napi_value UIOwner::CanConsumeTouchEvent(napi_env env,
                                         napi_callback_info info) {
  UIOwner* owner = nullptr;
  napi_value js_this = nullptr;
  size_t argc = 2;
  napi_value argv[argc];
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  napi_unwrap(env, js_this, reinterpret_cast<void**>(&owner));
  if (!owner) {
    return nullptr;
  }

  float point[2] = {0.f};
  double temp_value = 0.0;
  napi_get_value_double(env, argv[0], &temp_value);
  point[0] = static_cast<float>(temp_value);
  napi_get_value_double(env, argv[1], &temp_value);
  point[1] = static_cast<float>(temp_value);

  napi_value consumed_touch_event;
  napi_get_boolean(env, owner->CanConsumeTouchEvent(point),
                   &consumed_touch_event);
  return consumed_touch_event;
}

bool UIOwner::CanConsumeTouchEvent(float point[2]) {
  return event_dispatcher_->CanConsumeTouchEvent(point);
}

void UIOwner::InvokeUIMethod(int32_t id, const std::string& method,
                             PropBundleHarmony* args, int32_t callback_id) {
  if (const auto it = ui_holder_.find(id); it != ui_holder_.end()) {
    auto map = lepus::Dictionary::Create();
    for (const auto& p : args->GetProps()) {
      map->SetValue(p.first, p.second);
    }
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback =
        [context = std::weak_ptr(context_), callback_id](
            int32_t code, const lepus::Value& data) {
          auto ctx = context.lock();
          if (!ctx) {
            return;
          }
          auto ret = lepus::Dictionary::Create();
          ret->SetValue("code", code);
          ret->SetValue("data", data);
          ctx->CallJSApiCallbackWithValue(callback_id, lepus::Value(ret));
        };
    it->second->InvokeMethod(method, lepus::Value(map), std::move(callback));
  }
}

UIBase* UIOwner::FindLynxUIByComponentId(const std::string& component_id) {
  if (component_id.empty() || component_id == kDefaultComponentId) {
    return root_.get();
  }
  int32_t sign = kLynxRootSign;
  if (const auto& it = component_map_.find(component_id);
      it != component_map_.end()) {
    sign = it->second;
  }

  if (sign == kLynxRootSign) {
    return root_.get();
  }
  return FindUIBySign(sign);
}

void UIOwner::InvokeUIMethod(
    const std::string& component_id, const std::string& node,
    const std::string& method, const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  bool by_ref_id = false;
  if (args.IsTable()) {
    const auto& param = args.Table();
    by_ref_id = param->size() != 0 && param->GetValue("_isCallByRefId").Bool();
  }
  auto component = FindLynxUIByComponentId(component_id);
  UIBase* ui = nullptr;
  if (component) {
    ui = component->FindViewById(node, by_ref_id);
  }
  if (!ui) {
    callback(LynxGetUIResult::NODE_NOT_FOUND,
             lepus::Value("ui not found for: " + node));
  } else {
    ui->InvokeMethod(method, args, std::move(callback));
  }
}

void UIOwner::InvokeUIMethod(
    int32_t id, const std::string& method, const pub::Value& args,
    base::MoveOnlyClosure<void, int32_t, const pub::Value&> callback) {
  if (const auto it = ui_holder_.find(id); it != ui_holder_.end()) {
    const auto& map = pub::ValueUtils::ConvertValueToLepusValue(args);
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> cb =
        [callback = std::move(callback)](int32_t code,
                                         const lepus::Value& data) {
          callback(code, PubLepusValue(data));
        };

    if (map.IsJSValue()) {
      it->second->InvokeMethod(method, map.ToLepusValue(), std::move(cb));
    } else {
      it->second->InvokeMethod(method, map, std::move(cb));
    }
  }
}

void UIOwner::AddUIToExposedMap(UIBase* ui, std::string unique_id,
                                lepus::Value extra_data, bool is_custom_event) {
  ui_observer_->AddUIToExposedMap(ui, std::move(unique_id),
                                  std::move(extra_data), is_custom_event);
}

void UIOwner::RemoveUIFromExposedMap(UIBase* ui, std::string unique_id) {
  ui_observer_->RemoveUIFromExposedMap(ui, std::move(unique_id));
}

void UIOwner::TriggerExposureCheck() { ui_observer_->TriggerExposureCheck(); }

void UIOwner::StopExposure(const lepus::Value& options) {
  ui_observer_->StopExposure(options);
}

void UIOwner::ResumeExposure() { ui_observer_->ResumeExposure(); }

void UIOwner::SetObserverFrameRate(const lepus::Value& options) {
  ui_observer_->SetObserverFrameRate(options);
}

void UIOwner::CreateUIIntersectionObserver(int intersection_observer_id,
                                           const std::string& js_component_id,
                                           const lepus::Value& options) {
  ui_observer_->CreateUIIntersectionObserver(intersection_observer_id,
                                             js_component_id, options);
}

UIIntersectionObserver* UIOwner::GetUIIntersectionObserver(
    int intersection_observer_id) {
  return ui_observer_->GetUIIntersectionObserver(intersection_observer_id);
}

void UIOwner::NotifyUIScroll() { ui_observer_->NotifyUIScroll(); }

void UIOwner::OnTouchEvent(const ArkUI_UIInputEvent* event, UIBase* root,
                           bool from_overlay) {
  event_dispatcher_->OnTouchEvent(event, root, from_overlay);
}

bool UIOwner::EventThrough() { return event_dispatcher_->EventThrough(); }

bool UIOwner::ShouldBlockNativeEvent() {
  return event_dispatcher_->ShouldBlockNativeEvent();
}

void UIOwner::SetFocusedTarget(
    const std::weak_ptr<EventTarget>& focused_target) {
  event_dispatcher_->SetFocusedTarget(focused_target);
}

void UIOwner::UnsetFocusedTarget(
    const std::weak_ptr<EventTarget>& focused_target) {
  event_dispatcher_->UnsetFocusedTarget(focused_target);
}

void UIOwner::AttachGesturesToRoot(UIBase* root) {
  event_dispatcher_->AttachGesturesToRoot(root);
}

void UIOwner::OnGestureRecognized(UIBase* ui) {
  event_dispatcher_->OnGestureRecognized(ui);
}

void UIOwner::OnGestureRecognizedWithSign(int sign) {
  event_dispatcher_->OnGestureRecognizedWithSign(sign);
}

void UIOwner::SetTapSlop(const std::string& tap_slop) {
  event_dispatcher_->SetTapSlop(tap_slop);
}

void UIOwner::SetHasTouchPseudo(bool has_touch_pseudo) {
  event_dispatcher_->SetHasTouchPseudo(has_touch_pseudo);
}

void UIOwner::SetLongPressDuration(int32_t long_press_duration) {
  event_dispatcher_->SetLongPressDuration(long_press_duration);
}

void UIOwner::SendEvent(const LynxEvent& event) const {
  event_emitter_->SendEvent(event);
}

void UIOwner::HandleTouchEvent(const TouchEvent& touch_event) const {
  context_->HandleTouchEvent(touch_event);
}

void UIOwner::HandleMultiTouchEvent(const TouchEvent& touch_event) const {
  context_->HandleMultiTouchEvent(touch_event);
}

void UIOwner::HandleCustomEvent(const CustomEvent& custom_event) const {
  context_->HandleCustomEvent(custom_event);
}

void UIOwner::SendPseudoStatusEvent(int id, PseudoStatus pre_status,
                                    PseudoStatus current_status) const {
  event_emitter_->SendPseudoStatusEvent(id, pre_status, current_status);
}

void UIOwner::OnPseudoStatusChanged(int id, PseudoStatus pre_status,
                                    PseudoStatus current_status) const {
  context_->OnPseudoStatusChanged(id, pre_status, current_status);
}

void UIOwner::ConsumeGesture(int64_t id, int32_t gesture_id,
                             const lepus::Value& param) const {
  const auto it = ui_holder_.find(id);
  if (it != ui_holder_.end()) {
    UIBase* ui_base = it->second.get();
    ui_base->ConsumeGesture(gesture_id, param);
  }
}

void UIOwner::SendGlobalEvent(lepus::Value params) const {
  context_->SendGlobalEvent(std::move(params));
}

std::shared_ptr<GestureArenaManager> UIOwner::GetGestureArenaManager() const {
  return gesture_arena_manager_;
}

void UIOwner::DispatchTouchEventToGestureArena(
    std::string type, std::shared_ptr<TouchEvent> touch_event,
    const ArkUI_UIInputEvent* event) const {
  if (gesture_arena_manager_) {
    gesture_arena_manager_->DispatchTouchEventToArena(event, touch_event);
  }
}

void UIOwner::SetActiveUIToGestureArenaAtDownEvent(
    std::weak_ptr<EventTarget> target) {
  if (gesture_arena_manager_) {
    gesture_arena_manager_->SetActiveUIToArenaAtDownEvent(target);
  }
}

void UIOwner::SetVelocityToGestureArena(float velocity_x, float velocity_y) {
  if (gesture_arena_manager_) {
    gesture_arena_manager_->SetVelocity(velocity_x, velocity_y);
  }
}

void UIOwner::InitGestureArenaManager(LynxContext* context) {
  gesture_arena_manager_ = std::make_shared<GestureArenaManager>(true, context);
}

void UIOwner::PostTaskOnUIThread(base::closure task) const {
  context_->PostTaskOnUIThread(std::move(task));
}

const fml::RefPtr<fml::TaskRunner>& UIOwner::GetUITaskRunner() const {
  return context_->GetUITaskRunner();
}

const std::shared_ptr<base::VSyncMonitor>& UIOwner::VSyncMonitor() {
  if (!vsync_monitor_) {
    const auto& ui_task_runner = GetUITaskRunner();
    auto task = [this, ui_task_runner]() {
      vsync_monitor_ = base::VSyncMonitor::Create();
      vsync_monitor_->BindTaskRunner(ui_task_runner);
      vsync_monitor_->Init();
    };
    if (ui_task_runner) {
      ui_task_runner->PostSyncTask([task = std::move(task)]() { task(); });
    } else {
      task();
    }
  }

  return vsync_monitor_;
}

void UIOwner::MarkHasUIOperations(UIBase* ui) {
  if (ui->IsRoot()) {
    return;
  }
  layout_changed_nodes_[ui->Sign()] = ui->weak_from_this();
}

void UIOwner::MarkHasUIOperationsBottomUp(UIBase* ui) {
  if (ui->IsRoot()) {
    return;
  }
  UIBase* current = ui;
  if (current->NotifyParent()) {
    while (current != nullptr && !current->IsRoot()) {
      layout_changed_nodes_[current->Sign()] = current->weak_from_this();
      current = current->Parent();
    }
  } else {
    layout_changed_nodes_[ui->Sign()] = ui->weak_from_this();
  }
}

napi_value UIOwner::RequestLayout(napi_env env, napi_callback_info info) {
  napi_value js_this;
  size_t argc = 0;
  napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);

  UIOwner* obj = nullptr;
  napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!obj) {
    return nullptr;
  }
  obj->RequestLayout();
  return nullptr;
}

napi_value UIOwner::KeyboardStatusChanged(napi_env env,
                                          napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

  auto height = base::NapiUtil::ConvertToFloat(env, args[0]);

  UIOwner* obj = nullptr;
  napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
  if (!obj) {
    return nullptr;
  }

  bool is_show = height > 0;

  for (const auto& observer : obj->keyboard_event_observers_) {
    const auto ui = observer.second.lock();
    if (ui != nullptr) {
      if (is_show) {
        ui->OnKeyboardWillShow(height);
      } else {
        ui->OnKeyboardWillHide();
      }
    }
  }
  return nullptr;
}

void UIOwner::AddKeyboardEventObserver(int32_t sign) {
  if (const auto it = ui_holder_.find(sign); it != ui_holder_.end()) {
    std::weak_ptr<UIBase> weak_ptr(it->second);
    keyboard_event_observers_[sign] = weak_ptr;
  }
}

void UIOwner::RequestLayout() {
  auto* root = reinterpret_cast<UIRoot*>(root_.get());
  if (!root) {
    return;
  }
  NodeManager::Instance().RequestLayout(root->GetProxyNode());
}

void UIOwner::UpdateComponentIdMap(UIBase* ui,
                                   PropBundleHarmony* painting_data) {
  if (ui->Tag() != "component") {
    return;
  }
  ui->SetIsComponent(true);
  const auto& props = painting_data->GetProps();
  if (auto it = props.find("ComponentID"); it != props.end()) {
    const lepus::Value& v = it->second;
    std::string component_id;
    if (v.IsInt32()) {
      component_id = std::to_string(v.Int32());
    } else if (v.IsString()) {
      component_id = v.StdString();
    } else {
      component_id = kDefaultComponentId;
    }
    component_map_[component_id] = ui->Sign();
  }
}

void UIOwner::StartFluencyTrace(int sign, const std::string& scene,
                                const std::string& tag) const {
  base::NapiHandleScope scope(env_);
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OWNER_START_FLUENCY_TRACE,
              [&scene, &tag](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("scene", scene);
                ctx.event()->add_debug_annotations("tag", tag);
              });
  auto& fluency_trace_helper = context_->GetFluencyTraceHelper();
  if (!fluency_trace_helper.ShouldSendAllScrollEvent()) {
    return;
  }
  napi_value js_recv = base::NapiUtil::GetReferenceNapiValue(env_, js_this_);
  napi_value start =
      base::NapiUtil::GetReferenceNapiValue(env_, js_start_fluency_trace_);
  if (!js_recv || !start) {
    return;
  }

  size_t argc = 5;
  /**
   * 0 - sign int
   * 1 - scene string
   * 2 - tag string
   * 3 - instanceId int
   * 4 - the value of enableLynxScrollFluency in pageconfig double
   */
  napi_value argv[argc];
  napi_create_int32(env_, sign, &argv[0]);
  napi_create_string_latin1(env_, scene.data(), NAPI_AUTO_LENGTH, &argv[1]);
  napi_create_string_latin1(env_, tag.data(), NAPI_AUTO_LENGTH, &argv[2]);
  napi_create_int32(env_, context_->GetInstanceId(), &argv[3]);
  napi_create_double(env_, fluency_trace_helper.GetPageConfigProbability(),
                     &argv[4]);

  napi_value result;
  napi_call_function(env_, js_recv, start, argc, argv, &result);
}

void UIOwner::StopFluencyTrace(int sign) const {
  base::NapiHandleScope scope(env_);
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_OWNER_STOP_FLUENCY_TRACE);
  if (!context_->GetFluencyTraceHelper().ShouldSendAllScrollEvent()) {
    return;
  }
  napi_value js_recv = base::NapiUtil::GetReferenceNapiValue(env_, js_this_);
  napi_value stop =
      base::NapiUtil::GetReferenceNapiValue(env_, js_stop_fluency_trace_);
  if (!js_recv || !stop) {
    return;
  }

  size_t argc = 1;
  /**
   * 0 - sign int
   */
  napi_value argv[argc];
  napi_create_int32(env_, sign, &argv[0]);

  napi_value result;
  napi_call_function(env_, js_recv, stop, argc, argv, &result);
}

int UIOwner::GetJSNodeType(int sign, const std::string& tag) const {
  auto runnable = [this, &tag]() -> int {
    // napi_ref could be reset to nullptr on ui thread.;

    if (!js_this_) {
      return 0;
    }
    base::NapiHandleScope scope(env_);
    napi_value js_recv = base::NapiUtil::GetReferenceNapiValue(env_, js_this_);
    napi_value get_node_type_func =
        base::NapiUtil::GetReferenceNapiValue(env_, js_get_node_type_);
    if (!js_recv || !get_node_type_func) {
      return 0;
    }

    size_t argc = 1;
    napi_value argv[argc];
    napi_create_string_latin1(env_, tag.data(), NAPI_AUTO_LENGTH, &argv[0]);
    napi_value result;
    napi_call_function(env_, js_recv, get_node_type_func, argc, argv, &result);
    napi_valuetype type;
    napi_typeof(env_, result, &type);
    if (type == napi_undefined) {
      return 0;
    }
    int32_t node_type;
    napi_get_value_int32(env_, result, &node_type);
    return node_type;
  };

  int result;
  context_->PostSyncTaskOnUIThread(
      [&result, runnable = std::move(runnable)]() { result = runnable(); });
  return result;
}

void UIOwner::PostDrawEndTimingFrameCallback(
    const tasm::PipelineID& pipeline_id) const {
  base::NapiHandleScope scope(env_);
  napi_value js_recv = base::NapiUtil::GetReferenceNapiValue(env_, js_this_);
  napi_value post_draw_end_timing_frame_callback =
      base::NapiUtil::GetReferenceNapiValue(
          env_, post_draw_end_timing_frame_callback_);
  if (!js_recv || !post_draw_end_timing_frame_callback) {
    return;
  }
  size_t argc = 1;
  napi_value argv[argc];
  napi_create_string_latin1(env_, pipeline_id.data(), NAPI_AUTO_LENGTH,
                            &argv[0]);

  napi_value result;
  napi_call_function(env_, js_recv, post_draw_end_timing_frame_callback, argc,
                     argv, &result);
}

void UIOwner::OnAvoidKeyboardCallback(float translate_y) const {
  base::NapiHandleScope scope(env_);
  napi_value js_recv = base::NapiUtil::GetReferenceNapiValue(env_, js_this_);
  napi_value callback =
      base::NapiUtil::GetReferenceNapiValue(env_, on_avoid_keyboard_callback_);
  if (!js_recv || !callback) {
    return;
  }

  size_t argc = 1;
  /**
   * 0 - sign float
   */
  napi_value argv[argc];
  napi_create_int32(env_, translate_y, &argv[0]);

  napi_value result;
  napi_call_function(env_, js_recv, callback, argc, argv, &result);
}

void UIOwner::AddOrRemoveUIFromExclusiveSet(int32_t sign, bool exclusive) {
  if (exclusive) {
    accessibility_exclusive_.insert(sign);
  } else {
    auto it = accessibility_exclusive_.find(sign);
    if (it != accessibility_exclusive_.end()) {
      accessibility_exclusive_.erase(it);
    }
  }
}

bool UIOwner::ContainAccessibilityExclusiveUI(int32_t sign) {
  auto it = accessibility_exclusive_.find(sign);
  if (it != accessibility_exclusive_.end()) {
    return true;
  }
  return false;
}

void UIOwner::ResetAccessibilityAttrs() {
  for (const auto& [_, ui] : ui_holder_) {
    ui->ResetAccessibilityAttrs();
  }
}
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
