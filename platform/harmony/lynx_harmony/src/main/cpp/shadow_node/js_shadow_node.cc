// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/js_shadow_node.h"

#include <node_api.h>

#include <cstddef>
#include <limits>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <utility>

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/memory/ref_ptr_internal.h"
#include "base/include/platform/harmony/napi_util.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/base/harmony/napi_convert_helper.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node.h"

namespace lynx {
namespace tasm {
namespace harmony {

#pragma region JSExtraBundle

JSExtraBundle::JSExtraBundle(napi_env env, napi_value js_object) {
  napi_create_reference(env, js_object, 1, &js_object_);
  env_ = env;
}

JSExtraBundle::~JSExtraBundle() { napi_delete_reference(env_, js_object_); }

napi_value JSExtraBundle::JSObject() const {
  return base::NapiUtil::GetReferenceNapiValue(env_, js_object_);
}

void JSExtraBundle::ReleaseSelf() const { delete this; }

#pragma endregion JSExtraBundle

napi_value JSShadowNode::MarkDirty(napi_env env, napi_callback_info info) {
  JSShadowNode* node = nullptr;
  napi_value js_this = nullptr;
  size_t argc = 0;
  napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);
  napi_unwrap(env, js_this, reinterpret_cast<void**>(&node));
  // The NAPI wrap is removed on the UI thread. The ShadowNodeOwner ensures that
  // if the ShadowNode can be unwrapped from the napi_value, it still exists.
  if (node) {
    node->MarkDirty();
  }
  return nullptr;
}

napi_value JSShadowNode::RequestLayout(napi_env env, napi_callback_info info) {
  JSShadowNode* node = nullptr;
  napi_value js_this = nullptr;
  size_t argc = 0;
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, JS_SHADOW_NODE_REQUEST_LAYOUT);
  napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);
  napi_unwrap(env, js_this, reinterpret_cast<void**>(&node));
  if (node) {
    node->RequestLayout();
  }
  return nullptr;
}

napi_value JSShadowNode::GetChildren(napi_env env, napi_callback_info info) {
  // Deprecated.
  JSShadowNode* node;
  napi_value js_this;
  size_t argc = 0;
  napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);
  napi_unwrap(env, js_this, reinterpret_cast<void**>(&node));
  const auto& native_children = node->ShadowNode::GetChildren();
  const size_t count = native_children.size();
  int children_ids[count];
  for (int i = 0; i < count; i++) {
    children_ids[i] = native_children[i]->Signature();
  }
  return base::NapiUtil::CreateArrayBuffer(env, children_ids,
                                           count * sizeof(int));
}

LayoutResult JSShadowNode::Measure(float width, MeasureMode width_mode,
                                   float height, MeasureMode height_mode,
                                   bool final_measure) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, JS_SHADOW_NODE_MEASURE);
  width = width_mode == MeasureMode::Indefinite
              ? std::numeric_limits<float>::max()
              : width;
  height = height_mode == MeasureMode::Indefinite
               ? std::numeric_limits<float>::max()
               : height;
  LayoutResult measure_result;
  if (measure_) {
    auto task = [this, &measure_result, width, width_mode, height,
                 height_mode] {
      base::NapiHandleScope scope(env_);
      napi_value js_recv = GetJSObject();
      napi_value measure_func =
          base::NapiUtil::GetReferenceNapiValue(env_, measure_);
      if (!measure_func || !js_recv) {
        return;
      }
      napi_value result;
      size_t argc = 4;
      napi_value argv[4];
      napi_create_double(env_, width, &argv[0]);
      napi_create_int32(env_, static_cast<int32_t>(width_mode), &argv[1]);
      napi_create_double(env_, height, &argv[2]);
      napi_create_int32(env_, static_cast<int32_t>(height_mode), &argv[3]);
      napi_call_function(env_, js_recv, measure_func, argc, argv, &result);
      napi_value rets[3];
      napi_get_element(env_, result, 0, &rets[0]);
      napi_get_element(env_, result, 1, &rets[1]);
      napi_get_element(env_, result, 2, &rets[2]);
      double result_width, result_height, result_baseline;
      napi_get_value_double(env_, rets[0], &result_width);
      napi_get_value_double(env_, rets[1], &result_height);
      napi_get_value_double(env_, rets[2], &result_baseline);
      measure_result = LayoutResult{static_cast<float>(result_width),
                                    static_cast<float>(result_height),
                                    static_cast<float>(result_baseline)};
    };

    if (context_) {
      context_->PostSyncTaskOnUIThread(std::move(task));
    } else {
      // If context_ is not set, this method is called on the UI thread
      // therefore, execute the task directly.
      task();
    }
  }
  return measure_result;
}

