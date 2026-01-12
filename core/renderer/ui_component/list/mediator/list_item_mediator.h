// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_MEDIATOR_LIST_ITEM_MEDIATOR_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_MEDIATOR_LIST_ITEM_MEDIATOR_H_

#include <array>
#include <memory>
#include <string>

#include "core/list/list_item_element_delegate.h"

namespace lynx {
namespace tasm {

class Element;

class ListItemMediator : public lynx::list::ItemElementDelegate {
 public:
  ListItemMediator(Element* list_item_element);
  ~ListItemMediator() override = default;

  int32_t GetImplId() const override;

  std::string GetIdSelector() const override;

  float GetWidth() const override;

  float GetHeight() const override;

  float GetLeft() const override;

  float GetTop() const override;

  const std::array<float, 4>& GetPaddings() const override;

  const std::array<float, 4>& GetMargins() const override;

  const std::array<float, 4>& GetBorders() const override;

  void UpdateLayoutToPlatform(float left, float top) override;

  bool HasBoundEvent(const std::string& event_name) const override;

  void SendCustomEvent(const std::string& event_name,
                       const std::string& param_name,
                       std::unique_ptr<pub::Value> param) override;

  void OnListItemWillAppear(const std::string& item_key) override;

  void OnListItemDisappear(bool is_exist, const std::string& item_key) override;

  void FlushPatching() override;

  void FlushAnimatedStyle(CSSPropertyID id, CSSValue value) override;

  Element* list_item_element() { return list_item_element_; }

 private:
  Element* list_item_element_{nullptr};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_MEDIATOR_LIST_ITEM_MEDIATOR_H_
