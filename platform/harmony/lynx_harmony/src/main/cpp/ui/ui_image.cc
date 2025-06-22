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
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_helper.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_unit_utils.h"

namespace lynx {
namespace tasm {
namespace harmony {
static constexpr const char* const kModeAspectFit = "aspectFit";
static constexpr const char* const kModeAspectFill = "aspectFill";
static constexpr const char* const kModeScaleToFill = "scaleToFill";
static constexpr const char* const kBase64Scheme = "data:image";
static constexpr const char* const kLocalScheme = "file://";
static constexpr const char* const kLoadEventName = "load";
static constexpr const char* const kLoadEventImageWidth = "width";
static constexpr const char* const kLoadEventImageHeight = "height";
static constexpr const char* const kErrorEventName = "error";
static constexpr const char* const kErrorEventCode = "error_code";
static constexpr const char* const kErrorEventMsg = "errMsg";
static constexpr const char* const kStartPlayEventName = "startplay";
static constexpr const char* const kCurrentLoopEventName =
    "currentloopcomplete";
static constexpr const char* const kFinalLoopEventName = "finalloopcomplete";

constexpr uint64_t kFlagSrcChanged = 1;
constexpr uint64_t kFlagPlaceholderChanged = 1 << 1;
constexpr uint64_t kFlagCapInsetsChanged = 1 << 2;
constexpr uint64_t kFlagImageRenderingChanged = 1 << 3;
constexpr uint64_t kFlagTintColorChanged = 1 << 4;
constexpr uint64_t kFlagDropShadowChanged = 1 << 5;
constexpr uint64_t kFlagPaddingChanged = 1 << 6;
constexpr uint64_t kFlagFrameSizeChanged = 1 << 7;
constexpr uint64_t kFlagModeChanged = 1 << 8;

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
    {"skip-redirection", &UIImage::UpdateSkipRedirection},
    {"autoplay", &UIImage::UpdateAutoPlay},
    {"loop-count", &UIImage::UpdateLoopCount}};

std::unordered_map<std::string, UIImage::UIMethod>
    UIImage::image_ui_method_map_ = {
        {"startAnimate", &UIImage::StartAnimation},
        {"pauseAnimation", &UIImage::PauseAnimation},
        {"stopAnimation", &UIImage::StopAnimation},
        {"resumeAnimation", &UIImage::ResumeAnimation}};

UIImage::UIImage(LynxContext* context, int sign, const std::string& tag)
    : UIBase(context, ARKUI_NODE_CUSTOM, sign, tag), mode_(kModeScaleToFill) {
  InitAccessibilityAttrs(LynxAccessibilityMode::kEnable, "image");
}

void UIImage::InvokeMethod(
    const std::string& method, const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (auto it = image_ui_method_map_.find(method);
      it != image_ui_method_map_.end()) {
    (this->*it->second)(args, std::move(callback));
  } else {
    UIBase::InvokeMethod(method, args, std::move(callback));
  }
}

void UIImage::OnPropUpdate(const std::string& name, const lepus::Value& value) {
  UIBase::OnPropUpdate(name, value);
  if (auto it = prop_setters_.find(name); it != prop_setters_.end()) {
    ImagePropSetter setter = it->second;
    (this->*setter)(value);
  }
}

ImageDrawable::ImageMode UIImage::ConvertMode(const std::string& mode) {
  if (mode == kModeAspectFit) {
    return ImageDrawable::ImageMode::kAspectFit;
  }
  if (mode == kModeAspectFill) {
    return ImageDrawable::ImageMode::kAspectFill;
  }
  return ImageDrawable::ImageMode::kScaleToFill;
}

void UIImage::UpdateImageSource(const lepus::Value& value) {
  const auto& value_str = value.StdString();
  if (src_ != value_str) {
    src_ = value_str;
    dirty_flags_ |= kFlagSrcChanged;
  }
}

void UIImage::UpdateImageMode(const lepus::Value& value) {
  const auto& value_str = value.StdString();
  if (mode_ != value_str) {
    mode_ = value_str;
    dirty_flags_ |= kFlagModeChanged;
  }
}

void UIImage::UpdateLayout(float left, float top, float width, float height,
                           const float* paddings, const float* margins,
                           const float* sticky, float max_height,
                           uint32_t node_index) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, UI_IMAGE_UPDATE_LAYOUT);
  if (base::FloatsNotEqual(width, width_)) {
    dirty_flags_ |= kFlagFrameSizeChanged;
  }
  if (base::FloatsNotEqual(height_, height)) {
    dirty_flags_ |= kFlagFrameSizeChanged;
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
    dirty_flags_ |= kFlagPaddingChanged;
  }
  if (base::FloatsNotEqual(image_padding_left_, padding_left_)) {
    image_padding_left_ = padding_left_;
    dirty_flags_ |= kFlagPaddingChanged;
  }
  if (base::FloatsNotEqual(image_padding_right_, padding_right_)) {
    image_padding_right_ = padding_right_;
    dirty_flags_ |= kFlagPaddingChanged;
  }
  if (base::FloatsNotEqual(image_padding_bottom_, padding_bottom_)) {
    image_padding_bottom_ = padding_bottom_;
    dirty_flags_ |= kFlagPaddingChanged;
  }
}

