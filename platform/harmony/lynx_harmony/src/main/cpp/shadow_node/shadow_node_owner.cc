// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node_owner.h"

#include <js_native_api.h>

#include <cassert>
#include <memory>
#include <string>
#include <utility>

#include "base/include/float_comparison.h"
#include "base/include/platform/harmony/napi_util.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/base/harmony/napi_convert_helper.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/inline_placeholder_shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/js_shadow_node.h"

namespace lynx {
namespace tasm {
namespace harmony {
void LayoutVSyncProxy::ScheduleLayout(base::closure callback) {
  trigger_layout_callback_ = std::move(callback);
  if (!layout_scheduled_ && vsync_monitor_) {
    vsync_monitor_->AsyncRequestVSync(
        [weak_this = weak_from_this()](int64_t, int64_t) -> void {
          auto strong_this = weak_this.lock();
          if (strong_this) {
            strong_this->trigger_layout_callback_();
            strong_this->layout_scheduled_ = false;
          }
        });
  }
  layout_scheduled_ = true;
}

ShadowNodeOwner::ShadowNodeOwner()
    : text_measure_cache_(new TextMeasureCache),
      font_face_manager_(new FontFaceManager(this)) {}

// delete the reference to release the object in js.
ShadowNodeOwner::~ShadowNodeOwner() = default;

napi_value ShadowNodeOwner::Init(napi_env env, napi_value exports) {
#define DECLARE_NAPI_FUNCTION(name, func) \
  {(name), nullptr, (func), nullptr, nullptr, nullptr, napi_default, nullptr}

  napi_property_descriptor properties[] = {
      DECLARE_NAPI_FUNCTION("findJSShadowNodeBySign", FindJSShadowNodeBySign),
      DECLARE_NAPI_FUNCTION("measureNativeNode", MeasureLayoutNode),
      DECLARE_NAPI_FUNCTION("alignNativeNode", AlignLayoutNode),
      DECLARE_NAPI_FUNCTION("destroy", Destroy),
  };
#undef DECLARE_NAPI_FUNCTION

  constexpr size_t size = std::size(properties);

  napi_value cons;
  napi_status status =
      napi_define_class(env, "ShadowNodeOwner", NAPI_AUTO_LENGTH, New, nullptr,
                        size, properties, &cons);
  assert(status == napi_ok);

  status = napi_set_named_property(env, exports, "ShadowNodeOwner", cons);

  assert(status == napi_ok);

  return exports;
}

void ShadowNodeOwner::SetContext(const std::shared_ptr<LynxContext>& context) {
  context_ = context;

  // ty to load system font when Init ShadowNodeOwner
  // FIXME(linxs): need to fetch system font when system font updated later
  font_face_manager_->TryFetchSystemFont(env_);
}

void ShadowNodeOwner::SetLayoutNodeManager(
    LayoutNodeManager* layout_node_manager) {
  layout_node_manager_ = layout_node_manager;
}

int ShadowNodeOwner::CreateShadowNode(int sign, const std::string& tag,
                                      PropBundleHarmony* props,
                                      bool allow_inline) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, SHADOW_NODE_OWNER_CREATE_SHADOW_NODE + tag);
  int ret = COMMON;

  ShadowNode* node = nullptr;
  auto* node_info = context_->GetNodeInfo(tag);
  if (node_info && node_info->layout_node_creator &&
      (node_info->node_type & LayoutNodeType::CUSTOM)) {
    node = node_info->layout_node_creator(sign, tag);
  } else if (node_info == nullptr ||
             (node_info->node_type & LayoutNodeType::CUSTOM)) {
    // the tag is marked as CUSTOM, but has no related layout_node_creator, we
    // should createJSShadowNode
    node = CreateJSShadowNode(sign, tag, props);
  }

