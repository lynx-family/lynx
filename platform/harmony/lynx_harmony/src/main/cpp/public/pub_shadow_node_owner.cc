// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/public/pub_shadow_node_owner.h"

#include <node_api.h>

#include <string>
#include <utility>

#include "platform/harmony/lynx_harmony/src/main/cpp/public/pub_lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node_owner.h"

namespace lynx {
namespace tasm {
namespace harmony {

PubShadowNodeOwner::PubShadowNodeOwner(napi_env env,
                                       napi_value shadow_node_owner) {
  ShadowNodeOwner* ptr = nullptr;
  napi_unwrap(env, shadow_node_owner, reinterpret_cast<void**>(&ptr));
  shadow_node_owner_ = std::shared_ptr<ShadowNodeOwner>(ptr);
}

int PubShadowNodeOwner::CreateShadowNode(int sign, const std::string& tag,
                                         PubPropBundleHarmony* painting_data,
                                         bool allow_inline) const {
  return shadow_node_owner_->CreateShadowNode(
      sign, tag, painting_data->PropBundle(), allow_inline);
}

void PubShadowNodeOwner::RemoveLayoutNode(int parent, int child,
                                          int index) const {
  shadow_node_owner_->RemoveLayoutNode(parent, child, index);
}

void PubShadowNodeOwner::InsertLayoutNode(int parent, int child,
                                          int index) const {
  shadow_node_owner_->InsertLayoutNode(parent, child, index);
}

void PubShadowNodeOwner::MoveLayoutNode(int parent, int child, int from_index,
                                        int to_index) const {
  shadow_node_owner_->MoveLayoutNode(parent, child, from_index, to_index);
}

void PubShadowNodeOwner::DestroyNode(int sign) const {
  shadow_node_owner_->DestroyNode(sign);
}

void PubShadowNodeOwner::Destroy() const { shadow_node_owner_->Destroy(); }

void PubShadowNodeOwner::OnLayoutBefore(int id) const {
  shadow_node_owner_->OnLayoutBefore(id);
}

void PubShadowNodeOwner::OnLayout(int id, float left, float top, float width,
                                  float height) const {
  shadow_node_owner_->OnLayout(id, left, top, width, height);
}

fml::RefPtr<fml::RefCountedThreadSafeStorage>
PubShadowNodeOwner::GetExtraBundle(int id) const {
  return shadow_node_owner_->GetExtraBundle(id);
}

void PubShadowNodeOwner::UpdateLayoutNode(
    int id, PubPropBundleHarmony* painting_data) const {
  shadow_node_owner_->UpdateLayoutNode(id, painting_data->PropBundle());
}

void PubShadowNodeOwner::SetLayoutNodeManager(
    LayoutNodeManager* layout_node_manager) const {
  shadow_node_owner_->SetLayoutNodeManager(layout_node_manager);
}

void PubShadowNodeOwner::SetContext(PubLynxContext* context) {
  shadow_node_owner_->SetContext(context->Context());
}

void PubShadowNodeOwner::ScheduleLayout(base::closure callback) const {
  shadow_node_owner_->ScheduleLayout(std::move(callback));
}

LayoutResult PubShadowNodeOwner::MeasureNode(int id, float width,
                                             int32_t width_mode, float height,
                                             int32_t height_mode,
                                             bool final_measure) {
  return shadow_node_owner_->MeasureNode(id, width, width_mode, height,
                                         height_mode, final_measure);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
