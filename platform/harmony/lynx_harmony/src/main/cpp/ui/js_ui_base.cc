// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/js_ui_base.h"

#include <arkui/native_node_napi.h>
#include <node_api.h>

#include <string>
#include <utility>

#include "base/include/platform/harmony/napi_util.h"
#include "core/base/harmony/napi_convert_helper.h"
#include "core/renderer/ui_wrapper/common/harmony/prop_bundle_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/js_shadow_node.h"

namespace lynx {
namespace tasm {
namespace harmony {

void JSUIBase::InvokeMethod(const std::string& method, const lepus::Value& args,
                            UIMethodCallback callback) {
  base::NapiHandleScope scope(env_);
  napi_value js_recv = base::NapiUtil::GetReferenceNapiValue(env_, js_ref_);
  napi_value invoke_ui_method =
      base::NapiUtil::GetReferenceNapiValue(env_, js_ui_method_);

  if (!js_recv || !invoke_ui_method) {
    UIBase::InvokeMethod(method, args, std::move(callback));
    return;
  }

  size_t argc = 3;
  napi_value argv[argc];
  napi_create_string_latin1(env_, method.c_str(), method.size(), &argv[0]);
  argv[1] = base::NapiConvertHelper::CreateNapiValue(env_, args);

  JSCallbackWrapper* wrapper = new JSCallbackWrapper();
  wrapper->callback = std::move(callback);
  napi_create_function(
      env_, "callback", 9,
      [](napi_env env, napi_callback_info info) -> napi_value {
        napi_value js_obj;
        size_t argc = 2;
        napi_value argv[argc];
        JSCallbackWrapper* callback;
        napi_get_cb_info(env, info, &argc, argv, &js_obj,
                         reinterpret_cast<void**>(&callback));
        int32_t callback_id = base::NapiUtil::ConvertToInt32(env, argv[0]);
        auto params =
            base::NapiConvertHelper::ConvertToLepusValue(env, argv[1]);
        callback->callback(callback_id, params);
        return nullptr;
      },
      wrapper, &argv[2]);

  napi_wrap(
      env_, argv[2], wrapper,
      [](napi_env env, void* obj, void* data) {
        if (auto* wrapper = reinterpret_cast<JSCallbackWrapper*>(obj);
            wrapper != nullptr) {
          delete wrapper;
        }
      },
      nullptr, nullptr);
  napi_value result;
  napi_status status =
      napi_call_function(env_, js_recv, invoke_ui_method, argc, argv, &result);
  bool called = false;
  napi_get_value_bool(env_, result, &called);

  if (!called || status != napi_ok) {
    UIBase::InvokeMethod(method, args, std::move(wrapper->callback));
  }
}
void JSUIBase::SetFrameNode(ArkUI_NodeHandle frame_node) {
  if (frame_node_) {
    NodeManager::Instance().RemoveNode(node_, frame_node_);
    NodeManager::Instance().DisposeNode(frame_node_);
  }
  frame_node_ = frame_node;
  NodeManager::Instance().InsertNode(node_, frame_node_, 0);
}

napi_value JSUIBase::Constructor(napi_env env, napi_callback_info info) {
  size_t argc = 14;
  /** 0 - js ref
   *  1 - context ptr array
   *  2 - FrameNode napi_value
   *  3 - sign
   *  4 - tag
   *  5 - update func ref
   *  6 - layout func ref
   *  7 - ui method func ref
   *  8 - dispose func ref
   *  9 - focus change func ref
   *  10 - focusable func ref
   *  11 - nodeReady func ref
   *  12 - layout children via arkts arkui
   *  13 - updateExtraData func ref
   */
  napi_value argv[argc];
  napi_value js_this;
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  LynxContext* context = reinterpret_cast<LynxContext*>(
      base::NapiUtil::ConvertToPtr(env, argv[1]));
  napi_valuetype type;
  napi_typeof(env, argv[2], &type);
  ArkUI_NodeHandle node{nullptr};
  if (type != napi_null) {
    OH_ArkUI_GetNodeHandleFromNapiValue(env, argv[2], &node);
  }
  int sign = base::NapiUtil::ConvertToInt32(env, argv[3]);
  std::string tag = base::NapiUtil::ConvertToShortString(env, argv[4]);
  bool has_customized_layout = false;
  napi_get_value_bool(env, argv[12], &has_customized_layout);
  auto* ui = new JSUIBase(context, node, sign, tag, has_customized_layout);
  ui->env_ = env;
  napi_create_reference(env, argv[0], 1, &ui->js_ref_);
  napi_create_reference(env, argv[5], 1, &ui->js_update_);
  napi_create_reference(env, argv[6], 1, &ui->js_layout_);
  napi_create_reference(env, argv[7], 1, &ui->js_ui_method_);
  napi_create_reference(env, argv[8], 1, &ui->js_dispose_);
  napi_create_reference(env, argv[9], 1, &ui->js_focus_change_);
  napi_create_reference(env, argv[10], 1, &ui->js_focusable_);
  napi_create_reference(env, argv[11], 1, &ui->js_node_ready_);
  napi_create_reference(env, argv[13], 1, &ui->js_update_extra_data_);

  napi_wrap(
      env, js_this, ui, [](napi_env env, void* data, void*) {}, nullptr,
      nullptr);
  return js_this;
}

void JSUIBase::UpdateProps(PropBundleHarmony* props) {
  UIBase::UpdateProps(props);
  base::NapiHandleScope scope(env_);
  napi_value js_recv = base::NapiUtil::GetReferenceNapiValue(env_, js_ref_);
  napi_value update = base::NapiUtil::GetReferenceNapiValue(env_, js_update_);

  if (!js_recv || !update) {
    return;
  }
  napi_value prop_bundle = props->GetJSProps();
  napi_value events = props->GetJSEventHandler();
  size_t argc = 2;
  napi_value argv[argc];
  argv[0] = prop_bundle;
  argv[1] = events;
  napi_call_function(env_, js_recv, update, argc, argv, nullptr);
}

napi_value JSUIBase::GetChildren(napi_env env, napi_callback_info info) {
  JSUIBase* ui = nullptr;
  napi_value js_this;
  size_t argc = 0;
  napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);
  const auto status = napi_unwrap(env, js_this, reinterpret_cast<void**>(&ui));
  if (status != napi_ok || !ui) {
    return nullptr;
  }
  const auto& native_children = ui->Children();
  const size_t count = native_children.size();
  int children_ids[count];
  for (int i = 0; i < count; i++) {
    children_ids[i] = native_children[i]->Sign();
  }
  return base::NapiUtil::CreateArrayBuffer(env, children_ids,
                                           count * sizeof(int));
}

napi_value JSUIBase::SetFrameNode(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[argc];
  napi_value js_this;
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  JSUIBase* ui{nullptr};
  if (const auto status =
          napi_unwrap(env, js_this, reinterpret_cast<void**>(&ui));
      status != napi_ok || !ui) {
    return nullptr;
  }
  napi_valuetype type;
  napi_typeof(env, argv[0], &type);
  ArkUI_NodeHandle node{nullptr};
  if (type != napi_null) {
    OH_ArkUI_GetNodeHandleFromNapiValue(env, argv[0], &node);
  }
  ui->SetFrameNode(node);
  return nullptr;
}

napi_value JSUIBase::GetUIFromNativeContent(napi_env env,
                                            napi_callback_info info) {
  /*
   * 0 - nativeContent: NativeContent
   */
  size_t argc = 1;
  napi_value argv[argc];
  napi_value js_this;
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  NativeNodeContent* content{nullptr};
  napi_unwrap(env, argv[0], reinterpret_cast<void**>(&content));
  if (auto* ui = content ? content->UI() : nullptr; ui && ui->HasJSObject()) {
    const auto* js_ui = reinterpret_cast<JSUIBase*>(ui);
    napi_value ret = base::NapiUtil::GetReferenceNapiValue(env, js_ui->js_ref_);
    return ret;
  }
  return nullptr;
}

napi_value JSUIBase::GetContentSize(napi_env env, napi_callback_info info) {
  napi_value ret{nullptr};
  napi_create_array_with_length(env, 2, &ret);
  size_t argc = 1;
  napi_value argv[argc];
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
  NativeNodeContent* content{nullptr};
  napi_unwrap(env, argv[0], reinterpret_cast<void**>(&content));
  const UIBase* ui = content ? content->UI() : nullptr;
  napi_value width{nullptr}, height{nullptr};
  if (ui) {
    napi_create_double(env, ui->width_, &width);
    napi_create_double(env, ui->height_, &height);
  } else {
    napi_create_double(env, 0.0, &width);
    napi_create_double(env, 0.0, &height);
  }
  napi_set_element(env, ret, 0, width);
  napi_set_element(env, ret, 1, height);
  return ret;
}

napi_value JSUIBase::SetChildrenManagementFuncs(napi_env env,
                                                napi_callback_info info) {
  napi_value js_this;
  size_t argc = 2;
  napi_value argv[argc];
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  JSUIBase* ui{nullptr};
  napi_unwrap(env, js_this, reinterpret_cast<void**>(&ui));
  if (ui) {
    napi_create_reference(env, argv[0], 1, &ui->js_insert_child_);
    napi_create_reference(env, argv[1], 1, &ui->js_remove_child_);
  }
  return nullptr;
}

JSUIBase::~JSUIBase() {
  base::NapiHandleScope scope(env_);
  napi_value js_recv = base::NapiUtil::GetReferenceNapiValue(env_, js_ref_);
  napi_value dispose = base::NapiUtil::GetReferenceNapiValue(env_, js_dispose_);
  if (!js_recv || !dispose) {
    return;
  }
  if (frame_node_) {
    NodeManager::Instance().RemoveNode(node_, frame_node_);
    NodeManager::Instance().DisposeNode(frame_node_);
  }
  size_t argc = 0;
  napi_call_function(env_, js_recv, dispose, argc, nullptr, nullptr);
  napi_delete_reference(env_, js_focus_change_);
  napi_delete_reference(env_, js_dispose_);
  napi_delete_reference(env_, js_ui_method_);
  napi_delete_reference(env_, js_layout_);
  napi_delete_reference(env_, js_node_ready_);
  napi_delete_reference(env_, js_update_);
  napi_delete_reference(env_, js_focusable_);
  if (HasCustomizedLayout()) {
    napi_delete_reference(env_, js_insert_child_);
    napi_delete_reference(env_, js_remove_child_);
  }
  napi_delete_reference(env_, js_update_extra_data_);
  napi_value js_object = base::NapiUtil::GetReferenceNapiValue(env_, js_ref_);
  if (js_object) {
    napi_remove_wrap(env_, js_object, nullptr);
  }
  napi_delete_reference(env_, js_ref_);
}

void JSUIBase::AddChild(UIBase* child, int index) {
  if (index == -1) {
    children_.emplace_back(child);
  } else {
    children_.insert(children_.begin() + index, child);
  }
  child->SetParent(this);
  auto sibling = index != 0 ? Children().at(index - 1)->Node() : nullptr;
  if (!HasCustomizedLayout()) {
    NodeManager::Instance().InsertNodeAfter(node_, child->DrawNode(), sibling);
  } else {
    if (!child->NodeContent()) {
      context_->CreateNodeContent(child);
    }
    base::NapiHandleScope scope(env_);
    napi_value js_recv = base::NapiUtil::GetReferenceNapiValue(env_, js_ref_);
    napi_value insert =
        base::NapiUtil::GetReferenceNapiValue(env_, js_insert_child_);
    size_t argc = 2;
    napi_value argv[argc];
    /**
     * 0 - nativeContent: NativeContent,
     * 1 - index: number
     */
    argv[0] = child->NodeContent()->JSObject();
    napi_create_int32(env_, index, &argv[1]);
    napi_value result{nullptr};
    napi_call_function(env_, js_recv, insert, argc, argv, &result);
  }
}

void JSUIBase::RemoveChild(UIBase* child) {
  child->WillRemoveFromUIParent();
  child->SetParent(nullptr);
  children_.erase(std::remove(children_.begin(), children_.end(), child),
                  children_.end());
  if (HasCustomizedLayout()) {
    base::NapiHandleScope scope(env_);
    napi_value js_recv = base::NapiUtil::GetReferenceNapiValue(env_, js_ref_);
    napi_value remove =
        base::NapiUtil::GetReferenceNapiValue(env_, js_remove_child_);
    const size_t argc{1};
    napi_value argv[argc] = {nullptr};
    /**
     * 0 - nativeContent: NativeContent
     */
    if (!js_recv || !remove) {
      return;
    }
    NativeNodeContent* content = child->NodeContent();
    if (content) {
      argv[0] = content->JSNodeContent();
    }
    napi_call_function(env_, js_recv, remove, argc, argv, nullptr);
  } else {
    RemoveNode(child);
  }
}

napi_value JSUIBase::Init(napi_env env, napi_value exports) {
#define DECLARE_NAPI_FUNCTION(name, func) \
  {(name), nullptr, (func), nullptr, nullptr, nullptr, napi_default, nullptr}
#define DECLARE_NAPI_STATIC_FUNCTION(name, func) \
  { (name), nullptr, (func), nullptr, nullptr, nullptr, napi_static, nullptr }

  napi_property_descriptor properties[] = {
      DECLARE_NAPI_FUNCTION("getChildren", GetChildren),
      DECLARE_NAPI_FUNCTION("setFrameNode", SetFrameNode),
      DECLARE_NAPI_FUNCTION("setFocusedUI", SetFocusedUI),
      DECLARE_NAPI_FUNCTION("unsetFocusedUI", UnsetFocusedUI),
      DECLARE_NAPI_FUNCTION("setChildrenManagementFuncs",
                            SetChildrenManagementFuncs),
      DECLARE_NAPI_STATIC_FUNCTION("getUIFromNativeContent",
                                   GetUIFromNativeContent),
      DECLARE_NAPI_STATIC_FUNCTION("getContentSize", GetContentSize),
  };
#undef DECLARE_NAPI_FUNCTION
#undef DECLARE_NAPI_STATIC_FUNCTION

  constexpr size_t size = std::size(properties);

  napi_value cons;
  napi_status status =
      napi_define_class(env, "UIBase", NAPI_AUTO_LENGTH, Constructor, nullptr,
                        size, properties, &cons);
  assert(status == napi_ok);

  status = napi_set_named_property(env, exports, "UIBase", cons);
  return exports;
}

void JSUIBase::UpdateLayout(float left, float top, float width, float height,
                            const float* paddings, const float* margins,
                            const float* sticky, float max_height,
                            uint32_t node_index) {
  UIBase::UpdateLayout(left, top, width, height, paddings, margins, sticky,
                       max_height, node_index);
  base::NapiHandleScope scope(env_);
  napi_value js_recv = base::NapiUtil::GetReferenceNapiValue(env_, js_ref_);
  napi_value layout = base::NapiUtil::GetReferenceNapiValue(env_, js_layout_);
  /**
   * 0 - x: number,
   * 1 - y: number,
   * 2 - width: number,
   * 3 - height: number,
   * 4 - paddingLeft: number,
   * 5 - paddingTop: number,
   * 6 - paddingRight: number,
   * 7 - paddingBottom: number,
   * 8 - marginLeft: number,
   * 9 - marginTop: number,
   * 10 - marginRight: number,
   * 11 - marginBottom: number,
   * 12 - sticky?: number[]
   */
  size_t argc = 13;
  napi_value argv[argc];
  napi_create_double(env_, left, &argv[0]);
  napi_create_double(env_, top, &argv[1]);
  napi_create_double(env_, width, &argv[2]);
  napi_create_double(env_, height, &argv[3]);

  napi_create_double(env_, paddings[0], &argv[4]);
  napi_create_double(env_, paddings[1], &argv[5]);
  napi_create_double(env_, paddings[2], &argv[6]);
  napi_create_double(env_, paddings[3], &argv[7]);

  napi_create_double(env_, margins[0], &argv[8]);
  napi_create_double(env_, margins[1], &argv[9]);
  napi_create_double(env_, margins[2], &argv[10]);
  napi_create_double(env_, margins[3], &argv[11]);

  napi_value sticky_value;
  size_t sticky_count = 4;
  if (sticky) {
    napi_create_array_with_length(env_, sticky_count, &sticky_value);
    for (size_t i = 0; i < sticky_count; i++) {
      napi_value result = nullptr;
      napi_create_double(env_, sticky[i], &result);
      napi_set_element(env_, sticky_value, i, result);
    }
  } else {
    napi_get_undefined(env_, &sticky_value);
  }
  argv[12] = sticky_value;
  napi_value result;
  napi_call_function(env_, js_recv, layout, argc, argv, &result);
}

bool JSUIBase::Focusable() {
  base::NapiHandleScope scope(env_);
  size_t argc = 0;
  napi_value argv[argc];
  napi_value js_recv = base::NapiUtil::GetReferenceNapiValue(env_, js_ref_);
  napi_value focusable_func =
      base::NapiUtil::GetReferenceNapiValue(env_, js_focusable_);
  napi_value result;
  napi_call_function(env_, js_recv, focusable_func, argc, argv, &result);
  bool focusable = false;
  napi_get_value_bool(env_, result, &focusable);
  return focusable;
}

void JSUIBase::OnFocusChange(bool has_focus, bool is_focus_transition) {
  base::NapiHandleScope scope(env_);
  size_t argc = 2;
  napi_value argv[argc];
  napi_get_boolean(env_, has_focus, &argv[0]);
  napi_get_boolean(env_, is_focus_transition, &argv[1]);
  napi_value js_recv = base::NapiUtil::GetReferenceNapiValue(env_, js_ref_);
  napi_value focus_change =
      base::NapiUtil::GetReferenceNapiValue(env_, js_focus_change_);
  napi_value result;
  napi_call_function(env_, js_recv, focus_change, argc, argv, &result);
}

napi_value JSUIBase::SetFocusedUI(napi_env env, napi_callback_info info) {
  size_t argc = 0;
  napi_value js_this;
  napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);
  JSUIBase* ui;
  if (const napi_status status =
          napi_unwrap(env, js_this, reinterpret_cast<void**>(&ui));
      status != napi_ok || !ui) {
    return nullptr;
  }
  ui->context_->SetFocusedTarget(ui->WeakTarget());
  return nullptr;
}

napi_value JSUIBase::UnsetFocusedUI(napi_env env, napi_callback_info info) {
  size_t argc = 0;
  napi_value js_this;
  napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);
  JSUIBase* ui;
  const napi_status status =
      napi_unwrap(env, js_this, reinterpret_cast<void**>(&ui));
  if (status != napi_ok || !ui) {
    return nullptr;
  }
  ui->context_->UnsetFocusedTarget(ui->WeakTarget());
  return nullptr;
}

