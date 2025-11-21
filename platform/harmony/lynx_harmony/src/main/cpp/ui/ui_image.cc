// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_image.h"

#include <filemanagement/file_uri/oh_file_uri.h>
#include <native_drawing/drawing_bitmap.h>
#include <native_drawing/drawing_brush.h>
#include <native_drawing/drawing_color.h>
#include <native_drawing/drawing_color_filter.h>
#include <native_drawing/drawing_filter.h>
#include <native_drawing/drawing_image_filter.h>
#include <native_drawing/drawing_pixel_map.h>
#include <native_drawing/drawing_rect.h>
#include <native_drawing/drawing_sampling_options.h>
#include <native_drawing/drawing_shader_effect.h>

#include <cstdlib>
#include <utility>

#include "base/include/float_comparison.h"
#include "base/include/string/string_number_convert.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/css/css_color.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/image_shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_constants.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_helper.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_unit_utils.h"

namespace lynx {
namespace tasm {
namespace harmony {

using ImagePropSetter = void (UIImage::*)(const lepus::Value& value);
std::unordered_map<std::string, ImagePropSetter> UIImage::prop_setters_ = {
    {"src", &UIImage::UpdateImageSource},
    {"mode", &UIImage::UpdateImageMode},
    {"auto-size", &UIImage::UpdateAutoSize},
    {"blur-radius", &UIImage::UpdateBlurRadius},
    {"defer-src-invalidation", &UIImage::UpdateDeferSrcInvalidation},
    {"placeholder", &UIImage::UpdatePlaceholder},
    {"tint-color", &UIImage::UpdateTintColor},
    {"drop-shadow", &UIImage::UpdateDropShadow},
    {"cap-insets", &UIImage::UpdateCapInsets},
    {"cap-insets-scale", &UIImage::UpdateCapInsetScale},
    {"skip-redirection", &UIImage::UpdateSkipRedirection}};

void UIImage::OnPropUpdate(const std::string& name, const lepus::Value& value) {
  UIBase::OnPropUpdate(name, value);
  if (auto it = prop_setters_.find(name); it != prop_setters_.end()) {
    ImagePropSetter setter = it->second;
    (this->*setter)(value);
  }
}

void UIImage::OnDrawBehind(OH_Drawing_Canvas* canvas, ArkUI_NodeHandle node) {
  if (DrawNode() != Node()) {
    // draw background in draw-node
    return;
  }
  auto padding = NodeManager::Instance().GetAttribute(Node(), NODE_PADDING);
  float left = padding->value[3].f32;
  float top = padding->value[0].f32;
  if (base::IsZero(left) && base::IsZero(top)) {
    return UIBase::OnDrawBehind(canvas, node);
  }
  OH_Drawing_CanvasSave(canvas);
  OH_Drawing_CanvasTranslate(canvas, -left * context_->ScaledDensity(),
                             -top * context_->ScaledDensity());
  UIBase::OnDrawBehind(canvas, node);
  OH_Drawing_CanvasRestore(canvas);
}

UIImage::UIImage(LynxContext* context, int sign, const std::string& tag)
    : UIBase(context, ARKUI_NODE_IMAGE, sign, tag),
      mode_(image::kModeScaleToFill) {
  NodeManager::Instance().RegisterNodeEvent(Node(), NODE_IMAGE_ON_COMPLETE,
                                            NODE_IMAGE_ON_COMPLETE, this);
  NodeManager::Instance().RegisterNodeEvent(Node(), NODE_IMAGE_ON_ERROR,
                                            NODE_IMAGE_ON_ERROR, this);
  NodeManager::Instance().SetAttributeWithNumberValue(
      Node(), NODE_IMAGE_INTERPOLATION,
      static_cast<int32_t>(ARKUI_IMAGE_INTERPOLATION_LOW));
  NodeManager::Instance().SetAttributeWithNumberValue(Node(),
                                                      NODE_IMAGE_DRAGGABLE, 0);
  NodeManager::Instance().SetAttributeWithNumberValue(
      Node(), NODE_IMAGE_OBJECT_FIT,
      static_cast<int32_t>(ARKUI_OBJECT_FIT_FILL));
  InitAccessibilityAttrs(LynxAccessibilityMode::kEnable, "image");
}

ArkUI_ObjectFit UIImage::ConvertMode(const std::string& mode) {
  if (mode == image::kModeAspectFit) {
    return ARKUI_OBJECT_FIT_CONTAIN;
  }
  if (mode == image::kModeAspectFill) {
    return ARKUI_OBJECT_FIT_COVER;
  }
  return ARKUI_OBJECT_FIT_FILL;
}

void UIImage::UpdateImageSource(const lepus::Value& value) {
  const auto& value_str = value.StdString();
  if (src_ != value_str) {
    src_ = value_str;
    dirty_flags_ |= image::kFlagSrcChanged;
  }
}

void UIImage::UpdateImageMode(const lepus::Value& value) {
  const auto& value_str = value.StdString();
  if (mode_ != value_str) {
    mode_ = value_str;
    SetImageModeAttribute(mode_);
  }
}

void UIImage::UpdateLayout(float left, float top, float width, float height,
                           const float* paddings, const float* margins,
                           const float* sticky, float max_height,
                           uint32_t node_index) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_IMAGE_UPDATE_LAYOUT);
  UIBase::UpdateLayout(left, top, width, height, paddings, margins, sticky,
                       max_height, node_index);
  if (background_drawable_) {
    padding_top_ = background_drawable_->GetBorderTopWidth() + padding_top_;
    padding_left_ = background_drawable_->GetBorderLeftWidth() + padding_left_;
    padding_bottom_ =
        background_drawable_->GetBorderBottomWidth() + padding_bottom_;
    padding_right_ =
        background_drawable_->GetBorderRightWidth() + padding_right_;
  }
  if (image_padding_top_ != padding_top_) {
    image_padding_top_ = padding_top_;
    dirty_flags_ |= image::kFlagPaddingChanged;
  }
  if (image_padding_left_ != padding_left_) {
    image_padding_left_ = padding_left_;
    dirty_flags_ |= image::kFlagPaddingChanged;
  }
  if (image_padding_right_ != padding_right_) {
    image_padding_right_ = padding_right_;
    dirty_flags_ |= image::kFlagPaddingChanged;
  }
  if (image_padding_bottom_ != padding_bottom_) {
    image_padding_bottom_ = padding_bottom_;
    dirty_flags_ |= image::kFlagPaddingChanged;
  }
}

void UIImage::SetImageModeAttribute(const std::string& value) {
  NodeManager::Instance().SetAttributeWithNumberValue(
      Node(), NODE_IMAGE_OBJECT_FIT, static_cast<int32_t>(ConvertMode(value)));
}

UIImage::~UIImage() {
  NodeManager::Instance().UnregisterNodeEvent(Node(), NODE_IMAGE_ON_ERROR);
  NodeManager::Instance().UnregisterNodeEvent(Node(), NODE_IMAGE_ON_COMPLETE);
  if (color_filter_) {
    OH_Drawing_ColorFilterDestroy(color_filter_);
  }
  if (drawable_descriptor_) {
    OH_ArkUI_DrawableDescriptor_Dispose(drawable_descriptor_);
  }
}

void UIImage::OnNodeEvent(ArkUI_NodeEvent* event) {
  UIBase::OnNodeEvent(event);
  auto type = OH_ArkUI_NodeEvent_GetEventType(event);
  auto event_data = OH_ArkUI_NodeEvent_GetNodeComponentEvent(event);
  if (type == NODE_IMAGE_ON_COMPLETE && event_data->data[0].i32 == 1) {
    HandleImageSuccessCallback(event_data->data[1].f32,
                               event_data->data[2].f32);
  } else if (has_src_ && type == NODE_IMAGE_ON_ERROR) {
    auto error_code = event_data->data[0].i32;
    HandleImageFailCallback(error_code, error_code == image::kPathErrorCode
                                            ? image::kPathErrorMsg
                                            : image::kFormatErrorMsg);
  }
}

void UIImage::HandleImageSuccessCallback(float image_width,
                                         float image_height) {
  image_width_ = image_width;
  image_height_ = image_height;
  if (context_) {
    AutoSizeIfNeeded();
    if ((event_flags_ & image::kFlagImageLoadEvent) != 0) {
      auto dict = lepus::Dictionary::Create();
      dict->SetValue(image::kLoadEventImageHeight, image_height_);
      dict->SetValue(image::kLoadEventImageWidth, image_width_);
      CustomEvent event{Sign(), image::kLoadEventName, "detail",
                        lepus_value(dict)};
      context_->SendEvent(event);
    }
  }
}

void UIImage::HandleImageFailCallback(float error_code,
                                      const std::string& error_msg) {
  auto src =
      NodeManager::Instance().GetAttribute<std::string>(node_, NODE_IMAGE_SRC);
  LOGE("UIImage load image failed error_code: " << error_code << ", src: "
                                                << src << ", url: " << src_);
  if ((event_flags_ & image::kFlagImageErrorEvent) != 0 && context_) {
    auto dict = lepus::Dictionary::Create();
    dict->SetValue(image::kErrorEventCode, error_code);
    dict->SetValue(image::kErrorEventMsg, error_msg);
    CustomEvent event{Sign(), image::kErrorEventName, "detail",
                      lepus_value(dict)};
    context_->SendEvent(event);
  }
}
void UIImage::AutoSizeIfNeeded() {
  if (image_width_ == 0.f || image_height_ == 0 || !auto_size_) {
    return;
  }
  context_->FindShadowNodeAndRunTask(
      Sign(), [auto_size = auto_size_, image_width = image_width_,
               image_height = image_height_, width = width_,
               height = height_](ShadowNode* shadow_node) {
        auto* image_shadow_node =
            reinterpret_cast<ImageShadowNode*>(shadow_node);
        image_shadow_node->JustSize(auto_size, image_width, image_height, width,
                                    height);
      });
}

void UIImage::UpdateAutoSize(const lepus::Value& value) {
  auto_size_ = value.Bool();
}

void UIImage::UpdateBlurRadius(const lepus::Value& value) {
  CSSStringParser parser = CSSStringParser::FromLepusString(value, {});
  CSSValue radius;
  parser.ParseLengthTo(radius);
  if (!radius.IsEmpty()) {
    NodeManager::Instance().SetAttributeWithNumberValue(
        Node(), NODE_BLUR, radius.AsNumber() * context_->ScaledDensity());
  }
}

void UIImage::SetImageSrcFromPath(const std::string& url, bool placeholder) {
  ArkUI_NodeAttributeType type = placeholder ? NODE_IMAGE_ALT : NODE_IMAGE_SRC;
  bool is_local = base::BeginsWith(url, image::kLocalScheme) ||
                  base::BeginsWith(url, image::kResourceScheme);
  if (is_local) {
    ArkUI_AttributeItem item{.string = url.c_str()};
    NodeManager::Instance().SetAttribute(Node(), type, &item);
    return;
  }
  char* result = nullptr;
  OH_FileUri_GetUriFromPath(url.data(), url.size(), &result);
  ArkUI_AttributeItem item{.string = result};
  NodeManager::Instance().SetAttribute(Node(), type, &item);
  free(result);
}

void UIImage::LoadImageFromURL(bool placeholder) {
  const std::string& url = placeholder ? place_holder_ : src_;
  bool is_base64 = base::BeginsWith(url, image::kBase64Scheme);
  bool is_local = base::BeginsWith(url, image::kLocalScheme) ||
                  base::BeginsWith(url, image::kResourceScheme);
  if (is_base64 || is_local) {
    if (placeholder) {
      ArkUI_AttributeItem item{.string = url.c_str()};
      NodeManager::Instance().SetAttribute(Node(), NODE_IMAGE_ALT, &item);
    } else {
      SetImageSrcAttribute(url, is_base64);
    }
    return;
  }

  if (skip_redirection_) {
    LoadImageResource(url, placeholder
                               ? &UIImage::HandleImagePlaceholderResponse
                               : &UIImage::HandleImageSrcResponse);
    return;
  }

  auto resource_loader = context_->GetResourceLoader();
  if (!resource_loader) {
    return;
  }
  auto request = pub::LynxResourceRequest{url, pub::LynxResourceType::kImage};
  std::string redirect_url = resource_loader->ShouldRedirectUrl(request);
  if (redirect_url != url) {
    if (placeholder) {
      SetImageSrcFromPath(redirect_url, true);
    } else {
      SetImageSrcAttribute(redirect_url, false);
    }
  } else {
    LoadImageResource(url, placeholder
                               ? &UIImage::HandleImagePlaceholderResponse
                               : &UIImage::HandleImageSrcResponse);
  }
}

void UIImage::OnNodeReady() {
  UIBase::OnNodeReady();
  if ((dirty_flags_ & image::kFlagPlaceholderChanged) != 0) {
    LoadImageFromURL(true);
  }
  if ((dirty_flags_ & (image::kFlagSrcChanged | image::kFlagDropShadowChanged |
                       image::kFlagCapInsetsChanged)) != 0) {
    if (!defer_src_invalidation_) {
      NodeManager::Instance().ResetAttribute(Node(), NODE_IMAGE_SRC);
    }
    LoadImageFromURL();
  }
  if ((dirty_flags_ & image::kFlagImageRenderingChanged) != 0) {
    if (rendering_type_ == starlight::ImageRenderingType::kPixelated) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          Node(), NODE_IMAGE_INTERPOLATION,
          static_cast<int32_t>(ARKUI_IMAGE_INTERPOLATION_NONE));
    } else {
      NodeManager::Instance().SetAttributeWithNumberValue(
          Node(), NODE_IMAGE_INTERPOLATION,
          static_cast<int32_t>(ARKUI_IMAGE_INTERPOLATION_LOW));
    }
  }
  if ((dirty_flags_ & image::kFlagTintColorChanged) != 0) {
    if (color_filter_) {
      OH_Drawing_ColorFilterDestroy(color_filter_);
    }
    color_filter_ = OH_Drawing_ColorFilterCreateBlendMode(
        tint_color_, OH_Drawing_BlendMode::BLEND_MODE_SRC_IN);
    ArkUI_AttributeItem item{.object = color_filter_};
    NodeManager::Instance().SetAttribute(Node(), NODE_IMAGE_COLOR_FILTER,
                                         &item);
  }
  if ((dirty_flags_ & image::kFlagPaddingChanged) != 0) {
    if (effect_type_ == LynxImageEffectProcessor::ImageEffect::kNone) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          Node(), NODE_PADDING, padding_top_, padding_right_, padding_bottom_,
          padding_left_);
    } else {
      NodeManager::Instance().ResetAttribute(Node(), NODE_PADDING);
    }
  }
  dirty_flags_ = 0;
}

