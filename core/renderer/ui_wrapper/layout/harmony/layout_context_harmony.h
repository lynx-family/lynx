// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_LAYOUT_HARMONY_LAYOUT_CONTEXT_HARMONY_H_
#define CORE_RENDERER_UI_WRAPPER_LAYOUT_HARMONY_LAYOUT_CONTEXT_HARMONY_H_

#include <memory>
#include <string>
#include <unordered_set>

#include "core/public/layout_ctx_platform_impl.h"
#include "core/renderer/css/css_fragment.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node_owner.h"

namespace lynx {
namespace tasm {

class LayoutContextHarmony final : public LayoutCtxPlatformImpl {
 public:
  explicit LayoutContextHarmony(harmony::ShadowNodeOwner* node_owner)
      : node_owner_(node_owner) {}
  LayoutContextHarmony(const LayoutContextHarmony&) = delete;
  LayoutContextHarmony& operator=(const LayoutContextHarmony&) = delete;

  ~LayoutContextHarmony() override;

  void SetLayoutNodeManager(LayoutNodeManager* layout_node_manager) override;

  int CreateLayoutNode(int id, const std::string& tag, PropBundle* props,
                       bool allow_inline) override;
  void InsertLayoutNode(int parent, int child, int index) override;
  void RemoveLayoutNode(int parent, int child, int index) override;
  void MoveLayoutNode(int parent, int child, int from_index,
                      int to_index) override;
  void UpdateLayoutNode(int id, PropBundle* painting_data) override;
  void OnLayoutBefore(int tag) override;
  void OnLayout(int tag, float left, float top, float width, float height,
                const std::array<float, 4>& paddings,
                const std::array<float, 4>& borders) override;
  void ScheduleLayout(base::closure callback) override;
  void DestroyLayoutNodes(const std::unordered_set<int>& ids) override;
  void Destroy() override;
  void SetFontFaces(const CSSFontFaceRuleMap& fontfaces) override;
  void UpdateRootSize(float width, float height) override;
  std::unique_ptr<PlatformExtraBundle> GetPlatformExtraBundle(
      int32_t id) override;

 private:
  std::shared_ptr<harmony::ShadowNodeOwner> node_owner_;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_LAYOUT_HARMONY_LAYOUT_CONTEXT_HARMONY_H_
