// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_flatten_image.h"

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

#include <utility>

#include "base/include/float_comparison.h"
#include "base/include/string/string_number_convert.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/css/css_color.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
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

using ImagePropSetter = void (UIFlattenImage::*)(const lepus::Value& value);
std::unordered_map<std::string, ImagePropSetter> UIFlattenImage::prop_setters_ =
    {{"src", &UIFlattenImage::UpdateImageSource},
     {"mode", &UIFlattenImage::UpdateImageMode},
     {"auto-size", &UIFlattenImage::UpdateAutoSize},
     {"blur-radius", &UIFlattenImage::UpdateBlurRadius},
     {"defer-src-invalidation", &UIFlattenImage::UpdateDeferSrcInvalidation},
     {"placeholder", &UIFlattenImage::UpdatePlaceholder},
     {"tint-color", &UIFlattenImage::UpdateTintColor},
     {"drop-shadow", &UIFlattenImage::UpdateDropShadow},
     {"cap-insets", &UIFlattenImage::UpdateCapInsets},
     {"cap-insets-scale", &UIFlattenImage::UpdateCapInsetScale},
     {"skip-redirection", &UIFlattenImage::UpdateSkipRedirection},
     {"autoplay", &UIFlattenImage::UpdateAutoPlay},
     {"loop-count", &UIFlattenImage::UpdateLoopCount}};

std::unordered_map<std::string, UIFlattenImage::UIMethod>
    UIFlattenImage::image_ui_method_map_ = {
        {"startAnimate", &UIFlattenImage::StartAnimation},
        {"pauseAnimation", &UIFlattenImage::PauseAnimation},
        {"stopAnimation", &UIFlattenImage::StopAnimation},
        {"resumeAnimation", &UIFlattenImage::ResumeAnimation}};

UIFlattenImage::UIFlattenImage(LynxContext* context, int sign,
                               const std::string& tag)
    : UIBase(context, ARKUI_NODE_CUSTOM, sign, tag),
      mode_(image::kModeScaleToFill) {
  InitAccessibilityAttrs(LynxAccessibilityMode::kEnable, "image");
}

void UIFlattenImage::InvokeMethod(
    const std::string& method, const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (auto it = image_ui_method_map_.find(method);
      it != image_ui_method_map_.end()) {
    (this->*it->second)(args, std::move(callback));
  } else {
    UIBase::InvokeMethod(method, args, std::move(callback));
  }
}

void UIFlattenImage::OnPropUpdate(const std::string& name,
                                  const lepus::Value& value) {
  UIBase::OnPropUpdate(name, value);
  if (auto it = prop_setters_.find(name); it != prop_setters_.end()) {
    ImagePropSetter setter = it->second;
    (this->*setter)(value);
  }
}

ImageDrawable::ImageMode UIFlattenImage::ConvertMode(const std::string& mode) {
  if (mode == image::kModeAspectFit) {
    return ImageDrawable::ImageMode::kAspectFit;
  }
  if (mode == image::kModeAspectFill) {
    return ImageDrawable::ImageMode::kAspectFill;
  }
  return ImageDrawable::ImageMode::kScaleToFill;
}

void UIFlattenImage::UpdateImageSource(const lepus::Value& value) {
  const auto& value_str = value.StdString();
  if (src_ != value_str) {
    src_ = value_str;
    dirty_flags_ |= image::kFlagSrcChanged;
  }
}

void UIFlattenImage::UpdateImageMode(const lepus::Value& value) {
  const auto& value_str = value.StdString();
  if (mode_ != value_str) {
    mode_ = value_str;
    dirty_flags_ |= image::kFlagModeChanged;
  }
}

