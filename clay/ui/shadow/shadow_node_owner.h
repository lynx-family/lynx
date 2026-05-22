// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_SHADOW_NODE_OWNER_H_
#define CLAY_UI_SHADOW_SHADOW_NODE_OWNER_H_

#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/fml/memory/weak_ptr.h"
#include "clay/gfx/geometry/size.h"
#include "clay/public/layout_delegate.h"
#include "clay/ui/common/measure_constraint.h"

namespace clay {

class BaseView;
class ViewContext;
class RenderDelegate;
class ShadowNode;

class ShadowNodeOwner {
 public:
  explicit ShadowNodeOwner(fml::RefPtr<fml::TaskRunner> ui_task_runner);
  ~ShadowNodeOwner() { context_ = nullptr; }

  float Logical2ClayPixelRatio();

  void SetViewContext(ViewContext* context) { context_ = context; }
  ViewContext* GetViewContext() const { return context_; }

  void ScheduleLayout();
  void TriggerLayout();

  void SetDelegate(RenderDelegate* delegate) { delegate_ = delegate; }
  void SetLayoutDelegate(LayoutDelegate* delegate) {
    layout_delegate_ = delegate;
  }

  fml::RefPtr<fml::TaskRunner> GetUITaskRunner() const {
    return ui_task_runner_;
  }

  BaseView* FindViewByViewId(int id);

  uint32_t GetContextId() const;

  Size ViewportSize() const;

  void AddNode(int id, ShadowNode* node);
  void RemoveNode(int id);
  ShadowNode* GetNode(int id);
  void ClearNodes();

  void MarkDirty(ShadowNode* node) const;
  ClayLayoutStyles GetLayoutStyles(ShadowNode* node) const;
  MeasureResult MeasureNativeNode(ShadowNode* node,
                                  const MeasureConstraint& constraint) const;
  void AlignNativeNode(ShadowNode* node, float top, float left) const;

 private:
  fml::RefPtr<fml::TaskRunner> ui_task_runner_;
  ViewContext* context_ = nullptr;
  RenderDelegate* delegate_ = nullptr;
  LayoutDelegate* layout_delegate_ = nullptr;

  std::unordered_map<int, ShadowNode*> shadow_node_map_;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_SHADOW_NODE_OWNER_H_
