// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/public/pub_ui_owner.h"

#include <node_api.h>

#include <string>
#include <utility>

#include "platform/harmony/lynx_harmony/src/main/cpp/public/pub_lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/native_node_content.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_root.h"

namespace lynx {
namespace tasm {
namespace harmony {

PubUIOwner::PubUIOwner(napi_env env, napi_value ui_owner) {
  UIOwner* ptr = nullptr;
  napi_unwrap(env, ui_owner, reinterpret_cast<void**>(&ptr));
  ui_owner_ = std::shared_ptr<UIOwner>(ptr);
}

void PubUIOwner::CreateUI(int sign, const std::string& tag,
                          PubPropBundleHarmony* painting_data,
                          uint32_t node_index) const {
  ui_owner_->CreateUI(sign, tag, painting_data->PropBundle(), node_index);
}

void PubUIOwner::InsertUI(int parent, int child, int index) const {
  ui_owner_->InsertUI(parent, child, index);
}

void PubUIOwner::RemoveUI(int parent, int child, int index,
                          const bool is_move) const {
  ui_owner_->RemoveUI(parent, child, index, is_move);
}

void PubUIOwner::UpdateUI(int sign, PubPropBundleHarmony* props) const {
  ui_owner_->UpdateUI(sign, props->PropBundle());
}

void PubUIOwner::DestroyUI(int parent, int child, int index) const {
  ui_owner_->DestroyUI(parent, child, index);
}

void PubUIOwner::OnNodeReady(int sign) const { ui_owner_->OnNodeReady(sign); }

void PubUIOwner::UpdateLayout(int sign, float left, float top, float width,
                              float height, const float* paddings,
                              const float* margins, const float* sticky,
                              float max_height,
                              const uint32_t node_index) const {
  ui_owner_->UpdateLayout(sign, left, top, width, height, paddings, margins,
                          sticky, max_height, node_index);
}

void PubUIOwner::OnLayoutFinish(int32_t component_id,
                                int64_t operation_id) const {
  ui_owner_->OnLayoutFinish(component_id, operation_id);
}

void PubUIOwner::AttachPageRoot(napi_env env, napi_value root_content) const {
  NativeNodeContent* native_node_content;
  napi_unwrap(env, root_content,
              reinterpret_cast<void**>(&native_node_content));
  ui_owner_->AttachPageRoot(native_node_content);
}

void PubUIOwner::SetContext(PubLynxContext* context) const {
  ui_owner_->SetContext(context->Context());
}

void PubUIOwner::UpdateExtraData(
    int sign,
    const fml::RefPtr<fml::RefCountedThreadSafeStorage>& extra_data) const {
  ui_owner_->UpdateExtraData(sign, extra_data);
}

void PubUIOwner::InvokeUIMethod(
    int32_t id, const std::string& method, const pub::Value& args,
    base::MoveOnlyClosure<void, int32_t, const pub::Value&> callback) const {
  ui_owner_->InvokeUIMethod(id, method, args, std::move(callback));
}

int PubUIOwner::IndexOf(int child_id) const {
  UIBase* child_node = ui_owner_->FindUIBySign(child_id);
  if (!child_node) {
    return -1;
  }
  UIBase* parent_node = child_node->Parent();
  if (parent_node) {
    const auto& children = parent_node->Children();
    for (size_t index = 0; index < children.size(); index++) {
      if (children[index] == child_node) {
        return index;
      }
    }
  }
  return -1;
}

int PubUIOwner::GetUINodeByPosition(float x, float y) const {
  UIRoot* root = ui_owner_->Root();
  float pos[2] = {x / root->GetContext()->ScaledDensity(),
                  y / root->GetContext()->ScaledDensity()};
  EventTarget* ui = root->HitTest(pos);
  return ui ? ui->Sign() : -1;
}

std::string PubUIOwner::GetTag(int sign) const {
  UIBase* ui = ui_owner_->FindUIBySign(sign);
  return ui ? ui->Tag() : std::string();
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
