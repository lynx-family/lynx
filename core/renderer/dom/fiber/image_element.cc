// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/image_element.h"

#include <memory>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/text_props.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/dom/fragment/image_fragment_behavior.h"

namespace lynx {
namespace tasm {

ImageElement::ImageElement(ElementManager* manager, const base::String& tag)
    : FiberElement(manager, tag) {
  if (element_manager_ == nullptr) {
    return;
  }
  element_manager_->IncreaseImageElementCount();
}

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
  // TODO(songshourui.null): we can process image's attribute in C++ to optimize
  // the performance.
  if (EnableLayoutInElementMode()) {
    ProcessAttributeForLayoutInElement(key, value);
    attr_map_[key] = value;
  }
  FiberElement::SetAttributeInternal(key, value);
}

void ImageElement::ProcessAttributeForLayoutInElement(
    const base::String& key, const lepus::Value& value) {
  if (key.IsEqual(kImageAutoSize)) {
    has_auto_size_ = value.Bool();
  } else if (key.IsEqual(kSrc)) {
    url_ = value.String();
  }
}

void ImageElement::ResetAttribute(const base::String& key) {
  if (EnableLayoutInElementMode()) {
    attr_map_[key] = lepus::Value();
  }
  FiberElement::ResetAttribute(key);
}

int32_t ImageElement::GetBuiltInNodeInfo() const {
  return GetImageNodeInfo<GetOSType()>();
}

void ImageElement::SetupFragmentBehavior(Fragment* fragment) {
  fragment->SetBehavior(std::make_unique<ImageFragmentBehavior>(fragment));
}

}  // namespace tasm
}  // namespace lynx
