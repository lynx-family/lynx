// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/image_element.h"

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/text_props.h"

namespace lynx {
namespace tasm {

ImageElement::ImageElement(ElementManager* manager, const base::String& tag)
    : FiberElement(manager, tag) {}

void ImageElement::OnNodeAdded(FiberElement* child) {
  LOGE("image element can not insert any child!!!");
}

bool ImageElement::DisableFlattenWithOpacity() { return false; }

void ImageElement::ConvertToInlineElement() {
  if (tag_.IsEqual(kElementXImageTag)) {
    tag_ = BASE_STATIC_STRING(kElementXInlineImageTag);
  } else {
    tag_ = BASE_STATIC_STRING(kElementInlineImageTag);
  }
  data_model()->set_tag(tag_);
  UpdateTagToLayoutBundle();
  FiberElement::ConvertToInlineElement();
}

void ImageElement::SetAttributeInternal(const base::String& key,
                                        const lepus::Value& value) {
  if (EnableLayoutInElementMode()) {
    if (key.IsEqual(kSrc)) {
      src_ = value.String();
    }
    // TODO(linxs): other attributes
  } else {
    FiberElement::SetAttributeInternal(key, value);
  }
}

void ImageElement::BuildAttributedStringProps(int start, int end,
                                              PropArray* props) {
  // inline range start
  props->AddProp(kPropInlineStart);
  props->AddProp(start);

  // src
  props->AddProp(kPropImageSrc);
  props->AddProp(src_.c_str());

  // mode
  // TBD

  // size
  props->AddProp(kPropRectSize);
  float width =
      starlight::NLengthToFakeLayoutUnit(slnode()->GetCSSStyle()->GetWidth())
          .ClampIndefiniteToZero()
          .ToFloat();
  float height =
      starlight::NLengthToFakeLayoutUnit(slnode()->GetCSSStyle()->GetHeight())
          .ClampIndefiniteToZero()
          .ToFloat();
  props->AddProp(static_cast<int>(width));
  props->AddProp(static_cast<int>(height));

  // margin
  int margin_left =
      static_cast<int>(starlight::NLengthToFakeLayoutUnit(
                           slnode()->GetCSSStyle()->GetMarginLeft())
                           .ClampIndefiniteToZero()
                           .ToFloat());
  int margin_top =
      static_cast<int>(starlight::NLengthToFakeLayoutUnit(
                           slnode()->GetCSSStyle()->GetMarginRight())
                           .ClampIndefiniteToZero()
                           .ToFloat());
  int margin_right =
      static_cast<int>(starlight::NLengthToFakeLayoutUnit(
                           slnode()->GetCSSStyle()->GetMarginTop())
                           .ClampIndefiniteToZero()
                           .ToFloat());
  int margin_bottom =
      static_cast<int>(starlight::NLengthToFakeLayoutUnit(
                           slnode()->GetCSSStyle()->GetMarginBottom())
                           .ClampIndefiniteToZero()
                           .ToFloat());
  if (margin_left | margin_top | margin_right | margin_bottom) {
    props->AddProp(kPropMargin);
    props->AddProp(margin_left);
    props->AddProp(margin_top);
    props->AddProp(margin_right);
    props->AddProp(margin_bottom);
  }

  // inline range end
  props->AddProp(kPropInlineEnd);
  props->AddProp(end);
}

}  // namespace tasm
}  // namespace lynx
