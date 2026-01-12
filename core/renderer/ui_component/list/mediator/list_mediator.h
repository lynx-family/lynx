// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_MEDIATOR_LIST_MEDIATOR_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_MEDIATOR_LIST_MEDIATOR_H_

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/list/list_container_delegate.h"
#include "core/list/list_element_delegate.h"
#include "core/list/list_item_element_delegate.h"

namespace lynx {
namespace tasm {

class ListElement;
class ElementManager;

class ListMediator : public lynx::list::ElementDelegate {
 public:
  explicit ListMediator(ListElement* list_element);

  // All interfaces for list element.
  bool ResolveAttribute(const base::String& key, const lepus::Value& value);

  void ResolveListAxisGap(CSSPropertyID id, float gap);

  void PropsUpdateFinish();

  void OnLayoutChildren(const std::shared_ptr<PipelineOptions>& options);

  void FinishBindItemHolder(Element* list_item,
                            const std::shared_ptr<PipelineOptions>& option);

  void FinishBindItemHolders(const std::vector<Element*>& list_items,
                             const std::shared_ptr<PipelineOptions>& options);

  void ScrollByPlatformContainer(float content_offset_x, float content_offset_y,
                                 float original_x, float original_y);

  void ScrollToPosition(int index, float offset, int align, bool smooth);

  void ScrollStopped();

  void OnNextFrame();

  void OnAttachToElementManager();

  void OnListItemLayoutUpdated(Element* list_item);

  void SetEnableBatchRender(bool enable_batch_render);

  // Implement all interfaces for list::ElementDelegate
  int32_t GetImplId() const override;

  float GetPhysicalPixelsPerLayoutUnit() const override;

  float GetLayoutsUnitPerPx() const override;

  void MarkListElementLayoutDirty() override;

  bool IsRTL() const override;

  float GetWidth() const override;

  float GetHeight() const override;

  const std::array<float, 4>& GetPaddings() const override;

  const std::array<float, 4>& GetMargins() const override;

  const std::array<float, 4>& GetBorders() const override;

  void FlushListContainerInfo(const std::string& list_container_info_str,
                              std::unique_ptr<pub::Value> list_container_info,
                              bool from_fiber_data_source) override;

  void UpdateListLayoutNodeAttribute() override;

  bool ComponentAtIndex(uint32_t index, int64_t operationId = 0,
                        bool enable_reuse_notification = false) override;

  void ComponentAtIndexes(std::unique_ptr<pub::Value> index_array,
                          std::unique_ptr<pub::Value> operation_id_array,
                          bool enable_reuse_notification = false) override;

  void EnqueueComponent(int32_t list_item_id) override;

  void RemoveListItemPaintingNode(int32_t list_item_id) override;

  void InsertListItemPaintingNode(int32_t list_item_id) override;

  void FlushPatching(bool should_flush_finish_layout) override;

  void FlushImmediately() override;

  void UpdateContentOffsetAndSizeToPlatform(float content_size, float delta_x,
                                            float delta_y,
                                            bool is_init_scroll_offset,
                                            bool from_layout) override;

  bool HasBoundEvent(const std::string& event_name) const override;

  void SendCustomEvent(const std::string& event_name,
                       const std::string& param_name,
                       std::unique_ptr<pub::Value> param) override;

  void UpdateScrollInfo(float estimated_offset, bool smooth,
                        bool scrolling) override;

  int GetThreadStrategy() const override;

  bool IsFiberArch() const override;

  void CheckZIndex(
      lynx::list::ItemElementDelegate* list_item_delegate) const override;

  void ReportListItemLifecycleStatistic(
      const std::shared_ptr<tasm::PipelineOptions>& option,
      const std::string& item_key) const override;

  void OnErrorOccurred(base::LynxError error) const override;

  bool IsAttachToElementManager() const override;

  void MarkTiming(lynx::list::ListTiming flag) override;

  void RequestNextFrame() override;

  bool IsInDebugMode() const override;

 private:
  ListElement* list_element_{nullptr};
  std::unique_ptr<lynx::list::ContainerDelegate> list_container_delegate_{
      nullptr};
  std::unordered_map<Element*, std::unique_ptr<lynx::list::ItemElementDelegate>>
      list_item_delegate_map_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_MEDIATOR_LIST_MEDIATOR_H_
