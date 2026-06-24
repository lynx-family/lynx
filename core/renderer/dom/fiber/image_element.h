// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_IMAGE_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_IMAGE_ELEMENT_H_

#include <cstdint>
#include <memory>

#include "core/renderer/dom/element.h"
#include "core/renderer/dom/fiber/platform_types.h"
#include "core/renderer/ui_wrapper/painting/paint_image.h"

namespace lynx {
namespace tasm {

class ImageElement : public Element {
 public:
  ImageElement(ElementManager* manager, const base::String& tag);

  fml::RefPtr<Element> CloneElement(bool clone_resolved_props) const override {
    return fml::AdoptRef<Element>(
        new ImageElement(*this, clone_resolved_props));
  }

  bool is_image() const override { return true; }

  bool DisableFlattenWithOpacity();

  void ConvertToInlineElement() override;

  void AttachToElementManager(
      ElementManager* manager,
      const std::shared_ptr<CSSStyleSheetManager>& style_manager,
      bool keep_element_id) override;

  const base::String& src() const { return url_; }
  const ImagePaintInfo& paint_info() const { return paint_info_; }
  const AttrUMap& attr_map() const { return attr_map_; }

  void ResetAttribute(const base::String& key) override;

  int32_t GetBuiltInNodeInfo() const override;

 protected:
  ImageElement(const ImageElement& element, bool clone_resolved_props)
      : Element(element, clone_resolved_props) {}

  void OnNodeAdded(Element* child) override;

  void SetAttributeInternal(const base::String& key,
                            const lepus::Value& value) override;

  void ProcessAttributeForLayoutInElement(const base::String& key,
                                          const lepus::Value& value);

 public:
  void SetupFragmentBehavior(Fragment* fragment) override;

 protected:
  AttrUMap attr_map_;
  bool has_auto_size_{false};
  base::String url_;
  ImagePaintInfo paint_info_;

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

template <>
inline int32_t ImageElement::GetImageNodeInfo<OSType::kHarmony>() const {
  if (has_auto_size_) {
    return kCustomBuiltInNodeInfo;
  }
  // Layout-in-Element still uses the regular UI tree, so inline images need a
  // platform node. Fragment Layer renders them as part of the display list.
  return is_inline_element() && EnableFragmentLayerRender()
             ? kVirtualBuiltInNodeInfo
             : kCommonBuiltInNodeInfo;
}

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_IMAGE_ELEMENT_H_
