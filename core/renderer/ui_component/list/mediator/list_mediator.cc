// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/mediator/list_mediator.h"

#include <utility>

#include "base/include/value/base_string.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/ui_component/list/mediator/list_item_mediator.h"
#include "core/services/timing_handler/timing.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

ListMediator::ListMediator(ListElement* list_element)
    : list_element_(list_element),
      list_container_delegate_(lynx::list::CreateListContainerDelegate(
          this, std::make_shared<pub::PubValueFactoryDefault>())) {}

bool ListMediator::ResolveAttribute(const base::String& key,
                                    const lepus::Value& value) {
  if (list_container_delegate_) {
    return list_container_delegate_->ResolveAttribute(
        pub::ValueImplLepus(lepus::Value(key)), pub::ValueImplLepus(value));
  }
  return false;
}

void ListMediator::OnLayoutChildren(
    const std::shared_ptr<PipelineOptions>& options) {
  if (list_container_delegate_) {
    list_container_delegate_->OnLayoutChildren(options);
  }
}

void ListMediator::OnNextFrame() {
  if (list_container_delegate_) {
    list_container_delegate_->OnNextFrame();
  }
}

void ListMediator::FinishBindItemHolder(
    Element* list_item, const std::shared_ptr<PipelineOptions>& option) {
  if (list_container_delegate_) {
    if (list_item_delegate_map_.find(list_item) ==
        list_item_delegate_map_.end()) {
      list_item_delegate_map_[list_item] =
          std::make_unique<ListItemMediator>(list_item);
    }
    list_container_delegate_->FinishBindItemHolder(
        list_item_delegate_map_[list_item].get(), option);
  }
}

void ListMediator::FinishBindItemHolders(
    const std::vector<Element*>& list_items,
    const std::shared_ptr<PipelineOptions>& options) {
  if (list_container_delegate_) {
    std::vector<lynx::list::ItemElementDelegate*> list_item_delegate_array;
    for (Element* list_item : list_items) {
      if (list_item) {
        if (list_item_delegate_map_.find(list_item) ==
            list_item_delegate_map_.end()) {
          list_item_delegate_map_[list_item] =
              std::make_unique<ListItemMediator>(list_item);
        }
        list_item_delegate_array.emplace_back(
            list_item_delegate_map_[list_item].get());
      }
    }
    list_container_delegate_->FinishBindItemHolders(list_item_delegate_array,
                                                    options);
  }
}

void ListMediator::ScrollByPlatformContainer(float content_offset_x,
                                             float content_offset_y,
                                             float original_x,
                                             float original_y) {
  if (list_container_delegate_) {
    list_container_delegate_->ScrollByPlatformContainer(
        content_offset_x, content_offset_y, original_x, original_y);
  }
}

void ListMediator::ScrollToPosition(int index, float offset, int align,
                                    bool smooth) {
  if (list_container_delegate_) {
    list_container_delegate_->ScrollToPosition(index, offset, align, smooth);
  }
}

void ListMediator::ScrollStopped() {
  if (list_container_delegate_) {
    list_container_delegate_->ScrollStopped();
  }
}

void ListMediator::ResolveListAxisGap(CSSPropertyID id, float gap) {
  if (list_container_delegate_) {
    list_container_delegate_->ResolveListAxisGap(id, gap);
  }
}

void ListMediator::PropsUpdateFinish() {
  if (list_container_delegate_) {
    list_container_delegate_->PropsUpdateFinish();
  }
}

void ListMediator::OnAttachToElementManager() {
  if (list_container_delegate_) {
    list_container_delegate_->OnAttachToElementManager();
  }
}

void ListMediator::OnListItemLayoutUpdated(Element* list_item) {
  if (list_container_delegate_) {
    if (list_item_delegate_map_.find(list_item) ==
        list_item_delegate_map_.end()) {
      list_item_delegate_map_[list_item] =
          std::make_unique<ListItemMediator>(list_item);
    }
    list_container_delegate_->OnListItemLayoutUpdated(
        list_item_delegate_map_[list_item].get());
  }
}

void ListMediator::SetEnableBatchRender(bool enable_batch_render) {
  if (list_container_delegate_) {
    list_container_delegate_->SetEnableBatchRender(enable_batch_render);
  }
}

int32_t ListMediator::GetImplId() const { return list_element_->impl_id(); }

