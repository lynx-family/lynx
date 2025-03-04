// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_WRAPPER_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_WRAPPER_ELEMENT_H_

#include <utility>

#include "core/renderer/dom/fiber/fiber_element.h"

namespace lynx {
namespace tasm {

class WrapperElement : public FiberElement {
 public:
  enum Type {
    kCommon = 1 << 0,
    kTouchable = 1 << 1,
  };

  WrapperElement(ElementManager* manager, const base::String& tag);
  WrapperElement(ElementManager* manager);

  fml::RefPtr<FiberElement> CloneElement(
      bool clone_resolved_props) const override {
    return fml::AdoptRef<FiberElement>(
        new WrapperElement(*this, clone_resolved_props));
  }

  bool is_wrapper() const override { return true; }

  double GetFontSize() override;

  const InheritedProperty& GetInheritedProperty() override;

  ParallelFlushReturn PrepareForCreateOrUpdate() override;

  // When exec UpdateInheritedProperty, do nothing for Wrapper Element.
  void UpdateInheritedProperty() override {}

  virtual void MarkDirtyLite(const uint32_t flag) override;

  void MarkAsListItem() override;

  void SetWrapperType(Type type);

  void SetAttribute(const base::String& key, const lepus::Value& value,
                    bool need_update_data_model = true) override;

 protected:
  WrapperElement(const WrapperElement& element, bool clone_resolved_props)
      : FiberElement(element, clone_resolved_props) {
    is_layout_only_ = true;
  }

  void OnNodeAdded(FiberElement* child) override;
  void OnNodeRemoved(FiberElement* child) override;

  // type_ is a collection, that is, it may be multiple types of Wrapper at the
  // same time. For example, multiple Wrapper are nested, and the front-end
  // framework may compress multiple Wrapper into one.
  int32_t type_ = kCommon;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_WRAPPER_ELEMENT_H_
