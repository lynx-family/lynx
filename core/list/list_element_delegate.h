// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_LIST_ELEMENT_DELEGATE_H_
#define CORE_LIST_LIST_ELEMENT_DELEGATE_H_

#include <array>
#include <memory>
#include <string>

#include "base/include/debug/lynx_error.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/list/decoupled_list_types.h"
#include "core/list/list_item_element_delegate.h"
#include "core/public/pipeline_option.h"
#include "core/public/pub_value.h"

namespace lynx {
namespace list {

class ElementDelegate {
 public:
  virtual ~ElementDelegate() = default;

  virtual int32_t GetImplId() const = 0;
  virtual float GetPhysicalPixelsPerLayoutUnit() const = 0;
  virtual float GetLayoutsUnitPerPx() const = 0;
  virtual void MarkListElementLayoutDirty() = 0;
  virtual bool IsRTL() const = 0;
  virtual float GetWidth() const = 0;
  virtual float GetHeight() const = 0;
  virtual const std::array<float, 4>& GetPaddings() const = 0;
  virtual const std::array<float, 4>& GetMargins() const = 0;
  virtual const std::array<float, 4>& GetBorders() const = 0;
  virtual void FlushListContainerInfo(
      const std::string& list_container_info_str,
      std::unique_ptr<pub::Value> list_container_info,
      bool from_fiber_data_source) = 0;
  virtual void UpdateListLayoutNodeAttribute() = 0;
  virtual bool ComponentAtIndex(uint32_t index, int64_t operationId = 0,
                                bool enable_reuse_notification = false) = 0;
  virtual void EnqueueComponent(int32_t list_item_id) = 0;
  virtual void RemoveListItemPaintingNode(int32_t list_item_id) = 0;
  virtual void InsertListItemPaintingNode(int32_t list_item_id) = 0;
  virtual void FlushPatching(bool should_flush_finish_layout) = 0;
  virtual void FlushImmediately() = 0;
  virtual void UpdateContentOffsetAndSizeToPlatform(float content_size,
                                                    float delta_x,
                                                    float delta_y,
                                                    bool is_init_scroll_offset,
                                                    bool from_layout) = 0;
  virtual bool HasBoundEvent(const std::string& event_name) const = 0;
  virtual void SendCustomEvent(const std::string& event_name,
                               const std::string& param_name,
                               std::unique_ptr<pub::Value> param) = 0;
  virtual void UpdateScrollInfo(float estimated_offset, bool smooth,
                                bool scrolling) = 0;

  virtual void ComponentAtIndexes(
      std::unique_ptr<pub::Value> index_array,
      std::unique_ptr<pub::Value> operation_id_array,
      bool enable_reuse_notification = false) {}
  virtual int GetThreadStrategy() const {
    return base::ThreadStrategyForRendering::ALL_ON_UI;
  }
  virtual bool IsFiberArch() const { return false; }
  virtual void CheckZIndex(ItemElementDelegate* list_item_delegate) const {}
  virtual void ReportListItemLifecycleStatistic(
      const std::shared_ptr<tasm::PipelineOptions>& option,
      const std::string& item_key) const {}
  virtual void OnErrorOccurred(base::LynxError error) const {}
  virtual bool IsAttachToElementManager() const { return true; }
  virtual void MarkTiming(ListTiming flag) {}
  virtual void RequestNextFrame() {}
  virtual bool IsInDebugMode() const { return false; }
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_LIST_ELEMENT_DELEGATE_H_
