// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_LIST_ITEM_ELEMENT_DELEGATE_H_
#define CORE_LIST_LIST_ITEM_ELEMENT_DELEGATE_H_

#include <array>
#include <cstdint>
#include <memory>
#include <string>

#include "core/public/pub_value.h"

namespace lynx {
namespace list {

class ItemElementDelegate {
 public:
  virtual ~ItemElementDelegate() = default;
  virtual int32_t GetImplId() const = 0;
  virtual std::string GetIdSelector() const = 0;
  virtual float GetWidth() const = 0;
  virtual float GetHeight() const = 0;
  virtual const std::array<float, 4>& GetPaddings() const = 0;
  virtual const std::array<float, 4>& GetMargins() const = 0;
  virtual const std::array<float, 4>& GetBorders() const = 0;
  virtual void UpdateLayoutToPlatform(float left, float top) = 0;
  virtual bool HasBoundEvent(const std::string& event_name) const = 0;
  virtual void SendCustomEvent(const std::string& event_name,
                               const std::string& param_name,
                               std::unique_ptr<pub::Value> param) = 0;
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_LIST_ITEM_ELEMENT_DELEGATE_H_