void UIFlattenImage::UpdateLayout(float left, float top, float width,
                                  float height, const float* paddings,
                                  const float* margins, const float* sticky,
                                  float max_height, uint32_t node_index) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_IMAGE_UPDATE_LAYOUT);
  if (base::FloatsNotEqual(width, width_)) {
    dirty_flags_ |= image::kFlagFrameSizeChanged;
  }
  if (base::FloatsNotEqual(height_, height)) {
    dirty_flags_ |= image::kFlagFrameSizeChanged;
  }
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
  if (base::FloatsNotEqual(image_padding_top_, padding_top_)) {
    image_padding_top_ = padding_top_;
    dirty_flags_ |= image::kFlagPaddingChanged;
  }
  if (base::FloatsNotEqual(image_padding_left_, padding_left_)) {
    image_padding_left_ = padding_left_;
    dirty_flags_ |= image::kFlagPaddingChanged;
  }
  if (base::FloatsNotEqual(image_padding_right_, padding_right_)) {
    image_padding_right_ = padding_right_;
    dirty_flags_ |= image::kFlagPaddingChanged;
  }
  if (base::FloatsNotEqual(image_padding_bottom_, padding_bottom_)) {
    image_padding_bottom_ = padding_bottom_;
    dirty_flags_ |= image::kFlagPaddingChanged;
  }
}

void UIFlattenImage::OnNodeEvent(ArkUI_NodeEvent* event) {
  UIBase::OnNodeEvent(event);
}

