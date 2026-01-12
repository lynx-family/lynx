// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_LIST_CONTAINER_DELEGATE_H_
#define CORE_LIST_LIST_CONTAINER_DELEGATE_H_

#include <memory>
#include <vector>

#include "core/list/list_element_delegate.h"
#include "core/list/list_item_element_delegate.h"
#include "core/public/pipeline_option.h"
#include "core/public/pub_value.h"
#include "core/renderer/css/css_property_id.h"

namespace lynx {
namespace list {

class ContainerDelegate {
 public:
  virtual ~ContainerDelegate() = default;

  virtual void OnAttachToElementManager() = 0;
  virtual bool ResolveAttribute(const pub::Value& key,
                                const pub::Value& value) = 0;
  virtual void ResolveListAxisGap(tasm::CSSPropertyID id, float gap) = 0;
  virtual void PropsUpdateFinish() = 0;
  virtual void OnLayoutChildren(
      const std::shared_ptr<tasm::PipelineOptions>& options) = 0;
  virtual void FinishBindItemHolder(
      ItemElementDelegate* list_item_delegate,
      const std::shared_ptr<tasm::PipelineOptions>& options) = 0;
  virtual void FinishBindItemHolders(
      const std::vector<ItemElementDelegate*>& list_item_delegate_array,
      const std::shared_ptr<tasm::PipelineOptions>& options) = 0;
  virtual void ScrollByPlatformContainer(float content_offset_x,
                                         float content_offset_y,
                                         float original_x,
                                         float original_y) = 0;
  virtual void OnListItemLayoutUpdated(ItemElementDelegate* list_item) = 0;
  virtual void OnAttachedToElementManager() = 0;
  virtual void ScrollToPosition(int index, float offset, int align,
                                bool smooth) = 0;
  virtual void ScrollStopped() = 0;
  virtual void EnableInsertPlatformView() = 0;
  virtual void OnNextFrame() = 0;
  virtual void SetEnableBatchRender(bool enable_batch_render) = 0;
};

std::unique_ptr<ContainerDelegate> CreateListContainerDelegate(
    ElementDelegate* list_delegate,
    const std::shared_ptr<pub::PubValueFactory>& value_factory);

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_LIST_CONTAINER_DELEGATE_H_