void UIImage::UpdatePlaceholder(const lepus::Value& value) {
  const auto& value_str = value.StdString();
  if (place_holder_ != value_str) {
    place_holder_ = value_str;
    NodeManager::Instance().ResetAttribute(Node(), NODE_IMAGE_ALT);
    dirty_flags_ |= image::kFlagPlaceholderChanged;
  }
}

void UIImage::SetImageSrcAttribute(const std::string& value, bool is_base64) {
  if (effect_type_ != LynxImageEffectProcessor::ImageEffect::kNone) {
    if (effect_type_ == LynxImageEffectProcessor::ImageEffect::kCapInsets) {
      LynxImageEffectProcessor::CapInsetParams cap_insets_params{
          cap_insets_[3], cap_insets_[0],   cap_insets_[1],
          cap_insets_[2], cap_inset_scale_, GenerateCommonViewParams(),
      };
      HandleImageWithProcessor(value, is_base64, effect_type_,
                               cap_insets_params);
    } else if (effect_type_ ==
               LynxImageEffectProcessor::ImageEffect::kDropShadow) {
      LynxImageEffectProcessor::DropShadowParams shadow_params{
          shadow_radius_, shadow_color_, shadow_offset_x_, shadow_offset_y_,
          GenerateCommonViewParams()};
      HandleImageWithProcessor(value, is_base64, effect_type_, shadow_params);
    }
  } else {
    has_src_ = true;
    if (is_base64) {
      ArkUI_AttributeItem item{.string = value.data()};
      NodeManager::Instance().SetAttribute(Node(), NODE_IMAGE_SRC, &item);
    } else {
      SetImageSrcFromPath(value);
    }
  }
}

