// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/mediator/list_mediator.h"

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

ListMediator::ListMediator(Element* list_element)
    : list_element_(list_element),
      list_container_delegate_(lynx::list::CreateListContainerDelegate(
          this, std::make_shared<pub::PubValueFactoryDefault>())) {}

int32_t ListMediator::GetImplId() const { return -1; }

float ListMediator::GetPhysicalPixelsPerLayoutUnit() const { return 1.f; }

float ListMediator::GetLayoutsUnitPerPx() const { return 1.f; }

void ListMediator::MarkListElementLayoutDirty() {}

void ListMediator::OnErrorOccurred(base::LynxError error) const {}

bool ListMediator::IsRTL() const { return false; }

float ListMediator::GetWidth() const { return 0.f; }

float ListMediator::GetHeight() const { return 0.f; }

const std::array<float, 4>& ListMediator::GetPaddings() const {
  return list_element_->paddings();
}

const std::array<float, 4>& ListMediator::GetMargins() const {
  return list_element_->margins();
}

const std::array<float, 4>& ListMediator::GetBorders() const {
  return list_element_->borders();
}

void ListMediator::FlushListContainerInfo(
    const std::string& list_container_info_str,
    std::unique_ptr<pub::Value> list_container_info) {}

void ListMediator::UpdateListLayoutNodeAttribute() {}

bool ListMediator::ComponentAtIndex(
    uint32_t index, int64_t operationId /* = 0 */,
    bool enable_reuse_notification /* = false */) {
  return false;
}

void ListMediator::EnqueueComponent(int32_t list_item_id) {}

void ListMediator::RemoveListItemPaintingNode(int32_t list_item_id) {}

void ListMediator::InsertListItemPaintingNode(int32_t list_item_id) {}

void ListMediator::FlushPatching(bool should_flush_finish_layout) {}

void ListMediator::FlushImmediately() {}

void ListMediator::UpdateContentOffsetAndSizeToPlatform(
    float content_size, float delta_x, float delta_y,
    bool is_init_scroll_offset, bool from_layout) {}

void ListMediator::OnListItemWillAppear(int32_t list_item_id,
                                        const std::string& item_key) {}

void ListMediator::OnListItemDisappear(int32_t list_item_id, bool is_exist,
                                       const std::string& item_key) {}

int ListMediator::GetThreadStrategy() const {
  return base::ThreadStrategyForRendering::ALL_ON_UI;
}

bool ListMediator::HasBoundEvent(const std::string& event_name) const {
  return false;
}

void ListMediator::SendCustomEvent(const std::string& event_name,
                                   const std::string& param_name,
                                   std::unique_ptr<pub::Value> param) {}

void ListMediator::UpdateScrollInfo(float estimated_offset, bool smooth,
                                    bool scrolling) {}

}  // namespace tasm
}  // namespace lynx