float ListMediator::GetPhysicalPixelsPerLayoutUnit() const {
  ElementManager* element_manager = list_element_->element_manager();
  return element_manager
             ? element_manager->GetLynxEnvConfig().PhysicalPixelsPerLayoutUnit()
             : 1.f;
}

float ListMediator::GetLayoutsUnitPerPx() const {
  ElementManager* element_manager = list_element_->element_manager();
  return element_manager
             ? element_manager->GetLynxEnvConfig().LayoutsUnitPerPx()
             : 1.f;
}

void ListMediator::MarkListElementLayoutDirty() {
  list_element_->MarkLayoutDirty();
}

bool ListMediator::IsRTL() const {
  auto* computed_css_style = list_element_->computed_css_style();
  return computed_css_style && (computed_css_style->GetDirection() ==
                                    starlight::DirectionType::kRtl ||
                                computed_css_style->GetDirection() ==
                                    starlight::DirectionType::kLynxRtl);
}

float ListMediator::GetWidth() const { return list_element_->width(); }

float ListMediator::GetHeight() const { return list_element_->height(); }

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
    std::unique_ptr<pub::Value> list_container_info,
    bool from_fiber_data_source) {
  if (list_container_info && list_container_info->backend_type() ==
                                 pub::ValueBackendType::ValueBackendTypeLepus) {
    list_element_->FlushListContainerInfo(
        base::String(list_container_info_str),
        pub::ValueUtils::ConvertValueToLepusValue(*list_container_info));
  }
}

void ListMediator::UpdateListLayoutNodeAttribute() {
  list_element_->UpdateLayoutNodeAttribute(
      starlight::LayoutAttribute::kListContainer, lepus::Value(true));
}

bool ListMediator::ComponentAtIndex(
    uint32_t index, int64_t operationId /* = 0 */,
    bool enable_reuse_notification /* = false */) {
  ListNode* list_node = list_element_->GetListNode();
  if (list_node) {
    list_node->ComponentAtIndex(index, operationId, enable_reuse_notification);
    return true;
  }
  return false;
}

void ListMediator::ComponentAtIndexes(
    std::unique_ptr<pub::Value> index_array,
    std::unique_ptr<pub::Value> operation_id_array,
    bool enable_reuse_notification /* = false */) {
  ListNode* list_node = list_element_->GetListNode();
  if (list_node && index_array && operation_id_array &&
      index_array->backend_type() ==
          pub::ValueBackendType::ValueBackendTypeLepus &&
      operation_id_array->backend_type() ==
          pub::ValueBackendType::ValueBackendTypeLepus &&
      index_array->IsArray() && operation_id_array->IsArray()) {
    // Only handle value with back end type lepus.
    const lepus::Value& indexes_lepus =
        pub::ValueUtils::ConvertValueToLepusValue(*index_array);
    const lepus::Value& operations_lepus =
        pub::ValueUtils::ConvertValueToLepusValue(*operation_id_array);
    list_node->ComponentAtIndexes(std::move(indexes_lepus).Array(),
                                  std::move(operations_lepus).Array());
  }
}

void ListMediator::EnqueueComponent(int32_t list_item_id) {
  ListNode* list_node = list_element_->GetListNode();
  if (list_node) {
    list_node->EnqueueComponent(list_item_id);
  }
}

void ListMediator::RemoveListItemPaintingNode(int32_t list_item_id) {
  list_element_->element_container()->RemoveListItemPaintingNode(list_item_id);
}

void ListMediator::InsertListItemPaintingNode(int32_t list_item_id) {
  list_element_->element_container()->InsertListItemPaintingNode(list_item_id);
}

void ListMediator::FlushPatching(bool should_flush_finish_layout) {
  list_element_->element_container()->UpdateLayoutPatching();
  // Note: Add list's id to patching_node_ready_ids_ before invoking
  // UpdateNodeReadyPatching(), and in list's OnNodeReady to handle sticky
  // list items.
  list_element_->OnNodeReady();
  list_element_->element_container()->UpdateNodeReadyPatching();
  if (should_flush_finish_layout) {
    auto options = std::make_shared<PipelineOptions>();
    options->has_layout = true;
    list_element_->element_container()->FinishLayoutOperation(options);
  }
  list_element_->element_container()->FlushImmediately();
}

void ListMediator::FlushImmediately() {
  list_element_->element_container()->FlushImmediately();
}