void UIImage::SetEvents(const std::vector<lepus::Value>& events) {
  UIBase::SetEvents(events);
  if (events_.empty()) {
    return;
  }
  for (auto& event_ : events_) {
    if (event_ == image::kLoadEventName) {
      event_flags_ |= image::kFlagImageLoadEvent;
    } else if (event_ == image::kErrorEventName) {
      event_flags_ |= image::kFlagImageErrorEvent;
    }
  }
}

void UIImage::LoadImageResource(const std::string& url,
                                UIImage::ImageResourceHandler handler) {
  auto resource_loader = context_->GetResourceLoader();
  if (!resource_loader) {
    return;
  }
  auto request = pub::LynxResourceRequest{url, pub::LynxResourceType::kImage};
  resource_loader->LoadResourcePath(
      request, [weak_self = weak_from_this(), handler = std::move(handler),
                url](pub::LynxPathResponse& response) {
        auto self = weak_self.lock();
        if (!self) {
          return;
        }
        auto ui_image = std::static_pointer_cast<UIImage>(self);
        if (ui_image->GetSrc() == url || ui_image->GetPlaceholder() == url) {
          (ui_image.get()->*handler)(response);
        }
      });
}

void UIImage::HandleImageSrcResponse(pub::LynxPathResponse& response) {
  if (response.Success()) {
    SetImageSrcAttribute(response.path, false);
  } else if (response.err_code == error::E_RESOURCE_IMAGE_PIC_SOURCE) {
    //  During a cold start, the image library may occasionally experience
    //  successful requests that return empty data. As a temporary
    //  workaround,and will address it later.
    ArkUI_AttributeItem item{.string = src_.c_str()};
    NodeManager::Instance().SetAttribute(Node(), NODE_IMAGE_SRC, &item);
  } else {
    HandleImageFailCallback(response.err_code, response.err_msg);
  }
}

