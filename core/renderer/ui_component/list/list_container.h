// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_LIST_CONTAINER_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_LIST_CONTAINER_H_

#include <memory>
#include <vector>

#include "core/renderer/dom/element.h"
#include "core/renderer/ui_component/list/list_types.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {

class ListContainer {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual bool ResolveAttribute(const base::String& key,
                                  const lepus::Value& value) = 0;
    virtual void OnLayoutChildren() = 0;
    virtual void OnNextFrame() {}
    virtual void FinishBindItemHolder(Element* component,
                                      const PipelineOptions& option) = 0;
    virtual void FinishBindItemHolders(const std::vector<Element*>& list_items,
                                       const PipelineOptions& options) = 0;
    virtual void ScrollByPlatformContainer(float content_offset_x,
                                           float content_offset_y,
                                           float original_x,
                                           float original_y) = 0;
    virtual void ScrollToPosition(int index, float offset, int align,
                                  bool smooth) = 0;
    virtual void ScrollStopped() = 0;
    virtual void UpdateListContainerDataSource(
        fml::RefPtr<lepus::Dictionary>& list_container_info) = 0;
    virtual void AddEvent(const base::String& name) = 0;
    virtual void ClearEvents() = 0;
    virtual void ResolveListAxisGap(CSSPropertyID id,
                                    const lepus::Value& value) = 0;
    virtual void PropsUpdateFinish() = 0;
    virtual void OnListItemLayoutUpdated(Element* component) = 0;
    virtual void UpdateBatchRenderStrategy(
        list::BatchRenderStrategy strategy) = 0;
    virtual list::BatchRenderStrategy GetBatchRenderStrategy() = 0;
  };
  ListContainer(Element* element);
  ~ListContainer() = default;
  std::unique_ptr<Delegate>& list_container_delegate() {
    return list_container_delegate_;
  }

 protected:
  // Currently, the list container does not copy any member variables and is an
  // empty implementation.
  ListContainer(const ListContainer& list_container) {}

 private:
  std::unique_ptr<Delegate> list_container_delegate_;
};

namespace list {
std::unique_ptr<ListContainer::Delegate> CreateListContainerDelegate(
    Element* element);

bool IsInDebugMode();
}  // namespace list

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_LIST_CONTAINER_H_