void JSShadowNode::Align() {
  if (align_) {
    context_->PostSyncTaskOnUIThread([this] {
      base::NapiHandleScope scope(env_);
      napi_value js_recv = GetJSObject();
      napi_value align_func =
          base::NapiUtil::GetReferenceNapiValue(env_, align_);
      if (!align_func || !js_recv) {
        return;
      }
      size_t argc = 0;
      napi_value argv[argc];
      napi_call_function(env_, js_recv, align_func, argc, argv, nullptr);
    });
  }
}

void JSShadowNode::UpdateProps(PropBundleHarmony* props) {
  if (!props) {
    return;
  }
  context_->PostSyncTaskOnUIThread([this, props] {
    napi_value prop_bundle = props->GetJSProps();
    size_t argc = 1;
    napi_value argv[argc];
    argv[0] = prop_bundle;
    // todo(renzhognyue): retain ref to the method.
    base::NapiUtil::InvokeJsMethod(env_, js_ref_, "updateProps", argc, argv);
  });
}

napi_value JSShadowNode::Constructor(napi_env env, napi_callback_info info) {
  /**
    0 - js ref
    1 - sign: number
    2 - tag: string
  */
  size_t argc = 3;
  napi_value args[argc];
  napi_value js_this;
  napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
  // get node signature
  int sign;
  napi_get_value_int32(env, args[1], &sign);

  // get node tag
  std::string tag = base::NapiUtil::ConvertToShortString(env, args[2]);

  // C++ take the ownership of the instance. Managed by
  // ShadowNodeOwner::NodeHolder via std::unique_ptr.
  JSShadowNode* node = new JSShadowNode(sign, tag, env);

  napi_create_reference(env, args[0], 1, &node->js_ref_);
  napi_wrap(
      env, js_this, node, [](napi_env, void* data, void*) {}, nullptr, nullptr);
  return js_this;
}

JSShadowNode::~JSShadowNode() = default;

void JSShadowNode::OnPropsUpdate(const char* attr, const lepus::Value& value) {}

napi_value JSShadowNode::Init(napi_env env, napi_value exports) {
#define DECLARE_NAPI_FUNCTION(name, func) \
  {(name), nullptr, (func), nullptr, nullptr, nullptr, napi_default, nullptr}
  napi_property_descriptor properties[] = {
      DECLARE_NAPI_FUNCTION("setMeasureFunc", SetMeasureFunc),
      DECLARE_NAPI_FUNCTION("markDirty", MarkDirty),
      DECLARE_NAPI_FUNCTION("requestLayout", RequestLayout),
      DECLARE_NAPI_FUNCTION("getChildren", GetChildren),
      DECLARE_NAPI_FUNCTION("setExtraDataFunc", SetExtraDataFunc),
  };
#undef DECLARE_NAPI_FUNCTION

  constexpr size_t size = std::size(properties);

  napi_value cons;
  napi_status status =
      napi_define_class(env, "ShadowNode", NAPI_AUTO_LENGTH, Constructor,
                        nullptr, size, properties, &cons);
  assert(status == napi_ok);

  status = napi_set_named_property(env, exports, "ShadowNode", cons);
  return exports;
}

napi_value JSShadowNode::GetJSObject() const {
  return base::NapiUtil::GetReferenceNapiValue(env_, js_ref_);
}

