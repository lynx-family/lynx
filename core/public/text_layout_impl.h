// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_TEXT_LAYOUT_IMPL_H_
#define CORE_PUBLIC_TEXT_LAYOUT_IMPL_H_

#include "core/public/layout_node_value.h"

namespace lynx {
namespace tasm {

class Element;

class TextLayoutImpl {
 public:
  virtual ~TextLayoutImpl() = default;

  virtual void DispatchLayoutBefore(Element* element) = 0;

  virtual void Destroy(Element* element) {}

  virtual LayoutResult Measure(Element* element, float width, int width_mode,
                               float height, int height_mode) = 0;

  virtual void Align(Element* element) = 0;
};

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_PUBLIC_TEXT_LAYOUT_IMPL_H_
