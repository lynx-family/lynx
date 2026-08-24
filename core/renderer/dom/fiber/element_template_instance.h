// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_FIBER_ELEMENT_TEMPLATE_INSTANCE_H_
#define CORE_RENDERER_DOM_FIBER_ELEMENT_TEMPLATE_INSTANCE_H_

#include "base/include/value/ref_counted_class.h"
#include "core/renderer/dom/element.h"

namespace lynx {
namespace tasm {

class ElementTemplateInstanceSerializer;
class ElementTemplateInstance : public lepus::RefCounted {
 public:
  explicit ElementTemplateInstance(ElementManager* element_manager);
  ~ElementTemplateInstance() override;

  lepus::RefType GetRefType() const override {
    return lepus::RefType::kElementTemplate;
  }

  void SetTemplateKey(const base::String& template_key) {
    template_key_ = template_key;
  }
  void SetBundleUrl(const base::String& bundle_url) {
    bundle_url_ = bundle_url;
  }
  void SetTypedTag(const base::String& typed_tag);
  bool IsTypedTemplate() const { return !typed_tag_.empty(); }
  void SetRootAttributes(const lepus::Value& attributes);
  void SetAttributeSlots(const lepus::Value& attribute_slots);
  void SetOptions(const lepus::Value& options);
  void SetUid(const lepus::Value& uid);

  lepus::Value Serialize() const;

 private:
  friend class ElementTemplateInstanceSerializer;

  base::String template_key_;
  base::String bundle_url_;
  base::String typed_tag_;

  lepus::Value root_attributes_;
  uint32_t root_attributes_generation_{0};
  lepus::Value attribute_slots_;
  uint32_t attribute_slots_generation_{0};
  lepus::Value child_slots_;
  lepus::Value options_;
  lepus::Value uid_;
};

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_DOM_FIBER_ELEMENT_TEMPLATE_INSTANCE_H_
