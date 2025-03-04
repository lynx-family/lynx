// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_LAYOUT_EMPTY_LAYOUT_CONTEXT_EMPTY_IMPLEMENTATION_H_
#define CORE_RENDERER_UI_WRAPPER_LAYOUT_EMPTY_LAYOUT_CONTEXT_EMPTY_IMPLEMENTATION_H_

#include <memory>
#include <string>
#include <unordered_set>

#include "core/public/layout_ctx_platform_impl.h"
#include "core/renderer/ui_wrapper/layout/layout_context.h"

namespace lynx {
namespace tasm {

class DelegateEmptyImpl : public LayoutContext::Delegate {
 public:
  virtual void OnLayoutUpdate(int tag, float x, float y, float width,
                              float height,
                              const std::array<float, 4>& paddings,
                              const std::array<float, 4>& margins,
                              const std::array<float, 4>& borders,
                              const std::array<float, 4>* sticky_positions,
                              float max_height) override {}
  virtual void OnLayoutAfter(const PipelineOptions& options,
                             std::unique_ptr<PlatformExtraBundleHolder> holder,
                             bool has_layout) override {}
  virtual void OnNodeLayoutAfter(int32_t id) override {}
  virtual void PostPlatformExtraBundle(
      int32_t id, std::unique_ptr<tasm::PlatformExtraBundle> bundle) override {}
  virtual void OnCalculatedViewportChanged(const CalculatedViewport& viewport,
                                           int tag) override {}
  virtual void SetTiming(tasm::Timing timing) override {}
  virtual void OnFirstMeaningfulLayout() override {}
};

class PlatformImplEmptyImpl : public LayoutCtxPlatformImpl {
 public:
  virtual int CreateLayoutNode(int sign, const std::string& tag,
                               PropBundle* props, bool allow_inline) override {
    return 1;
  }
  virtual void UpdateLayoutNode(int sign, PropBundle* props) override {}
  virtual void InsertLayoutNode(int parent, int child, int index) override {}
  virtual void RemoveLayoutNode(int parent, int child, int index) override {}
  virtual void MoveLayoutNode(int parent, int child, int from_index,
                              int to_index) override {}
  virtual void DestroyLayoutNodes(const std::unordered_set<int>& ids) override {
  }
  virtual void ScheduleLayout(base::closure callback) override {}
  virtual void OnLayoutBefore(int sign) override {}
  virtual void OnLayout(int id, float left, float top, float width,
                        float height, const std::array<float, 4>& paddings,
                        const std::array<float, 4>& borders) override {}
  virtual void Destroy() override {}
  virtual void SetFontFaces(const CSSFontFaceRuleMap&) override {}
  virtual void SetLayoutNodeManager(
      LayoutNodeManager* layout_node_manager) override {}
};

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_UI_WRAPPER_LAYOUT_EMPTY_LAYOUT_CONTEXT_EMPTY_IMPLEMENTATION_H_