void UIImage::HandleImagePlaceholderResponse(pub::LynxPathResponse& response) {
  if (response.Success()) {
    SetImageSrcFromPath(response.path, true);
  }
}

void UIImage::UpdateCapInsets(const lepus::Value& value) {
  const auto& value_str = value.StdString();
  std::vector<std::string> cap_insets_str;
  base::SplitString(value_str, ' ', cap_insets_str);
  cap_insets_.clear();
  dirty_flags_ |= image::kFlagCapInsetsChanged;
  for (const std::string& cap_inset : cap_insets_str) {
    if (cap_inset.length() >= 3 && base::EndsWith(cap_inset, "px")) {
      float cap_inset_value;
      std::string px_str(cap_inset.substr(0, cap_inset.length() - 2));
      if (base::StringToFloat(px_str, cap_inset_value)) {
        cap_insets_.emplace_back(cap_inset_value);
      }
    }
  }
  if (cap_insets_.size() == 4) {
    effect_type_ = LynxImageEffectProcessor::ImageEffect::kCapInsets;
  } else {
    effect_type_ = LynxImageEffectProcessor::ImageEffect::kNone;
  }
}

void UIImage::UpdateCapInsetScale(const lepus::Value& value) {
  if (value.IsNumber()) {
    cap_inset_scale_ = static_cast<float>(value.Number());
  } else if (value.IsString()) {
    const auto& value_str = value.StdString();
    base::StringToFloat(value_str, cap_inset_scale_);
  }
}

