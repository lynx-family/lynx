// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_IMAGE_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_IMAGE_ELEMENT_H_

#include <cstdint>

#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/fiber/platform_types.h"

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

  const char* src() {
    auto it = attr_map_.find(BASE_STATIC_STRING(kSrc));
    return it == attr_map_.end() ? "" : it->second.CString();
  }

  void ResetAttribute(const base::String& key) override;

  int32_t GetBuiltInNodeInfo() const override;

 protected:
  ImageElement(const ImageElement& element, bool clone_resolved_props)
      : FiberElement(element, clone_resolved_props) {}

  void OnNodeAdded(FiberElement* child) override;

  void SetAttributeInternal(const base::String& key,
                            const lepus::Value& value) override;

  void ProcessAttributeForLayoutInElement(const base::String& key,
                                          const lepus::Value& value);

 public:
  void SetupFragmentBehavior(Fragment* fragment) override;

 protected:
  AttrUMap attr_map_;
  bool has_auto_size_{false};

 private:
  template <OSType type>
  int32_t GetImageNodeInfo() const {
    if (has_auto_size_) {
      return kCustomBuiltInNodeInfo;
    }
    return is_inline_element() ? kVirtualBuiltInNodeInfo
                               : kCommonBuiltInNodeInfo;
  }
};

template <>
inline int32_t ImageElement::GetImageNodeInfo<OSType::kIOS>() const {
  if (has_auto_size_) {
    return kCustomBuiltInNodeInfo;
  }
  return kCommonBuiltInNodeInfo;
}

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_IMAGE_ELEMENT_H_