void UIFlattenImage::HandleImageSuccessCallback(float image_width,
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

void UIFlattenImage::HandleImageFailCallback(float error_code,
                                             const std::string& error_msg) {
  LOGE("UIFlattenImage load image failed error_code = " << error_code
                                                        << " url = " << src_)
  if ((event_flags_ & image::kFlagImageErrorEvent) != 0 && context_) {
    auto dict = lepus::Dictionary::Create();
    dict->SetValue(image::kErrorEventCode, error_code);
    dict->SetValue(image::kErrorEventMsg, error_msg);
    CustomEvent event{Sign(), image::kErrorEventName, "detail",
                      lepus_value(dict)};
    context_->SendEvent(event);
  }
}

void UIFlattenImage::AutoSizeIfNeeded() {
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

void UIFlattenImage::UpdateAutoSize(const lepus::Value& value) {
  auto_size_ = value.Bool();
}

void UIFlattenImage::UpdateBlurRadius(const lepus::Value& value) {
  CSSStringParser parser = CSSStringParser::FromLepusString(value, {});
  CSSValue radius;
  parser.ParseLengthTo(radius);
  if (!radius.IsEmpty()) {
    NodeManager::Instance().SetAttributeWithNumberValue(
        Node(), NODE_BLUR, radius.AsNumber() * context_->ScaledDensity());
  }
}

void UIFlattenImage::LoadImageFromURL(bool is_src) {
  const std::string& url = is_src ? src_ : place_holder_;
  bool is_base64 = base::BeginsWith(url, image::kBase64Scheme);
  bool is_local = base::BeginsWith(url, image::kLocalScheme) ||
                  base::BeginsWith(url, image::kResourceScheme);
  if (is_base64 || is_local) {
    SetImageAttribute(url, is_base64, is_src);
    return;
  }

  if (skip_redirection_) {
    LoadImageResource(url,
                      is_src ? &UIFlattenImage::HandleImageSrcResponse
                             : &UIFlattenImage::HandleImagePlaceholderResponse);
    return;
  }

  auto resource_loader = context_->GetResourceLoader();
  if (!resource_loader) {
    return;
  }
  auto request = pub::LynxResourceRequest{url, pub::LynxResourceType::kImage};
  std::string redirect_url = resource_loader->ShouldRedirectUrl(request);
  if (redirect_url != url) {
    // local resource
    SetImageAttribute(url, false, is_src);
  } else {
    LoadImageResource(url,
                      is_src ? &UIFlattenImage::HandleImageSrcResponse
                             : &UIFlattenImage::HandleImagePlaceholderResponse);
  }
}

bool UIFlattenImage::hasAnimationEvent() {
  return (event_flags_ &
          (image::kFlagImageCurrentLoopEvent | image::kFlagImageFinalLoopEvent |
           image::kFlagImageStartPlayEvent)) != 0;
}

void UIFlattenImage::OnNodeReady() {
  UIBase::OnNodeReady();
  if (!src_image_drawable_) {
    src_image_drawable_ = std::make_unique<ImageDrawable>(weak_from_this());
    if (hasAnimationEvent()) {
      src_image_drawable_->AddImageAnimationListener(this);
    }
  }
  if (!placeholder_image_drawable_) {
    placeholder_image_drawable_ =
        std::make_unique<ImageDrawable>(weak_from_this());
  }
  if ((dirty_flags_ &
       (image::kFlagPaddingChanged | image::kFlagFrameSizeChanged)) != 0) {
    if (src_image_drawable_) {
      src_image_drawable_->UpdateBounds(
          0, 0, width_, height_, padding_left_, padding_top_, padding_right_,
          padding_bottom_, context_->ScaledDensity());
    }
    if (placeholder_image_drawable_) {
      placeholder_image_drawable_->UpdateBounds(
          0, 0, width_, height_, padding_left_, padding_top_, padding_right_,
          padding_bottom_, context_->ScaledDensity());
    }
  }
  if ((dirty_flags_ & image::kFlagModeChanged) != 0) {
    if (src_image_drawable_) {
      src_image_drawable_->UpdateMode(ConvertMode(mode_));
    }
    if (placeholder_image_drawable_) {
      placeholder_image_drawable_->UpdateMode(ConvertMode(mode_));
    }
  }
  if ((dirty_flags_ & image::kFlagImageRenderingChanged) != 0) {
    if (src_image_drawable_) {
      src_image_drawable_->UpdateImageRendering(rendering_type_);
    }
  }
  if ((dirty_flags_ & image::kFlagTintColorChanged) != 0) {
    if (src_image_drawable_) {
      src_image_drawable_->UpdateTintColor(tint_color_);
    }
  }
  if ((dirty_flags_ & image::kFlagPlaceholderChanged) != 0) {
    LoadImageFromURL(false);
  }
  if ((dirty_flags_ & (image::kFlagSrcChanged | image::kFlagDropShadowChanged |
                       image::kFlagCapInsetsChanged)) != 0) {
    if (!defer_src_invalidation_) {
      src_image_drawable_->ResetContent();
    }
    LoadImageFromURL(true);
  }
  dirty_flags_ = 0;
}

void UIFlattenImage::UpdatePlaceholder(const lepus::Value& value) {
  const auto& value_str = value.StdString();
  if (place_holder_ != value_str) {
    place_holder_ = value_str;
    dirty_flags_ |= image::kFlagPlaceholderChanged;
  }
}

void UIFlattenImage::SetImageAttribute(const std::string& value, bool is_base64,
                                       bool is_src) {
  LynxImageEffectProcessor::EffectParams params{};
  LynxImageEffectProcessor::ImageEffect effect =
      LynxImageEffectProcessor::ImageEffect::kNone;
  if (is_src) {
    effect = effect_type_;
    if (effect_type_ != LynxImageEffectProcessor::ImageEffect::kNone) {
      if (effect_type_ == LynxImageEffectProcessor::ImageEffect::kCapInsets) {
        LynxImageEffectProcessor::CapInsetParams cap_insets_params{
            cap_insets_[3], cap_insets_[0],   cap_insets_[1],
            cap_insets_[2], cap_inset_scale_, GenerateCommonViewParams(),
        };
        params = cap_insets_params;
      } else if (effect_type_ ==
                 LynxImageEffectProcessor::ImageEffect::kDropShadow) {
        LynxImageEffectProcessor::DropShadowParams shadow_params{
            shadow_radius_, shadow_color_, shadow_offset_x_, shadow_offset_y_,
            GenerateCommonViewParams()};
        params = shadow_params;
      }
    }
  }
  HandleImageWithProcessor(value, is_base64, effect, params, is_src);
}

void UIFlattenImage::SetEvents(const std::vector<lepus::Value>& events) {
  UIBase::SetEvents(events);
  if (events_.empty()) {
    return;
  }
  for (auto& event_ : events_) {
    if (event_ == image::kLoadEventName) {
      event_flags_ |= image::kFlagImageLoadEvent;
    } else if (event_ == image::kErrorEventName) {
      event_flags_ |= image::kFlagImageErrorEvent;
    } else if (event_ == image::kCurrentLoopEventName) {
      event_flags_ |= image::kFlagImageCurrentLoopEvent;
    } else if (event_ == image::kFinalLoopEventName) {
      event_flags_ |= image::kFlagImageFinalLoopEvent;
    } else if (event_ == image::kStartPlayEventName) {
      event_flags_ |= image::kFlagImageStartPlayEvent;
    }
  }
}

void UIFlattenImage::LoadImageResource(
    const std::string& url, UIFlattenImage::ImageResourceHandler handler) {
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
        auto ui_image = std::static_pointer_cast<UIFlattenImage>(self);
        if (ui_image->GetSrc() == url || ui_image->GetPlaceholder() == url) {
          (ui_image.get()->*handler)(response);
        }
      });
}

