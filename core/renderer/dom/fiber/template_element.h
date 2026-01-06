// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_TEMPLATE_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_TEMPLATE_ELEMENT_H_

#include "core/renderer/dom/fiber/fiber_element.h"

namespace lynx {
namespace tasm {

class TemplateAssembler;
class TemplateEntry;

class SlotElement : public FiberElement {
 public:
  SlotElement(ElementManager* element_manager);
  ~SlotElement() override;

  bool is_wrapper() const override { return true; }

  bool is_slot() const override { return true; }

  void HandleInsert(const fml::RefPtr<FiberElement>& node,
                    const fml::RefPtr<FiberElement>& ref);

  void HandleRemove(const fml::RefPtr<FiberElement>& node);

  FiberElement* pre_{nullptr};
  FiberElement* after_{nullptr};

  FiberElement* current_first_{nullptr};
  FiberElement* current_last_{nullptr};

  FiberElement* dangling_parent_{nullptr};
};

class TemplateElement : public FiberElement {
 public:
  TemplateElement(ElementManager* element_manager);
  ~TemplateElement() override;

  bool is_template() const override { return true; }

  void SetTASM(TemplateAssembler* tasm) { tasm_ = tasm; }

  void SetOpCodes(const lepus::Value& op_codes) { op_codes_ = op_codes; }

  void SetTemplateKey(const base::String& template_key) {
    template_key_ = template_key;
  }

  void SetBundleUrl(const base::String& bundle_url) {
    bundle_url_ = bundle_url;
  }

  void AsyncCreateElementTree();

  fml::RefPtr<FiberElement> GetRoot();

 private:
  TemplateAssembler* tasm_;
  TemplateEntry* entry_;

  base::String template_key_;
  base::String bundle_url_{"__Card__"};

  fml::RefPtr<FiberElement> root_;
  base::Vector<fml::RefPtr<FiberElement>> result_;

  lepus::Value op_codes_;

  base::OnceTaskRefptr<base::Vector<fml::RefPtr<FiberElement>>> async_task_{
      nullptr};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_TEMPLATE_ELEMENT_H_
