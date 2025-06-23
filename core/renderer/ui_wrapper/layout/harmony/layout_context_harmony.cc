// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/ui_wrapper/layout/harmony/layout_context_harmony.h"

#include <utility>

#include "core/renderer/ui_wrapper/common/harmony/platform_extra_bundle_harmony.h"

namespace lynx {
namespace tasm {

LayoutContextHarmony::~LayoutContextHarmony() = default;

void LayoutContextHarmony::SetLayoutNodeManager(
    LayoutNodeManager* layout_node_manager) {
  node_owner_->SetLayoutNodeManager(layout_node_manager);
}

int LayoutContextHarmony::CreateLayoutNode(int id, const std::string& tag,
                                           PropBundle* painting_data,
                                           bool allow_inline) {
  return node_owner_->CreateShadowNode(
      id, tag, reinterpret_cast<PropBundleHarmony*>(painting_data),
      allow_inline);
}

void LayoutContextHarmony::InsertLayoutNode(int parent, int child, int index) {
  node_owner_->InsertLayoutNode(parent, child, index);
}

void LayoutContextHarmony::RemoveLayoutNode(int parent, int child, int index) {
  node_owner_->RemoveLayoutNode(parent, child, index);
}

void LayoutContextHarmony::MoveLayoutNode(int parent, int child, int from_index,
                                          int to_index) {
  node_owner_->MoveLayoutNode(parent, child, from_index, to_index);
}

void LayoutContextHarmony::UpdateLayoutNode(int id, PropBundle* painting_data) {
  node_owner_->UpdateLayoutNode(
      id, reinterpret_cast<PropBundleHarmony*>(painting_data));
}

void LayoutContextHarmony::OnLayoutBefore(int id) {
  node_owner_->OnLayoutBefore(id);
}

void LayoutContextHarmony::OnLayout(int id, float left, float top, float width,
                                    float height,
                                    const std::array<float, 4>& paddings,
                                    const std::array<float, 4>& borders) {
  node_owner_->OnLayout(id, left, top, width, height);
}

void LayoutContextHarmony::ScheduleLayout(base::closure callback) {
  node_owner_->ScheduleLayout(std::move(callback));
}

void LayoutContextHarmony::DestroyLayoutNodes(
    const std::unordered_set<int>& ids) {
  for (const int& node : ids) {
    node_owner_->DestroyNode(node);
  }
}

void LayoutContextHarmony::Destroy() { node_owner_->Destroy(); }

void LayoutContextHarmony::SetFontFaces(const CSSFontFaceRuleMap& fontfaces) {
  for (const auto& font : fontfaces) {
    auto& font_family = font.first;
    harmony::FontFace font_face(font_family);

    // type: std::vector<std::shared_ptr<CSSFontFaceRule>>
    for (const auto& font_face_token : font.second) {
      // type: std::pair<font-family, CSSFontFaceAttrsMap>
      for (const auto& font_face_attr : font_face_token->second) {
        // type: std::unordered_map<key, std::string>, key may: 'src' or
        // 'font-family'
        static constexpr const char* kSrcKey = "src";
        if (font_face_attr.first == kSrcKey) {
          font_face.ParseAndAddSrc(font_face_attr.second);
        }
      }
    }
    if (!font_face.GetSrc().empty()) {
      node_owner_->AddFontFace(font_family, std::move(font_face));
    }
  }
}

std::unique_ptr<PlatformExtraBundle>
LayoutContextHarmony::GetPlatformExtraBundle(int32_t id) {
  auto const& bundle = node_owner_->GetExtraBundle(id);
  if (bundle == nullptr) return nullptr;

  return std::make_unique<PlatformExtraBundleHarmony>(id, nullptr,
                                                      std::move(bundle));
}

void LayoutContextHarmony::UpdateRootSize(float width, float height) {
  node_owner_->UpdateRootSize(width, height);
}

}  // namespace tasm
}  // namespace lynx
