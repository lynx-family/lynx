// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_DECOUPLED_LIST_CONTAINER_DEFAULT_H_
#define CORE_LIST_DECOUPLED_LIST_CONTAINER_DEFAULT_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/list/list_container_delegate.h"
#include "core/list/list_element_delegate.h"
#include "core/list/list_item_element_delegate.h"

namespace lynx {
namespace list {

class ListContainerDefault : public ContainerDelegate {
 public:
  ListContainerDefault(
      ElementDelegate* list_delegate,
      const std::shared_ptr<pub::PubValueFactory>& value_factory) {}

  ~ListContainerDefault() override {}

  bool ResolveAttribute(const pub::Value& key,
                        const pub::Value& value) override {}

  void ResolveListAxisGap(lynx::tasm::CSSPropertyID id, float gap) override {}

  void PropsUpdateFinish() override {}

  void OnLayoutChildren(
      const std::shared_ptr<tasm::PipelineOptions>& options) override {}

  void FinishBindItemHolder(
      ItemElementDelegate* list_item_delegate,
      const std::shared_ptr<tasm::PipelineOptions>& option) override {}

  void ScrollByPlatformContainer(float content_offset_x, float content_offset_y,
                                 float original_x, float original_y) override {}

  void OnListItemLayoutUpdated(
      ItemElementDelegate* list_item_delegate) override {}

  void OnAttachedToElementManager() override {}

  void ScrollToPosition(int index, float offset, int align,
                        bool smooth) override {}

  void ScrollStopped() override {}

  void EnableInsertPlatformView() override {}
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_DECOUPLED_LIST_CONTAINER_DEFAULT_H_
