// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_ELEMENT_BUNDLE_H_
#define CORE_RENDERER_DOM_ELEMENT_BUNDLE_H_

#include <utility>

#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace tasm {

// Element bundle is used to manage the copied element tree.
class ElementBundle {
 public:
  ElementBundle() = default;
  ElementBundle(const ElementBundle&) = default;
  ElementBundle(ElementBundle&&) = default;
  ElementBundle& operator=(const ElementBundle&) = default;
  ElementBundle& operator=(ElementBundle&&) = default;

  explicit ElementBundle(lepus::Value&& page_node)
      : page_node_(std::move(page_node)) {}

  ElementBundle DeepClone() const;

  bool IsValid() const { return page_node_.IsRefCounted(); }

  const lepus::Value& GetPageNode() const { return page_node_; }

 private:
  lepus::Value page_node_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_ELEMENT_BUNDLE_H_
