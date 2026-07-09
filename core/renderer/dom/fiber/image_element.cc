// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/image_element.h"

#include <memory>

#include "base/include/string/string_number_convert.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/text_props.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/dom/fragment/image_fragment_behavior.h"

namespace lynx {
namespace tasm {

namespace {
constexpr char kImageMode[] = "mode";
constexpr char kImageBlurRadius[] = "blur-radius";
constexpr char kImagePlaceholder[] = "placeholder";
constexpr char kImageTintColor[] = "tint-color";
constexpr char kImageCapInsets[] = "cap-insets";
constexpr char kImageCapInsetsScale[] = "cap-insets-scale";
constexpr char kImageSkipRedirection[] = "skip-redirection";
constexpr char kImageAutoplay[] = "autoplay";
constexpr char kImageLoopCount[] = "loop-count";

BASE_STATIC_STRING_DECL(kModeAspectFit, "aspectFit");
BASE_STATIC_STRING_DECL(kModeAspectFill, "aspectFill");
BASE_STATIC_STRING_DECL(kModeCenter, "center");

ImageFitMode ResolveImageFitMode(const base::String& mode) {
  if (mode.IsEqual(kModeAspectFit)) {
    return ImageFitMode::kAspectFit;
  }
  if (mode.IsEqual(kModeAspectFill)) {
    return ImageFitMode::kAspectFill;
  }
  if (mode.IsEqual(kModeCenter)) {
    return ImageFitMode::kCenter;
  }
  return ImageFitMode::kScaleToFill;
}

float ResolveCapInsetsScale(const lepus::Value& value) {
  if (value.IsNumber()) {
    return static_cast<float>(value.Number());
  }
  float scale = 1.f;
  if (value.IsString() && base::StringToFloat(value.StdString(), scale)) {
    return scale;
  }
  return 1.f;
}
}  // namespace

ImageElement::ImageElement(ElementManager* manager, const base::String& tag)
    : Element(manager, tag) {
  if (element_manager_ == nullptr) {
    return;
  }

  if (element_manager_->IsFragmentLayerRenderModeOn()) {
    SetDefaultOverflow(element_manager_->GetDefaultTextOverflow());
  }

  element_manager_->IncreaseImageElementCount();
}

void ImageElement::AttachToElementManager(
    ElementManager* manager,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager,
    bool keep_element_id) {
  Element::AttachToElementManager(manager, style_manager, keep_element_id);

  if (element_manager_->IsFragmentLayerRenderModeOn()) {
    SetDefaultOverflow(manager->GetDefaultTextOverflow());
  }
}

void ImageElement::OnNodeAdded(Element* child) {
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
  Element::ConvertToInlineElement();
}

void ImageElement::SetAttributeInternal(const base::String& key,
                                        const lepus::Value& value) {
  // TODO(songshourui.null): we can process image's attribute in C++ to optimize
  // the performance.
  if (EnableLayoutInElementMode()) {
    ProcessAttributeForLayoutInElement(key, value);
    attr_map_[key] = value;
  }
  Element::SetAttributeInternal(key, value);
}

void ImageElement::ProcessAttributeForLayoutInElement(
    const base::String& key, const lepus::Value& value) {
  if (key.IsEqual(kImageAutoSize)) {
    has_auto_size_ = value.IsBool() && value.Bool();
    paint_info_.auto_size = has_auto_size_;
  } else if (key.IsEqual(kSrc)) {
    url_ = value.String();
  } else if (key.IsEqual(kImageMode)) {
    paint_info_.mode = value.IsString() ? ResolveImageFitMode(value.String())
                                        : ImageFitMode::kScaleToFill;
  } else if (key.IsEqual(kImageBlurRadius)) {
    paint_info_.blur_radius =
        value.IsString() ? value.String() : base::String();
  } else if (key.IsEqual(kImagePlaceholder)) {
    paint_info_.placeholder =
        value.IsString() ? value.String() : base::String();
  } else if (key.IsEqual(kImageTintColor)) {
    paint_info_.tint_color = value.IsString() ? value.String() : base::String();
  } else if (key.IsEqual(kImageCapInsets)) {
    paint_info_.cap_insets = value.IsString() ? value.String() : base::String();
  } else if (key.IsEqual(kImageCapInsetsScale)) {
    paint_info_.cap_insets_scale = ResolveCapInsetsScale(value);
  } else if (key.IsEqual(kImageSkipRedirection)) {
    paint_info_.skip_redirection = value.IsBool() && value.Bool();
  } else if (key.IsEqual(kImageAutoplay)) {
    paint_info_.autoplay = !value.IsBool() || value.Bool();
  } else if (key.IsEqual(kImageLoopCount)) {
    paint_info_.loop_count =
        value.IsNumber() ? static_cast<int32_t>(value.Number()) : 0;
  }
}

void ImageElement::ResetAttribute(const base::String& key) {
  if (EnableLayoutInElementMode()) {
    attr_map_[key] = lepus::Value();
    if (key.IsEqual(kSrc)) {
      url_ = base::String();
    } else if (key.IsEqual(kImageAutoSize)) {
      has_auto_size_ = false;
      paint_info_.auto_size = false;
    } else if (key.IsEqual(kImageMode)) {
      paint_info_.mode = ImageFitMode::kScaleToFill;
    } else if (key.IsEqual(kImageBlurRadius)) {
      paint_info_.blur_radius = base::String();
    } else if (key.IsEqual(kImagePlaceholder)) {
      paint_info_.placeholder = base::String();
    } else if (key.IsEqual(kImageTintColor)) {
      paint_info_.tint_color = base::String();
    } else if (key.IsEqual(kImageCapInsets)) {
      paint_info_.cap_insets = base::String();
    } else if (key.IsEqual(kImageCapInsetsScale)) {
      paint_info_.cap_insets_scale = 1.f;
    } else if (key.IsEqual(kImageSkipRedirection)) {
      paint_info_.skip_redirection = false;
    } else if (key.IsEqual(kImageAutoplay)) {
      paint_info_.autoplay = true;
    } else if (key.IsEqual(kImageLoopCount)) {
      paint_info_.loop_count = 0;
    }
  }
  Element::ResetAttribute(key);
}

int32_t ImageElement::GetBuiltInNodeInfo() const {
  return GetImageNodeInfo<GetOSType()>();
}

void ImageElement::SetupFragmentBehavior(Fragment* fragment) {
  fragment->SetBehavior(std::make_unique<ImageFragmentBehavior>(fragment));
}

}  // namespace tasm
}  // namespace lynx
