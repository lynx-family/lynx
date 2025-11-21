// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/public/pub_ui_owner.h"

#include <node_api.h>

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/public/box_model.h"
#include "devtool/embedder/core/screenshot_thread_manager.h"
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
  env_ = env;
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

int32_t PubUIOwner::GetTagInfo(const std::string& tag) const {
  return ui_owner_->GetTagInfo(tag);
}

void PubUIOwner::UpdateContentOffsetForListContainer(
    int32_t container_id, float content_size, float delta_x, float delta_y,
    bool is_init_scroll_offset, bool from_layout) {
  ui_owner_->UpdateContentOffsetForListContainer(
      container_id, content_size, delta_x, delta_y, is_init_scroll_offset,
      from_layout);
}

void PubUIOwner::UpdateScrollInfo(int32_t container_id, bool smooth,
                                  float estimated_offset, bool scrolling) {
  ui_owner_->UpdateScrollInfo(container_id, smooth, estimated_offset,
                              scrolling);
}

void PubUIOwner::InsertListItemPaintingNode(int list_sign, int child_sign) {
  ui_owner_->InsertListItemPaintingNode(list_sign, child_sign);
}

void PubUIOwner::RemoveListItemPaintingNode(int list_sign, int child_sign) {
  ui_owner_->RemoveListItemPaintingNode(list_sign, child_sign);
}

void PubUIOwner::FetchTransformValue(
    int id, const std::vector<float>& pad_border_margin_layout,
    std::vector<float>& res) {
  std::vector<float> point(8, 0);
  auto ui = ui_owner_->FindUIBySign(id);
  if (!ui) {
    return;
  }

  for (int i = 0; i < 4; ++i) {
    if (i == 0) {
      ui->GetTransformValue(
          pad_border_margin_layout[BoxModelOffset::PAD_LEFT] +
              pad_border_margin_layout[BoxModelOffset::BORDER_LEFT] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_LEFT],
          -pad_border_margin_layout[BoxModelOffset::PAD_RIGHT] -
              pad_border_margin_layout[BoxModelOffset::BORDER_RIGHT] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_RIGHT],
          pad_border_margin_layout[BoxModelOffset::PAD_TOP] +
              pad_border_margin_layout[BoxModelOffset::BORDER_TOP] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_TOP],
          -pad_border_margin_layout[BoxModelOffset::PAD_BOTTOM] -
              pad_border_margin_layout[BoxModelOffset::BORDER_BOTTOM] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_BOTTOM],
          point);
    } else if (i == 1) {
      ui->GetTransformValue(
          pad_border_margin_layout[BoxModelOffset::BORDER_LEFT] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_LEFT],
          -pad_border_margin_layout[BoxModelOffset::BORDER_RIGHT] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_RIGHT],
          pad_border_margin_layout[BoxModelOffset::BORDER_TOP] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_TOP],
          -pad_border_margin_layout[BoxModelOffset::BORDER_BOTTOM] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_BOTTOM],
          point);
    } else if (i == 2) {
      ui->GetTransformValue(
          pad_border_margin_layout[BoxModelOffset::LAYOUT_LEFT],
          -pad_border_margin_layout[BoxModelOffset::LAYOUT_RIGHT],
          pad_border_margin_layout[BoxModelOffset::LAYOUT_TOP],
          -pad_border_margin_layout[BoxModelOffset::LAYOUT_BOTTOM], point);
    } else {
      ui->GetTransformValue(
          -pad_border_margin_layout[BoxModelOffset::MARGIN_LEFT] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_LEFT],
          pad_border_margin_layout[BoxModelOffset::MARGIN_RIGHT] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_RIGHT],
          -pad_border_margin_layout[BoxModelOffset::MARGIN_TOP] +
              pad_border_margin_layout[BoxModelOffset::LAYOUT_TOP],
          pad_border_margin_layout[BoxModelOffset::MARGIN_BOTTOM] -
              pad_border_margin_layout[BoxModelOffset::LAYOUT_BOTTOM],
          point);
    }
    res[i * 8] = point[0];
    res[i * 8 + 1] = point[1];
    res[i * 8 + 2] = point[2];
    res[i * 8 + 3] = point[3];
    res[i * 8 + 4] = point[4];
    res[i * 8 + 5] = point[5];
    res[i * 8 + 6] = point[6];
    res[i * 8 + 7] = point[7];
  }
}

void PubUIOwner::TakeSnapshot(size_t max_width, size_t max_height, int quality,
                              const TakeSnapshotCompletedCallback& callback) {
  auto context = ui_owner_->Context();
  if (!context) {
    return;
  }
  static lynx::base::NoDestructor<lynx::fml::Thread> screenshot_thread(
      "Lynx_SnapshotThread");
  context->TakeScreenShot(max_width, max_height, quality,
                          screenshot_thread->GetTaskRunner(), callback);
}

void PubUIOwner::SetGestures(int id, const pub::Value& gesture_array) {
  auto ui = ui_owner_->FindUIBySign(id);
  if (!ui) {
    return;
  }
  GestureMap gesture_map;
  gesture_array.ForeachArray([&](int64_t index, const pub::Value& value) {
    if (!value.IsMap()) {
      return;
    }
    int32_t gesture_id = value.GetValueForKey("id")->Number();
    int32_t gesture_type = value.GetValueForKey("type")->Number();

    auto callback_names = value.GetValueForKey("callbackNames");
    auto relation_map_value = value.GetValueForKey("relationMap");
    auto config_map = value.GetValueForKey("configMap");

    std::vector<GestureCallback> gesture_callback;
    callback_names->ForeachArray(
        [&](int64_t index, const pub::Value& callback_name) {
          if (!callback_name.IsString()) {
            return;
          }
          gesture_callback.emplace_back(callback_name.str(), lepus::Value(),
                                        lepus::Value());
        });
    std::unordered_map<std::string, std::vector<uint32_t>> relation_map;
    relation_map_value->ForeachMap(
        [&](const pub::Value& key, const pub::Value& value) {
          if (!key.IsString() || !value.IsArray()) {
            return;
          }
          std::vector<uint32_t> relation_ids;
          value.ForeachArray([&](int64_t index, const pub::Value& relation_id) {
            if (!relation_id.IsNumber()) {
              return;
            }
            relation_ids.push_back(static_cast<uint32_t>(relation_id.Number()));
          });
          relation_map[key.str()] = std::move(relation_ids);
        });

    gesture_map[gesture_id] = std::make_shared<GestureDetector>(
        gesture_id, static_cast<GestureType>(gesture_type),
        std::move(gesture_callback), std::move(relation_map),
        pub::ValueUtils::ConvertValueToLepusValue(*config_map));
  });
  ui->SetGestureDetectors(gesture_map);
}

void PubUIOwner::SetGestureState(int id, int gesture_id, int32_t state) {
  auto ui = ui_owner_->FindUIBySign(id);
  if (!ui) {
    return;
  }
  ui->SetGestureDetectorState(gesture_id, state);
}

void PubUIOwner::ConsumeGesture(int id, int gesture_id,
                                const pub::Value& params) {
  auto ui = ui_owner_->FindUIBySign(id);
  if (!ui) {
    return;
  }
  ui->ConsumeGesture(gesture_id,
                     pub::ValueUtils::ConvertValueToLepusValue(params));
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
