// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_TESTING_MOCK_LIST_ELEMENT_H_
#define CORE_LIST_TESTING_MOCK_LIST_ELEMENT_H_

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "core/list/list_element_delegate.h"
#include "core/list/testing/mock_list_item_element.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"

namespace lynx {
namespace list {

static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 2.f;

class MockListElement : public ElementDelegate {
 public:
  MockListElement() = default;
  ~MockListElement() override = default;

  int32_t GetImplId() const override { return impl_id_; }
  float GetPhysicalPixelsPerLayoutUnit() const override {
    return kDefaultPhysicalPixelsPerLayoutUnit;
  }
  float GetLayoutsUnitPerPx() const override {
    return kDefaultLayoutsUnitPerPx;
  }
  MOCK_METHOD(void, MarkListElementLayoutDirty, (), (override));
  bool IsRTL() const override { return is_rtl_; }
  float GetWidth() const override { return width_; }
  float GetHeight() const override { return height_; }
  const std::array<float, 4>& GetPaddings() const override { return paddings_; }
  const std::array<float, 4>& GetMargins() const override { return margins_; }
  const std::array<float, 4>& GetBorders() const override { return borders_; }
  void FlushListContainerInfo(const std::string& list_container_info_str,
                              std::unique_ptr<pub::Value> list_container_info,
                              bool from_fiber_data_source) override {}
  MOCK_METHOD(void, UpdateListLayoutNodeAttribute, (), (override));
  MOCK_METHOD(bool, ComponentAtIndex,
              (uint32_t index, int64_t operationId,
               bool enable_reuse_notification),
              (override));
  MOCK_METHOD(void, EnqueueComponent, (int32_t list_item_id), (override));
  void RemoveListItemPaintingNode(int32_t list_item_id) override {}
  void InsertListItemPaintingNode(int32_t list_item_id) override {}
  void FlushPatching(bool should_flush_finish_layout) override {}
  void FlushImmediately() override {}
  void UpdateContentOffsetAndSizeToPlatform(float content_size, float delta_x,
                                            float delta_y,
                                            bool is_init_scroll_offset,
                                            bool from_layout) override {}
  bool HasBoundEvent(const std::string& event_name) const override {
    return events_.find(event_name) != events_.end();
  }
  MOCK_METHOD(void, SendCustomEvent,
              (const std::string& event_name, const std::string& param_name,
               std::unique_ptr<pub::Value> param),
              (override));
  void UpdateScrollInfo(float estimated_offset, bool smooth,
                        bool scrolling) override {}
  void ComponentAtIndexes(std::unique_ptr<pub::Value> index_array,
                          std::unique_ptr<pub::Value> operation_id_array,
                          bool enable_reuse_notification = false) {}
  int GetThreadStrategy() const {
    return base::ThreadStrategyForRendering::ALL_ON_UI;
  }
  bool IsFiberArch() const { return is_fiber_arch_; }
  void CheckZIndex(ItemElementDelegate* list_item_delegate) const {}
  void ReportListItemLifecycleStatistic(
      const std::shared_ptr<tasm::PipelineOptions>& option,
      const std::string& item_key) const {}
  MOCK_METHOD(void, OnErrorOccurred, (base::LynxError error),
              (const, override));
  bool IsAttachToElementManager() const { return is_attached_; }
  void MarkTiming(ListTiming flag) {}
  void RequestNextFrame() {}
  bool IsInDebugMode() const { return is_debug_mode_; }

  void ClearListItemElements() { list_item_elements_.clear(); }
  void AddListItemElement(
      const std::string& item_key,
      std::unique_ptr<MockListItemElement> list_item_element) {
    list_item_elements_[item_key] = std::move(list_item_element);
  }
  MockListItemElement* GetListItemElement(const std::string& item_key) {
    if (auto it = list_item_elements_.find(item_key);
        it != list_item_elements_.end()) {
      return it->second.get();
    }
    return nullptr;
  }

 private:
  int32_t impl_id_{1};
  bool is_fiber_arch_{false};
  bool is_rtl_{false};
  bool is_attached_{true};
  bool is_debug_mode_{false};
  float width_{500.f};
  float height_{500.f};
  std::array<float, 4> paddings_{0.f, 0.f, 0.f, 0.f};
  std::array<float, 4> margins_{0.f, 0.f, 0.f, 0.f};
  std::array<float, 4> borders_{0.f, 0.f, 0.f, 0.f};
  std::unordered_set<std::string> events_;
  std::unordered_map<std::string, std::unique_ptr<MockListItemElement>>
      list_item_elements_;
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_TESTING_MOCK_LIST_ELEMENT_H_
