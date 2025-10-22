// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_MEDIATOR_LIST_MEDIATOR_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_MEDIATOR_LIST_MEDIATOR_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "core/list/list_container_delegate.h"

namespace lynx {
namespace tasm {

class Element;

class ListMediator : public lynx::list::ElementDelegate {
 public:
  ListMediator(Element* list_element);

  int32_t GetImplId() const override;

  float GetPhysicalPixelsPerLayoutUnit() const override;

  float GetLayoutsUnitPerPx() const override;

  void MarkListElementLayoutDirty() override;

  void OnErrorOccurred(base::LynxError error) const override;

  bool IsRTL() const override;

  float GetWidth() const override;

  float GetHeight() const override;

  const std::array<float, 4>& GetPaddings() const override;

  const std::array<float, 4>& GetMargins() const override;

  const std::array<float, 4>& GetBorders() const override;

  void FlushListContainerInfo(
      const std::string& list_container_info_str,
      std::unique_ptr<pub::Value> list_container_info) override;

  void UpdateListLayoutNodeAttribute() override;

  bool ComponentAtIndex(uint32_t index, int64_t operationId = 0,
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

  void OnListItemWillAppear(int32_t list_item_id,
                            const std::string& item_key) override;

  void OnListItemDisappear(int32_t list_item_id, bool is_exist,
                           const std::string& item_key) override;

  int GetThreadStrategy() const override;

  bool HasBoundEvent(const std::string& event_name) const override;

  void SendCustomEvent(const std::string& event_name,
                       const std::string& param_name,
                       std::unique_ptr<pub::Value> param) override;

  void UpdateScrollInfo(float estimated_offset, bool smooth,
                        bool scrolling) override;

 private:
  Element* list_element_{nullptr};
  std::unique_ptr<lynx::list::ContainerDelegate> list_container_delegate_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_MEDIATOR_LIST_MEDIATOR_H_
