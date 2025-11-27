// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_LAYOUT_CONTEXT_CLAY_H_
#define CLAY_LYNX_ADAPTOR_LAYOUT_CONTEXT_CLAY_H_

#include <memory>
#include <string>
#include <unordered_set>

#include "clay/public/layout_delegate.h"
#include "clay/ui/component/view_context.h"
#include "core/public/layout_ctx_platform_impl.h"
#include "core/public/layout_node_value.h"
#include "core/public/lynx_engine_proxy.h"
#include "core/public/lynx_layout_proxy.h"

namespace lynx {
namespace tasm {

class LayoutContextClay : public LayoutCtxPlatformImpl,
                          public clay::LayoutDelegate {
 public:
  explicit LayoutContextClay(clay::ViewContext* view_context);
  ~LayoutContextClay() override;

  void SetEngineProxy(
      const std::shared_ptr<shell::LynxEngineProxy>& engine_proxy) {
    engine_proxy_ = engine_proxy;
  }

  void SetLayoutProxy(
      const std::shared_ptr<shell::LynxLayoutProxy>& layout_proxy) {
    layout_proxy_ = layout_proxy;
  }

  void SetLayoutNodeManager(LayoutNodeManager* layout_node_manager) override;

  int CreateLayoutNode(int id, const std::string& tag, PropBundle* props,
                       bool allow_inline) override;
  void UpdateLayoutNode(int id, PropBundle* painting_data) override;
  void OnLayoutBefore(int tag) override;
  void ScheduleLayout() override;
  void SetFontFaces(const CSSFontFaceRuleMap& fontfaces) override;
  void InsertLayoutNode(int parent, int child, int index) override;
  void RemoveLayoutNode(int parent, int child, int index) override;
  void OnLayout(int tag, float left, float top, float width, float height,
                const std::array<float, 4>& paddings,
                const std::array<float, 4>& borders) override;
  void UpdateRootSize(float width, float height) override;
  void DestroyLayoutNodes(const std::unordered_set<int>& ids) override;
  void Destroy() override;
  std::unique_ptr<PlatformExtraBundle> GetPlatformExtraBundle(
      int32_t id) override;

 private:
  // clay::LayoutDelegate
  void OnTriggerLayout() override;
  void OnMarkDirty(int32_t id) override;
  void OnAlignNativeNode(int32_t id, float offset_top,
                         float offset_left) override;
  ClayMeasureOutput OnMeasureNativeNode(int32_t id, float width, int width_mode,
                                        float height, int height_mode) override;
  ClayLayoutStyles OnGetLayoutStyles(int32_t id) override;

  friend class MeasureFuncImpl;
  LayoutResult MeasureImpl(int sign, int width, int width_mode, int height,
                           int height_mode);
  void AlignmentImpl(int sign);

  void SetAttribute(int sign, PropBundle* attributes);

  bool has_pending_layout_ = true;
  clay::ViewContext* view_context_ = nullptr;
  LayoutNodeManager* layout_node_manager_ = nullptr;
  std::shared_ptr<shell::LynxEngineProxy> engine_proxy_ = nullptr;
  std::shared_ptr<shell::LynxLayoutProxy> layout_proxy_ = nullptr;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_LAYOUT_CONTEXT_CLAY_H_