  if (node != nullptr) {
    ret = CUSTOM;
    if (node->IsVirtual()) {
      ret |= VIRTUAL;
    }
  } else {
    // No subclass of ShadowNode but allow_inline is true,
    // should create a default ShadowNode
    if (allow_inline) {
      node = new InlinePlaceholderShadowNode(sign, tag);
      ret |= (COMMON | INLINE);
    }
  }
  if (node) {
    node_holder_[sign] = fml::AdoptRef(node);
    node->SetLayoutNodeManager(layout_node_manager_);
    node->SetContext(context_.get());
    node->AdoptSlNode();
    node->UpdateProps(props);
  }
  return ret;
}

void ShadowNodeOwner::InsertLayoutNode(int parent, int child, int index) {
  auto parent_it = node_holder_.find(parent);
  auto child_it = node_holder_.find(child);
  if (parent_it == node_holder_.end() || child_it == node_holder_.end()) {
    return;
  }
  const auto& parent_node = parent_it->second;
  if (index == -1) {
    index = parent_node->GetChildren().size();
  }
  parent_node->AddChild(child_it->second.get(), index);
}

void ShadowNodeOwner::RemoveLayoutNode(int parent, int child, int index) {
  auto parent_it = node_holder_.find(parent);
  auto child_it = node_holder_.find(child);
  if (parent_it == node_holder_.end() || child_it == node_holder_.end()) {
    return;
  }
  parent_it->second->RemoveChild(child_it->second.get());
}

void ShadowNodeOwner::MoveLayoutNode(int parent, int child, int from_index,
                                     int to_index) {}

void ShadowNodeOwner::UpdateLayoutNode(int id,
                                       PropBundleHarmony* painting_data) {
  auto it = node_holder_.find(id);
  if (it == node_holder_.end()) {
    return;
  }
  it->second->UpdateProps(painting_data);
}

void ShadowNodeOwner::OnLayoutBefore(int id) {
  auto it = node_holder_.find(id);
  if (it == node_holder_.end()) {
    return;
  }
  it->second->OnLayoutBefore();
}

void ShadowNodeOwner::OnLayout(int id, float left, float top, float width,
                               float height) {
  auto it = node_holder_.find(id);
  if (it == node_holder_.end()) {
    return;
  }
  it->second->UpdateLayout(left, top, width, height);
}

LayoutResult ShadowNodeOwner::MeasureNode(int id, float width,
                                          int32_t width_mode, float height,
                                          int32_t height_mode,
                                          bool final_measure) {
  auto it = node_holder_.find(id);
  if (it == node_holder_.end()) {
    return {};
  }
  return it->second->MeasureLayoutNode(
      width, static_cast<MeasureMode>(width_mode), height,
      static_cast<MeasureMode>(height_mode), final_measure);
}

void ShadowNodeOwner::ScheduleLayout(base::closure callback) {
  auto task = base::MoveOnlyClosure<void>(
      [weak_self = weak_from_this(), closure = std::move(callback)]() mutable {
        auto strong_this = weak_self.lock();
        strong_this->GetLayoutVSyncProxy()->ScheduleLayout(std::move(closure));
      });
  context_->RunOnLayoutThread(std::move(task));
}

ShadowNode* ShadowNodeOwner::CreateJSShadowNode(int sign,
                                                const std::string& tag,
                                                PropBundleHarmony* props) {
  auto runnable = [this, sign, &tag]() -> ShadowNode* {
    napi_value js_recv = base::NapiUtil::GetReferenceNapiValue(env_, js_);
    napi_value create = base::NapiUtil::GetReferenceNapiValue(env_, create_);
    if (!js_recv || !create) {
      return nullptr;
    }
    size_t argc = 2;
    napi_value argv[argc];
    napi_create_int32(env_, sign, &argv[0]);
    napi_create_string_latin1(env_, tag.data(), NAPI_AUTO_LENGTH, &argv[1]);
    napi_value result;
    napi_call_function(env_, js_recv, create, argc, argv, &result);
    napi_valuetype type;
    napi_typeof(env_, result, &type);
    if (type == napi_undefined) {
      return nullptr;
    }
    JSShadowNode* node;
    napi_unwrap(env_, result, reinterpret_cast<void**>(&node));
    return node;
  };

  ShadowNode* result = nullptr;
  NativeCallJSTask(
      [&result, runnable = std::move(runnable)]() { result = runnable(); });
  return result;
}

void ShadowNodeOwner::DestroyNode(int sign) {
  if (const auto& it = node_holder_.find(sign); it != node_holder_.end()) {
    DestroyNode(it->second);
    node_holder_.erase(it);
  }
}

void ShadowNodeOwner::Destroy() {
  for (auto& [_, node] : node_holder_) {
    DestroyNode(node);
  }
  node_holder_.clear();
}

void ShadowNodeOwner::DestroyNode(const fml::RefPtr<ShadowNode>& node) {
  node->MarkDestroyed();
  if (node->HasJSObject()) {
    context_->RunOnUIThread([node] { node->Destroy(); });
  } else {
    node->Destroy();
  }
}

void ShadowNodeOwner::NativeCallJSTask(base::closure task, bool sync) {
  if (sync) {
    context_->GetUITaskRunner()->PostSyncTask(std::move(task));
  } else {
    context_->GetUITaskRunner()->PostTask(std::move(task));
  }
}

void ShadowNodeOwner::JSCallNativeTask(base::closure task, bool sync) {
  if (sync) {
    context_->GetLayoutTaskRunner()->PostSyncTask(std::move(task));
  } else {
    context_->GetLayoutTaskRunner()->PostTask(std::move(task));
  }
}

void ShadowNodeOwner::FindShadowNodeAndRunTask(
    int sign, base::MoveOnlyClosure<void, ShadowNode*> task) {
  context_->RunOnLayoutThread(
      [weak_self = weak_from_this(), task = std::move(task), sign] {
        auto strong_self = weak_self.lock();
        if (strong_self) {
          if (const auto& it = strong_self->node_holder_.find(sign);
              it != strong_self->node_holder_.end()) {
            task(it->second.get());
          }
        }
      });
}

fml::RefPtr<fml::RefCountedThreadSafeStorage> ShadowNodeOwner::GetExtraBundle(
    int sign) {
  auto it = node_holder_.find(sign);
  if (it == node_holder_.end()) {
    return nullptr;
  }
  return it->second->getExtraBundle();
}

napi_value ShadowNodeOwner::New(napi_env env, napi_callback_info info) {
  napi_status status;
  /**
   * 0 - js object ref
   * 1 - create func ref
   */
  size_t argc = 3;
  napi_value args[argc];
  napi_value js_this;
  status = napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
  assert(status == napi_ok);

  napi_valuetype valuetype;
  status = napi_typeof(env, args[0], &valuetype);
  assert(status == napi_ok);

  // Share lifttime with JS instance, JS object retain the native instance.
  ShadowNodeOwner* obj = new ShadowNodeOwner();

  obj->env_ = env;

  // C++ own this object, LayoutContextHarmony takes the ownership of
  // ShadowNodeOwner.
  napi_create_reference(env, args[0], 1, &obj->js_);
  napi_create_reference(env, args[1], 1, &obj->create_);

  napi_wrap(
      env, js_this, obj, [](napi_env env, void* data, void* hint) {}, nullptr,
      nullptr);

  assert(status == napi_ok);

  return js_this;
}

napi_value ShadowNodeOwner::FindJSShadowNodeBySign(napi_env env,
                                                   napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[argc];
  napi_value js_this;
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  int sign = -1;
  napi_get_value_int32(env, argv[0], &sign);
  ShadowNodeOwner* node_owner = nullptr;
  napi_unwrap(env, js_this, reinterpret_cast<void**>(&node_owner));

  if (!node_owner) {
    return nullptr;
  }
  const auto* node = node_owner->FindShadowNodeBySign(sign);
  if (node) {
    return node->GetJSObject();
  }
  return nullptr;
}

napi_value ShadowNodeOwner::MeasureLayoutNode(napi_env env,
                                              napi_callback_info info) {
  constexpr int INDEX_SIGNATURE = 0;
  constexpr int INDEX_WIDTH = 1;
  constexpr int INDEX_WIDTH_MODE = 2;
  constexpr int INDEX_HEIGHT = 3;
  constexpr int INDEX_HEIGHT_MODE = 4;
  constexpr int INDEX_FINAL_MEASURE = 5;

  napi_value js_this;
  size_t argc = 6;
  napi_value argv[argc];
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  ShadowNodeOwner* node_owner = nullptr;
  napi_unwrap(env, js_this, reinterpret_cast<void**>(&node_owner));

  if (!node_owner) {
    return nullptr;
  }

  int sign;
  napi_get_value_int32(env, argv[INDEX_SIGNATURE], &sign);
  const auto& it = node_owner->node_holder_.find(sign);
  if (it == node_owner->node_holder_.end()) {
    return nullptr;
  }

  double width;
  napi_get_value_double(env, argv[INDEX_WIDTH], &width);
  int32_t width_mode;
  napi_get_value_int32(env, argv[INDEX_WIDTH_MODE], &width_mode);

  double height;
  napi_get_value_double(env, argv[INDEX_HEIGHT], &height);
  int32_t height_mode;
  napi_get_value_int32(env, argv[INDEX_HEIGHT_MODE], &height_mode);

  bool final_measure = false;
  napi_get_value_bool(env, argv[INDEX_FINAL_MEASURE], &final_measure);
  const auto& node = it->second;
  LayoutResult size = node->MeasureLayoutNode(
      static_cast<float>(width), static_cast<MeasureMode>(width_mode),
      static_cast<float>(height), static_cast<MeasureMode>(height_mode),
      final_measure);
  float measure_result[3] = {size.width_, size.height_, size.baseline_};
  return base::NapiUtil::CreateArrayBuffer(env, measure_result,
                                           3 * sizeof(float));
}

napi_value ShadowNodeOwner::AlignLayoutNode(napi_env env,
                                            napi_callback_info info) {
  constexpr int INDEX_SIGNATURE = 0;
  constexpr int INDEX_TOP = 1;
  constexpr int INDEX_LEFT = 2;

  napi_value js_this;
  size_t argc = 3;
  napi_value argv[argc];
  napi_get_cb_info(env, info, &argc, argv, &js_this, nullptr);
  ShadowNodeOwner* node_owner = nullptr;
  napi_unwrap(env, js_this, reinterpret_cast<void**>(&node_owner));

  if (!node_owner) {
    return nullptr;
  }

  int sign;
  napi_get_value_int32(env, argv[INDEX_SIGNATURE], &sign);
  const auto& it = node_owner->node_holder_.find(sign);
  if (it == node_owner->node_holder_.end()) {
    return nullptr;
  }
  const auto& node = it->second;
  double top = 0.f;
  double left = 0.f;
  napi_get_value_double(env, argv[INDEX_TOP], &top);
  napi_get_value_double(env, argv[INDEX_LEFT], &left);
  node->AlignLayoutNode(static_cast<float>(top), static_cast<float>(left));
  return nullptr;
}

napi_value ShadowNodeOwner::Destroy(napi_env env, napi_callback_info info) {
  napi_value js_this;
  size_t argc = 0;
  napi_get_cb_info(env, info, &argc, nullptr, &js_this, nullptr);

  ShadowNodeOwner* obj;
  napi_status status =
      napi_remove_wrap(env, js_this, reinterpret_cast<void**>(&obj));
  NAPI_THROW_IF_FAILED_NULL(env, status,
                            "ShadowNodeOwner napi_remove_wrap failed!");
  obj->context_->ResetNodeOwner();
  napi_delete_reference(env, obj->js_);
  napi_delete_reference(env, obj->create_);
  napi_delete_reference(env, obj->destroy_);
  obj->js_ = nullptr;
  obj->create_ = nullptr;
  obj->destroy_ = nullptr;
  obj->env_ = nullptr;
  return nullptr;
}

ShadowNode* ShadowNodeOwner::FindShadowNodeBySign(int sign) const {
  if (const auto it = node_holder_.find(sign); it != node_holder_.end()) {
    return it->second.get();
  }
  return nullptr;
}

void ShadowNodeOwner::AddFontFace(std::string font_family, FontFace font_face) {
  font_face_manager_->AddFontFace(std::move(font_family), std::move(font_face));
}

void ShadowNodeOwner::UpdateRootSize(float width, float height) {
  if (base::FloatsNotEqual(width, root_width_) ||
      base::FloatsNotEqual(height, root_height_)) {
    root_width_ = width;
    root_height_ = height;
    context_->RunOnUIThread([weak_owner = weak_from_this(), width, height] {
      auto strong_owner = weak_owner.lock();
      if (strong_owner && strong_owner->js_) {
        napi_value args[2];
        napi_create_double(strong_owner->env_, width, &args[0]);
        napi_create_double(strong_owner->env_, height, &args[1]);
        base::NapiUtil::InvokeJsMethod(strong_owner->env_, strong_owner->js_,
                                       "updateRootSize", 2, args);
      }
    });
  }
}

const fml::RefPtr<fml::TaskRunner>& ShadowNodeOwner::GetLayoutTaskRunner()
    const {
  return context_->GetLayoutTaskRunner();
}

const std::shared_ptr<LayoutVSyncProxy>&
ShadowNodeOwner::GetLayoutVSyncProxy() {
  if (!vsync_proxy_) {
    const auto& layout_task_runner = GetLayoutTaskRunner();
    if (layout_task_runner) {
      layout_task_runner->PostSyncTask(
          [this]() { vsync_proxy_ = std::make_shared<LayoutVSyncProxy>(); });
    } else {
      vsync_proxy_ = std::make_shared<LayoutVSyncProxy>();
    }
  }

  return vsync_proxy_;
}

const std::shared_ptr<base::VSyncMonitor>& ShadowNodeOwner::VSyncMonitor() {
  return GetLayoutVSyncProxy()->VSyncMonitor();
}

void ShadowNodeOwner::NotifySystemFontUpdated() {
  LOGI(
      "ShadowNodeOwner::NotifySystemFontUpdating size:" << node_holder_.size());
  for (auto& [id, node] : node_holder_) {
    if (node->IsTextShadowNode()) {
      node->MarkDirty();
    }
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