void UIImage::UpdateSkipRedirection(const lepus::Value& value) {
  skip_redirection_ = value.Bool();
}

void UIImage::UpdateDeferSrcInvalidation(const lepus::Value& value) {
  defer_src_invalidation_ = value.Bool();
}

void UIImage::SetImageRendering(const lepus::Value& value) {
  UIBase::SetImageRendering(value);
  dirty_flags_ |= image::kFlagImageRenderingChanged;
}

void UIImage::UpdateTintColor(const lepus::Value& value) {
  CSSStringParser parser =
      CSSStringParser::FromLepusString(value, CSSParserConfigs());
  CSSValue color = parser.ParseCSSColor();
  if (color.GetValue().IsEmpty()) {
    LOGD("tint-color is either invalid or undefined, reset it");
    NodeManager::Instance().ResetAttribute(Node(), NODE_IMAGE_COLOR_FILTER);
  } else {
    tint_color_ = color.GetValue().UInt32();
    dirty_flags_ |= image::kFlagTintColorChanged;
  }
}

void UIImage::UpdateDropShadow(const lepus::Value& value) {
  dirty_flags_ |= image::kFlagDropShadowChanged;
  if (value.IsString()) {
    const auto& value_str = value.StdString();
    std::vector<std::string> drop_shadow_str;
    base::SplitString(value_str, ' ', drop_shadow_str);
    if (drop_shadow_str.size() == 4) {
      float screen_size[2] = {0};
      context_->ScreenSize(screen_size);
      const int32_t width_index = 0;
      float pixel_radio = context_->DevicePixelRatio();
      shadow_offset_x_ = LynxUnitUtils::ToVPFromUnitValue(
          drop_shadow_str[0], screen_size[width_index], pixel_radio);
      shadow_offset_y_ = LynxUnitUtils::ToVPFromUnitValue(
          drop_shadow_str[1], screen_size[width_index], pixel_radio);
      shadow_radius_ = LynxUnitUtils::ToVPFromUnitValue(
          drop_shadow_str[2], screen_size[width_index], pixel_radio);
      CSSColor hex_color;
      CSSColor::Parse(drop_shadow_str[3], hex_color);
      shadow_color_ = hex_color.Cast();
      effect_type_ = LynxImageEffectProcessor::ImageEffect::kDropShadow;
      return;
    }
  }
  effect_type_ = LynxImageEffectProcessor::ImageEffect::kNone;
}

