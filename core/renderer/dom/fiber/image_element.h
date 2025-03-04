// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_IMAGE_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_IMAGE_ELEMENT_H_

#include "core/renderer/dom/fiber/fiber_element.h"

namespace lynx {
namespace tasm {

class ImageElement : public FiberElement {
 public:
  ImageElement(ElementManager* manager, const base::String& tag);

  fml::RefPtr<FiberElement> CloneElement(
      bool clone_resolved_props) const override {
    return fml::AdoptRef<FiberElement>(
        new ImageElement(*this, clone_resolved_props));
  }

  bool is_image() const override { return true; }

  bool DisableFlattenWithOpacity();

  void ConvertToInlineElement() override;

 protected:
  ImageElement(const ImageElement& element, bool clone_resolved_props)
      : FiberElement(element, clone_resolved_props) {}

  void OnNodeAdded(FiberElement* child) override;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_IMAGE_ELEMENT_H_