void UIImage::OnNodeEvent(ArkUI_NodeEvent* event) {
  UIBase::OnNodeEvent(event);
}

void UIImage::HandleImageSuccessCallback(float image_width,
                                         float image_height) {
  image_width_ = image_width;
  image_height_ = image_height;
  if (context_) {
    AutoSizeIfNeeded();
    if (has_load_event_) {
      auto dict = lepus::Dictionary::Create();
      dict->SetValue(kLoadEventImageHeight, image_height_);
      dict->SetValue(kLoadEventImageWidth, image_width_);
      CustomEvent event{Sign(), kLoadEventName, "detail", lepus_value(dict)};
      context_->SendEvent(event);
    }
  }
}

void UIImage::HandleImageFailCallback(float error_code,
                                      const std::string& error_msg) {
  LOGE("UIImage load image failed error_code = " << error_code
                                                 << " url = " << src_)
  if (has_error_event_ && context_) {
    auto dict = lepus::Dictionary::Create();
    dict->SetValue(kErrorEventCode, error_code);
    dict->SetValue(kErrorEventMsg, error_msg);
    CustomEvent event{Sign(), kErrorEventName, "detail", lepus_value(dict)};
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

void UIImage::LoadImageFromURL(bool is_src) {
  const std::string& url = is_src ? src_ : place_holder_;
  bool is_base64 = base::BeginsWith(url, kBase64Scheme);
  if (is_base64) {
    SetImageAttribute(url, is_base64, is_src);
    return;
  }
  bool is_local = base::BeginsWith(url, kLocalScheme);
  if (is_local) {
    SetImageAttribute(url, !is_base64, is_src);
    return;
  }

  if (skip_redirection_) {
    LoadImageResource(url, &UIImage::HandleImageSrcResponse);
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
    SetImageAttribute(url, !is_base64, is_src);
  } else {
    LoadImageResource(url, &UIImage::HandleImageSrcResponse);
  }
}

void UIImage::OnNodeReady() {
  UIBase::OnNodeReady();
  if (!src_image_drawable_) {
    src_image_drawable_ = std::make_unique<ImageDrawable>(weak_from_this());
    if (has_start_play_event_ || has_current_loop_event_ ||
        has_final_loop_event_) {
      src_image_drawable_->AddImageAnimationListener(this);
    }
  }
  if (!placeholder_image_drawable_) {
    placeholder_image_drawable_ =
        std::make_unique<ImageDrawable>(weak_from_this());
  }
  if ((dirty_flags_ & (kFlagPaddingChanged | kFlagFrameSizeChanged)) != 0) {
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
  if ((dirty_flags_ & kFlagModeChanged) != 0) {
    if (src_image_drawable_) {
      src_image_drawable_->UpdateMode(ConvertMode(mode_));
    }
    if (placeholder_image_drawable_) {
      placeholder_image_drawable_->UpdateMode(ConvertMode(mode_));
    }
  }
  if ((dirty_flags_ & kFlagImageRenderingChanged) != 0) {
    if (src_image_drawable_) {
      src_image_drawable_->UpdateImageRendering(rendering_type_);
    }
  }
  if ((dirty_flags_ & kFlagTintColorChanged) != 0) {
    if (src_image_drawable_) {
      src_image_drawable_->UpdateTintColor(tint_color_);
    }
  }
  if ((dirty_flags_ & kFlagPlaceholderChanged) != 0) {
    LoadImageFromURL(false);
  }
  if ((dirty_flags_ & (kFlagSrcChanged | kFlagDropShadowChanged |
                       kFlagCapInsetsChanged)) != 0) {
    if (!defer_src_invalidation_) {
      src_image_drawable_->ResetContent();
    }
    LoadImageFromURL(true);
  }
  dirty_flags_ = 0;
}

void UIImage::UpdatePlaceholder(const lepus::Value& value) {
  const auto& value_str = value.StdString();
  if (place_holder_ != value_str) {
    place_holder_ = value_str;
    dirty_flags_ |= kFlagPlaceholderChanged;
  }
}

void UIImage::SetImageAttribute(const std::string& value, bool is_base64,
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

void UIImage::SetEvents(const std::vector<lepus::Value>& events) {
  UIBase::SetEvents(events);
  if (events_.empty()) {
    return;
  }
  for (auto& event_ : events_) {
    if (event_ == kLoadEventName) {
      has_load_event_ = true;
    } else if (event_ == kErrorEventName) {
      has_error_event_ = true;
    } else if (event_ == kCurrentLoopEventName) {
      has_current_loop_event_ = true;
    } else if (event_ == kFinalLoopEventName) {
      has_final_loop_event_ = true;
    } else if (event_ == kStartPlayEventName) {
      has_start_play_event_ = true;
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
    SetImageAttribute(response.path, false, true);
  } else if (response.err_code == error::E_RESOURCE_IMAGE_PIC_SOURCE) {
    // TODO(chengjunnan)
    //  During a cold start, the image library may occasionally experience
    //  successful requests that return empty data. As a temporary
    //  workaround,and will address it later.
    //    ArkUI_AttributeItem item{.string = src_.c_str()};
    //    NodeManager::Instance().SetAttribute(Node(), NODE_IMAGE_SRC, &item);
  } else {
    HandleImageFailCallback(response.err_code, response.err_msg);
  }
}

void UIImage::HandleImagePlaceholderResponse(pub::LynxPathResponse& response) {
  if (response.Success()) {
    SetImageAttribute(response.path, false, false);
  } else if (response.err_code == error::E_RESOURCE_IMAGE_PIC_SOURCE) {
    // TODO(chengjunnan)
    //  During a cold start, the image library may occasionally experience
    //  successful requests that return empty data. As a temporary
    //  workaround,and will address it later.
    //    ArkUI_AttributeItem item{.string = src_.c_str()};
    //    NodeManager::Instance().SetAttribute(Node(), NODE_IMAGE_SRC, &item);
  } else {
    HandleImageFailCallback(response.err_code, response.err_msg);
  }
}

void UIImage::UpdateCapInsets(const lepus::Value& value) {
  const auto& value_str = value.StdString();
  std::vector<std::string> cap_insets_str;
  base::SplitString(value_str, ' ', cap_insets_str);
  cap_insets_.clear();
  dirty_flags_ |= kFlagCapInsetsChanged;
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
  const auto& value_str = value.StdString();
  base::StringToFloat(value_str, cap_inset_scale_);
}

void UIImage::UpdateSkipRedirection(const lepus::Value& value) {
  skip_redirection_ = value.Bool();
}

void UIImage::UpdateDeferSrcInvalidation(const lepus::Value& value) {
  defer_src_invalidation_ = value.Bool();
}

void UIImage::SetImageRendering(const lepus::Value& value) {
  UIBase::SetImageRendering(value);
  dirty_flags_ |= kFlagImageRenderingChanged;
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
    dirty_flags_ |= kFlagTintColorChanged;
  }
}

void UIImage::UpdateDropShadow(const lepus::Value& value) {
  dirty_flags_ |= kFlagDropShadowChanged;
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
        auto ui_image = std::static_pointer_cast<UIImage>(self);
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

void UIImage::OnDraw(OH_Drawing_Canvas* canvas, ArkUI_NodeHandle node) {
  UIBase::OnDraw(canvas, node);
  if (node == Node()) {
    if (placeholder_image_drawable_ &&
        placeholder_image_drawable_->HasContent()) {
      placeholder_image_drawable_->Render(canvas);
    }
    if (src_image_drawable_ && src_image_drawable_->HasContent()) {
      src_image_drawable_->Render(canvas);
    }
  }
}

void UIImage::UpdateAutoPlay(const lepus::Value& value) {
  auto_play_ = value.Bool();
}

void UIImage::UpdateLoopCount(const lepus::Value& value) {
  loop_count_ = value.Number();
}

void UIImage::StartAnimation(
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

void UIImage::StopAnimation(
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

void UIImage::PauseAnimation(
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

void UIImage::ResumeAnimation(
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

void UIImage::onAnimationStart() {
  if (has_start_play_event_) {
    CustomEvent event{Sign(), kStartPlayEventName, "detail", lepus_value()};
    context_->SendEvent(event);
  }
}

void UIImage::onAnimationRepeat() {
  if (has_current_loop_event_) {
    CustomEvent event{Sign(), kCurrentLoopEventName, "detail", lepus_value()};
    context_->SendEvent(event);
  }
}

void UIImage::onAnimationStop() {
  if (has_final_loop_event_ || has_current_loop_event_) {
    CustomEvent current_event{Sign(), kCurrentLoopEventName, "detail",
                              lepus_value()};
    CustomEvent final_event{Sign(), kFinalLoopEventName, "detail",
                            lepus_value()};
    context_->SendEvent(current_event);
    context_->SendEvent(final_event);
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