LynxImageEffectProcessor::CommonViewParams UIImage::GenerateCommonViewParams() {
  return {width_,
          height_,
          padding_left_,
          padding_top_,
          padding_right_,
          padding_bottom_,
          context_->ScaledDensity()};
}

void UIImage::HandleImageWithProcessor(
    const std::string& url, bool is_base64,
    LynxImageEffectProcessor::ImageEffect effect_type,
    const LynxImageEffectProcessor::EffectParams& params) {
  LynxImageHelper::DecodeImageAsync(
      context_->GetNapiEnv(), url, is_base64,
      [weak_self = weak_from_this()](LynxImageHelper::ImageResponse& response) {
        auto self = weak_self.lock();
        if (!self) {
          return;
        }
        if (!response.Success()) {
          LOGE("decode error " << response.err_code);
          return;
        }
        auto ui_image = std::static_pointer_cast<UIImage>(self);
        std::unique_ptr<LynxBaseImage> pixel_maps = std::move(response.data);
        if (ui_image->drawable_descriptor_) {
          OH_ArkUI_DrawableDescriptor_Dispose(ui_image->drawable_descriptor_);
          ui_image->drawable_descriptor_ = nullptr;
        }
        if (pixel_maps && pixel_maps->FirstFrame()) {
          ui_image->drawable_descriptor_ =
              OH_ArkUI_DrawableDescriptor_CreateFromPixelMap(
                  pixel_maps->FirstFrame()->Bitmap());
          ArkUI_AttributeItem item{.object = ui_image->drawable_descriptor_};
          NodeManager::Instance().SetAttribute(ui_image->Node(), NODE_IMAGE_SRC,
                                               &item);
        }
      },
      LynxImageEffectProcessor(effect_type, params));
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
