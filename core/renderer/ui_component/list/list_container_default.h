// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_LIST_CONTAINER_DEFAULT_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_LIST_CONTAINER_DEFAULT_H_

#include <vector>

#include "core/renderer/ui_component/list/list_container.h"

namespace lynx {
namespace tasm {

class ListContainerDefault : public ListContainer::Delegate {
 public:
  ListContainerDefault() = default;
  ~ListContainerDefault() override = default;

  bool ResolveAttribute(const base::String& key,
                        const lepus::Value& value) override {
    return true;
  }
  void FinishBindItemHolder(Element* component,
                            const PipelineOptions& option) override {}
  void FinishBindItemHolders(const std::vector<Element*>& list_items,
                             const PipelineOptions& options) override {}
  void OnLayoutChildren() override {}
  void ScrollByPlatformContainer(float content_offset_x, float content_offset_y,
                                 float original_x, float original_y) override {}
  void ScrollToPosition(int index, float offset, int align,
                        bool smooth) override {}
  void ScrollStopped() override {}
  void UpdateListContainerDataSource(
      fml::RefPtr<lepus::Dictionary>& list_container_info) override {}
  void AddEvent(const base::String& name) override {}
  void ClearEvents() override {}
  void ResolveListAxisGap(CSSPropertyID id,
                          const lepus::Value& value) override {}
  void PropsUpdateFinish() override {}
  void OnListItemLayoutUpdated(Element* component) override {}
  void UpdateBatchRenderStrategy(list::BatchRenderStrategy strategy) {}
  list::BatchRenderStrategy GetBatchRenderStrategy() {
    return list::BatchRenderStrategy::kDefault;
  }

 protected:
  // Currently, the list container does not copy any member variables and is an
  // empty implementation.
  ListContainerDefault(const ListContainerDefault& list_container_default) {}
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_LIST_CONTAINER_DEFAULT_H_