void ListMediator::UpdateContentOffsetAndSizeToPlatform(
    float content_size, float delta_x, float delta_y,
    bool is_init_scroll_offset, bool from_layout) {
  list_element_->element_container()->UpdateContentOffsetForListContainer(
      content_size, delta_x, delta_y, is_init_scroll_offset, from_layout);
}

bool ListMediator::HasBoundEvent(const std::string& event_name) const {
  return list_element_->event_map().contains(event_name) ||
         list_element_->lepus_event_map().contains(event_name);
}

void ListMediator::SendCustomEvent(const std::string& event_name,
                                   const std::string& param_name,
                                   std::unique_ptr<pub::Value> param) {
  ElementManager* element_manager = list_element_->element_manager();
  if (element_manager && param &&
      param->backend_type() == pub::ValueBackendType::ValueBackendTypeLepus) {
    element_manager->SendNativeCustomEvent(
        event_name, list_element_->impl_id(),
        pub::ValueUtils::ConvertValueToLepusValue(*param), param_name);
  }
}

void ListMediator::UpdateScrollInfo(float estimated_offset, bool smooth,
                                    bool scrolling) {
  list_element_->element_container()->UpdateScrollInfo(estimated_offset, smooth,
                                                       scrolling);
}

int ListMediator::GetThreadStrategy() const {
  ElementManager* element_manager = list_element_->element_manager();
  return element_manager ? element_manager->GetThreadStrategy()
                         : base::ThreadStrategyForRendering::ALL_ON_UI;
}

bool ListMediator::IsFiberArch() const { return list_element_->IsFiberArch(); }

void ListMediator::CheckZIndex(
    lynx::list::ItemElementDelegate* list_item_delegate) const {
  if (list_item_delegate) {
    Element* list_item_element =
        static_cast<ListItemMediator*>(list_item_delegate)->list_item_element();
    if (list_item_element->has_z_props() &&
        !list_element_->IsStackingContextNode()) {
      DLIST_LOGE(
          "[List] ListMediator::CheckZIndex: list is not stacking context node "
          "when child has z-index.");
    }
  }
}

void ListMediator::ReportListItemLifecycleStatistic(
    const std::shared_ptr<tasm::PipelineOptions>& option,
    const std::string& item_key) const {
  if (option->enable_report_list_item_life_statistic_) {
    std::string id_selector;
    auto* data_model = list_element_->data_model();
    if (data_model) {
      id_selector = data_model->idSelector().str();
    }
    report::EventTracker::OnEvent(
        [option = option, item_key, id_selector](report::MoveOnlyEvent& event) {
          event.SetName(list::kListItemLifecycleStatistic);
          event.SetProps(list::kListIdSelector, id_selector);
          event.SetProps(list::kItemKey, item_key);
          if (option->list_item_life_option_.update_duration() > 0.) {
            event.SetProps(list::kListItemUpdateDuration,
                           option->list_item_life_option_.update_duration());
          } else {
            event.SetProps(list::kListItemRenderDuration,
                           option->list_item_life_option_.render_duration());
            event.SetProps(list::kListItemDispatchDuration,
                           option->list_item_life_option_.dispatch_duration());
          }
          event.SetProps(list::kListItemLayoutDuration,
                         option->list_item_life_option_.layout_duration());
        });
  }
}

void ListMediator::OnErrorOccurred(base::LynxError error) const {
  ElementManager* element_manager = list_element_->element_manager();
  if (element_manager) {
    element_manager->OnErrorOccurred(std::move(error));
  }
}

bool ListMediator::IsAttachToElementManager() const {
  return list_element_->element_manager();
}

void ListMediator::MarkTiming(lynx::list::ListTiming flag) {
  if (flag == lynx::list::ListTiming::kRenderChildrenStart) {
    TimingCollector::Instance()->Mark(timing::kListRenderChildrenStart);
  } else if (flag == lynx::list::ListTiming::kRenderChildrenEnd) {
    TimingCollector::Instance()->Mark(timing::kListRenderChildrenEnd);
  } else if (flag == lynx::list::ListTiming::kFullFillRenderChildrenEnd) {
    TimingCollector::Instance()->Mark(timing::kListFullFillRenderChildrenEnd);
  }
}

void ListMediator::RequestNextFrame() { list_element_->RequestNextFrame(); }

bool ListMediator::IsInDebugMode() const {
  return LynxEnv::GetInstance().IsLynxDebugEnabled();
}

}  // namespace tasm
}  // namespace lynx
