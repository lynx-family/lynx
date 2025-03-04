// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_TEXT_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_TEXT_ELEMENT_H_

#include <memory>

#include "core/renderer/dom/fiber/fiber_element.h"

namespace lynx {
namespace tasm {

class TextElement : public FiberElement {
 public:
  TextElement(ElementManager* manager, const base::String& tag);

  fml::RefPtr<FiberElement> CloneElement(
      bool clone_resolved_props) const override {
    return fml::AdoptRef<FiberElement>(
        new TextElement(*this, clone_resolved_props));
  }

  bool is_text() const override { return true; }
  void SetStyleInternal(CSSPropertyID id, const tasm::CSSValue& value,
                        bool force_update = false) override;
  void ConvertToInlineElement() override;

  void AttachToElementManager(
      ElementManager* manager,
      const std::shared_ptr<CSSStyleSheetManager>& style_manager,
      bool keep_element_id) override;

 protected:
  void OnNodeAdded(FiberElement* child) override;
  void SetAttributeInternal(const base::String& key,
                            const lepus::Value& value) override;

  TextElement(const TextElement& element, bool clone_resolved_props)
      : FiberElement(element, clone_resolved_props) {}

 private:
  void ResolveAndFlushFontFaces(const base::String& font_family);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_TEXT_ELEMENT_H_