void JSUIBase::OnNodeReady() {
  UIBase::OnNodeReady();
  base::NapiHandleScope scope(env_);
  napi_value js_recv = base::NapiUtil::GetReferenceNapiValue(env_, js_ref_);
  napi_value node_ready =
      base::NapiUtil::GetReferenceNapiValue(env_, js_node_ready_);
  if (!js_recv || !node_ready) {
    return;
  }
  napi_call_function(env_, js_recv, node_ready, 0, nullptr, nullptr);
}

void JSUIBase::UpdateExtraData(
    const fml::RefPtr<fml::RefCountedThreadSafeStorage>& extra_data) {
  if (extra_data) {
    base::NapiHandleScope scope(env_);
    napi_value js_recv = base::NapiUtil::GetReferenceNapiValue(env_, js_ref_);
    napi_value update_extra_data =
        base::NapiUtil::GetReferenceNapiValue(env_, js_update_extra_data_);
    if (!js_recv || !update_extra_data) {
      return;
    }
    size_t argc = 1;
    napi_value argv[1];
    auto* bundle = reinterpret_cast<JSExtraBundle*>(extra_data.get());
    argv[0] = bundle->JSObject();
    napi_status status = napi_call_function(env_, js_recv, update_extra_data,
                                            argc, argv, nullptr);
    if (status != napi_ok) {
      return;
    }
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