void UIFlattenImage::HandleImageSrcResponse(pub::LynxPathResponse& response) {
  if (response.Success()) {
    SetImageAttribute(response.path, false, true);
  } else if (response.err_code == error::E_RESOURCE_IMAGE_PIC_SOURCE) {
    //  During a cold start, the image library may occasionally experience
    //  successful requests that return empty data. As a temporary
    //  workaround,and will address it later.
  } else {
    HandleImageFailCallback(response.err_code, response.err_msg);
  }
}

void UIFlattenImage::HandleImagePlaceholderResponse(
    pub::LynxPathResponse& response) {
  if (response.Success()) {
    SetImageAttribute(response.path, false, false);
  } else if (response.err_code == error::E_RESOURCE_IMAGE_PIC_SOURCE) {
    //  During a cold start, the image library may occasionally experience
    //  successful requests that return empty data. As a temporary
    //  workaround,and will address it later.
  } else {
    HandleImageFailCallback(response.err_code, response.err_msg);
  }
}

void UIFlattenImage::UpdateCapInsets(const lepus::Value& value) {
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

void UIFlattenImage::UpdateCapInsetScale(const lepus::Value& value) {
  if (value.IsNumber()) {
    cap_inset_scale_ = static_cast<float>(value.Number());
  } else if (value.IsString()) {
    const auto& value_str = value.StdString();
    base::StringToFloat(value_str, cap_inset_scale_);
  }
}

void UIFlattenImage::UpdateSkipRedirection(const lepus::Value& value) {
  skip_redirection_ = value.Bool();
}

void UIFlattenImage::UpdateDeferSrcInvalidation(const lepus::Value& value) {
  defer_src_invalidation_ = value.Bool();
}

void UIFlattenImage::SetImageRendering(const lepus::Value& value) {
  UIBase::SetImageRendering(value);
  dirty_flags_ |= image::kFlagImageRenderingChanged;
}

void UIFlattenImage::UpdateTintColor(const lepus::Value& value) {
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

void UIFlattenImage::UpdateDropShadow(const lepus::Value& value) {
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

LynxImageEffectProcessor::CommonViewParams
UIFlattenImage::GenerateCommonViewParams() {
  return {width_,
          height_,
          padding_left_,
          padding_top_,
          padding_right_,
          padding_bottom_,
          context_->ScaledDensity()};
}

void UIFlattenImage::HandleImageWithProcessor(
    const std::string& url, bool is_base64,
    LynxImageEffectProcessor::ImageEffect effect_type,
    const LynxImageEffectProcessor::EffectParams& params, bool is_src) {
  LynxImageHelper::DecodeImageAsync(
      context_->GetNapiEnv(), url, is_base64,
      [is_src,
       weak_self = weak_from_this()](LynxImageHelper::ImageResponse& response) {
        auto self = weak_self.lock();
        if (!self) {
          return;
        }
        if (!response.Success()) {
          LOGE("decode error " << response.err_code);
          return;
        }
        auto ui_image = std::static_pointer_cast<UIFlattenImage>(self);
        if (is_src) {
          ui_image->HandleImageSuccessCallback(response.data->Width(),
                                               response.data->Height());
          ui_image->src_image_drawable_->UpdateLoopCount(ui_image->loop_count_);
          ui_image->src_image_drawable_->UpdateDrawCurrent(
              std::move(response.data));
          if (ui_image->auto_play_) {
            ui_image->src_image_drawable_->StartAnimation();
          }
        } else {
          ui_image->placeholder_image_drawable_->UpdateDrawCurrent(
              std::move(response.data));
        }
      },
      LynxImageEffectProcessor(effect_type, params));
}

void UIFlattenImage::OnDraw(OH_Drawing_Canvas* canvas, ArkUI_NodeHandle node) {
  UIBase::OnDraw(canvas, node);
  if (node == Node()) {
    Render(canvas);
  }
}

void UIFlattenImage::OnDrawBehind(OH_Drawing_Canvas* canvas,
                                  ArkUI_NodeHandle node) {
  UIBase::OnDrawBehind(canvas, node);
  Render(canvas);
}

void UIFlattenImage::UpdateAutoPlay(const lepus::Value& value) {
  auto_play_ = value.Bool();
}

void UIFlattenImage::UpdateLoopCount(const lepus::Value& value) {
  loop_count_ = static_cast<int32_t>(value.Number());
}

void UIFlattenImage::StartAnimation(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (src_image_drawable_ && src_image_drawable_->IsAnimate()) {
    src_image_drawable_->StopAnimation();
    src_image_drawable_->StartAnimation();
    callback(LynxGetUIResult::SUCCESS, lepus::Value("Animation started."));
  } else {
    callback(LynxGetUIResult::PARAM_INVALID,
             lepus::Value("Not support start yet"));
  }
}

void UIFlattenImage::StopAnimation(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (src_image_drawable_ && src_image_drawable_->IsAnimate()) {
    src_image_drawable_->StopAnimation();
    callback(LynxGetUIResult::SUCCESS, lepus::Value("Animation stopped."));
  } else {
    callback(LynxGetUIResult::PARAM_INVALID,
             lepus::Value("Not support stop yet"));
  }
}

void UIFlattenImage::PauseAnimation(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (src_image_drawable_ && src_image_drawable_->IsAnimate()) {
    src_image_drawable_->PauseAnimation();
    callback(LynxGetUIResult::SUCCESS, lepus::Value("Animation paused."));
  } else {
    callback(LynxGetUIResult::PARAM_INVALID,
             lepus::Value("Not support pause yet"));
  }
}

void UIFlattenImage::ResumeAnimation(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (src_image_drawable_ && src_image_drawable_->IsAnimate()) {
    src_image_drawable_->StartAnimation();
    callback(LynxGetUIResult::SUCCESS, lepus::Value("Animation resumed."));
  } else {
    callback(LynxGetUIResult::PARAM_INVALID,
             lepus::Value("Not support resume yet"));
  }
}

void UIFlattenImage::onAnimationStart() {
  if ((event_flags_ & image::kFlagImageStartPlayEvent) != 0) {
    CustomEvent event{Sign(), image::kStartPlayEventName, "detail",
                      lepus_value()};
    context_->SendEvent(event);
  }
}

void UIFlattenImage::onAnimationRepeat() {
  if ((event_flags_ & image::kFlagImageCurrentLoopEvent) != 0) {
    CustomEvent event{Sign(), image::kCurrentLoopEventName, "detail",
                      lepus_value()};
    context_->SendEvent(event);
  }
}

void UIFlattenImage::onAnimationStop() {
  if ((event_flags_ & image::kFlagImageCurrentLoopEvent) != 0) {
    CustomEvent event{Sign(), image::kCurrentLoopEventName, "detail",
                      lepus_value()};
    context_->SendEvent(event);
  }
  if ((event_flags_ & image::kFlagImageFinalLoopEvent) != 0) {
    CustomEvent event{Sign(), image::kFinalLoopEventName, "detail",
                      lepus_value()};
    context_->SendEvent(event);
  }
}

void UIFlattenImage::Render(OH_Drawing_Canvas* canvas) const {
  if (placeholder_image_drawable_ &&
      placeholder_image_drawable_->HasContent()) {
    placeholder_image_drawable_->Render(canvas);
  }
  if (src_image_drawable_ && src_image_drawable_->HasContent()) {
    src_image_drawable_->Render(canvas);
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