// Should Only be called inside JSObject's constructor.
napi_value JSShadowNode::SetMeasureFunc(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value argv[argc];
  napi_value js_this;
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  JSShadowNode* node;
  napi_unwrap(env, js_this, reinterpret_cast<void**>(&node));
  napi_valuetype type;
  napi_typeof(env, argv[0], &type);
  napi_valuetype align_func;
  napi_typeof(env, argv[1], &align_func);

  if (type != napi_null) {
    napi_create_reference(env, argv[0], 0, &node->measure_);
    node->SetCustomMeasureFunc(node);
  }

  if (align_func != napi_null) {
    napi_create_reference(env, argv[1], 0, &node->align_);
  }
  return nullptr;
}

// Should Only be called inside JSObject's constructor.
napi_value JSShadowNode::SetExtraDataFunc(napi_env env,
                                          napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  napi_value js_this;
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  JSShadowNode* node;
  napi_unwrap(env, js_this, reinterpret_cast<void**>(&node));
  napi_valuetype type;
  napi_typeof(env, argv[0], &type);
  if (type != napi_function || !node) {
    node->extra_bundle_getter_ = nullptr;
    return nullptr;
  }
  napi_create_reference(env, argv[0], 0, &node->extra_bundle_getter_);
  return nullptr;
}

fml::RefPtr<fml::RefCountedThreadSafeStorage> JSShadowNode::getExtraBundle() {
  if (!extra_bundle_getter_) {
    return nullptr;
  }

  napi_value result = nullptr;
  context_->PostSyncTaskOnUIThread([this, ret = &result] {
    base::NapiHandleScope scope(env_);
    napi_value extra_bundle_getter =
        base::NapiUtil::GetReferenceNapiValue(env_, extra_bundle_getter_);
    napi_value js_this = GetJSObject();

    if (!extra_bundle_getter || !js_this) {
      return;
    }
    size_t argc = 0;
    napi_call_function(env_, js_this, extra_bundle_getter, argc, nullptr, ret);
  });

  return result ? fml::AdoptRef(
                      reinterpret_cast<fml::RefCountedThreadSafeStorage*>(
                          new JSExtraBundle(env_, result)))
                : nullptr;
}

// Called by ShadowNodeOwner.
void JSShadowNode::Destroy() {
  base::NapiHandleScope scope(env_);
  napi_value js_object = GetJSObject();
  if (js_object) {
    napi_remove_wrap(env_, js_object, nullptr);
  }
  napi_delete_reference(env_, js_ref_);
  if (measure_) {
    napi_delete_reference(env_, measure_);
  }
  if (align_) {
    napi_delete_reference(env_, align_);
  }
  if (extra_bundle_getter_) {
    napi_delete_reference(env_, extra_bundle_getter_);
  }
  measure_ = nullptr;
  align_ = nullptr;
  env_ = nullptr;
  js_ref_ = nullptr;
}

void JSShadowNode::MarkDestroyed() {
  // The destroyed state is managed by ShadowNodeOwner on the Layout thread.
  // Before the instance invokes methods on the UI thread, it must synchronize
  // with this state. The destruction of the instance, which removes
  // NAPI-related members, is posted from the Layout thread to the UI thread
  // asynchronously.
  std::unique_lock<std::shared_mutex> lock(destroyed_mutex_);
  ShadowNode::MarkDestroyed();
}

void JSShadowNode::MarkDirty() const {
  if (!context_) {
    return;
  }
  // The existence of the instance does not imply that the node is not being
  // removed. We synchronize the state here to ensure that the ShadowNode is not
  // marked as destroyed on the Layout thread.
  std::shared_lock<std::shared_mutex> lock(destroyed_mutex_);
  if (!IsDestroyed()) {
    context_->RunOnLayoutThread(
        [node = fml::Ref(this)] { node->ShadowNode::MarkDirty(); });
  }
}

void JSShadowNode::RequestLayout() const {
  if (!context_) {
    return;
  }
  std::shared_lock<std::shared_mutex> lock(destroyed_mutex_);
  if (!IsDestroyed()) {
    context_->RunOnLayoutThread(
        [node = fml::Ref(this)] { node->ShadowNode::RequestLayout(); });
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
