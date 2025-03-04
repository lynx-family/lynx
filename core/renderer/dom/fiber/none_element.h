// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_NONE_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_NONE_ELEMENT_H_

#include "core/renderer/dom/fiber/fiber_element.h"

namespace lynx {
namespace tasm {

class NoneElement : public FiberElement {
 public:
  NoneElement(ElementManager* manager);

  fml::RefPtr<FiberElement> CloneElement(
      bool clone_resolved_props) const override {
    return fml::AdoptRef<FiberElement>(
        new NoneElement(*this, clone_resolved_props));
  }

  bool is_none() const override { return true; }

 protected:
  NoneElement(const NoneElement& element, bool clone_resolved_props);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_NONE_ELEMENT_H_
