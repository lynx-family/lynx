// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_TESTING_MOCK_LIST_ITEM_ELEMENT_H_
#define CORE_LIST_TESTING_MOCK_LIST_ITEM_ELEMENT_H_

#include <array>
#include <memory>
#include <string>
#include <unordered_set>

#include "core/list/list_item_element_delegate.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"

namespace lynx {
namespace list {

class MockListItemElement : public ItemElementDelegate {
 public:
  MockListItemElement(int32_t impl_id) : impl_id_(impl_id) {}
  ~MockListItemElement() override = default;

  int32_t GetImplId() const override { return impl_id_; }
  std::string GetIdSelector() const {
    return std::string("mock_list_item_element_") + std::to_string(impl_id_);
  }
  float GetWidth() const override { return width_; }
  float GetHeight() const override { return height_; }
  float GetLeft() const override { return left_; }
  float GetTop() const override { return top_; }
  const std::array<float, 4>& GetPaddings() const override { return paddings_; }
  const std::array<float, 4>& GetMargins() const override { return margins_; }
  const std::array<float, 4>& GetBorders() const override { return borders_; }
  void UpdateLayoutToPlatform(float left, float top) override {
    left_ = left;
    top_ = top;
  }
  bool HasBoundEvent(const std::string& event_name) const override {
    return events_.find(event_name) != events_.end();
  }
  MOCK_METHOD(void, SendCustomEvent,
              (const std::string& event_name, const std::string& param_name,
               std::unique_ptr<pub::Value> param),
              (override));
  void OnListItemWillAppear(const std::string& item_key) override {}
  void OnListItemDisappear(bool is_exist,
                           const std::string& item_key) override {}

 private:
  int32_t impl_id_{-1};
  float left_{0.f};
  float top_{0.f};
  float width_{500.f};
  float height_{100.f};
  std::array<float, 4> paddings_{0.f, 0.f, 0.f, 0.f};
  std::array<float, 4> margins_{0.f, 0.f, 0.f, 0.f};
  std::array<float, 4> borders_{0.f, 0.f, 0.f, 0.f};
  std::unordered_set<std::string> events_;
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_TESTING_MOCK_LIST_ITEM_ELEMENT_H_
