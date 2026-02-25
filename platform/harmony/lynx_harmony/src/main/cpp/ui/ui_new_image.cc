// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_new_image.h"

#include <filemanagement/file_uri/oh_file_uri.h>
#include <native_drawing/drawing_color_filter.h>

#include <cstdlib>
#include <utility>

#include "base/include/float_comparison.h"
#include "base/include/string/string_number_convert.h"
#include "base/trace/native/trace_event.h"
#include "clay/ui/component/component_constants.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/renderer/css/css_color.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/image_shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_config.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_constants.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_helper.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_unit_utils.h"

namespace lynx {
namespace tasm {
namespace harmony {

ImageService* UINewImage::image_service = nullptr;

using ImagePropSetter = void (UINewImage::*)(const lepus::Value& value);
std::unordered_map<std::string, ImagePropSetter> UINewImage::prop_setters_ = {
    {"src", &UINewImage::UpdateImageSource},
    {"mode", &UINewImage::UpdateImageMode},
    {"auto-size", &UINewImage::UpdateAutoSize},
    {"blur-radius", &UINewImage::UpdateBlurRadius},
    {"downsampling", &UINewImage::UpdateDownsampling},
    {"defer-src-invalidation", &UINewImage::UpdateDeferSrcInvalidation},
    {"placeholder", &UINewImage::UpdatePlaceholder},
    {"tint-color", &UINewImage::UpdateTintColor},
    {"drop-shadow", &UINewImage::UpdateDropShadow},
    {"cap-insets", &UINewImage::UpdateCapInsets},
    {"cap-insets-scale", &UINewImage::UpdateCapInsetScale},
    {"skip-redirection", &UINewImage::UpdateSkipRedirection},
    {"autoplay", &UINewImage::UpdateAutoPlay},
    {"loop-count", &UINewImage::UpdateLoopCount},
    {"enable-report-info", &UINewImage::UpdateEnableReportInfo}};

std::unordered_map<std::string, UINewImage::UIMethod>
    UINewImage::image_ui_method_map_ = {
        {"startAnimate", &UINewImage::StartAnimation},
        {"pauseAnimation", &UINewImage::PauseAnimation},
        {"stopAnimation", &UINewImage::StopAnimation},
        {"resumeAnimation", &UINewImage::ResumeAnimation}};

void UINewImage::OnPropUpdate(const std::string& name,
                              const lepus::Value& value) {
  UIBase::OnPropUpdate(name, value);
  if (auto it = prop_setters_.find(name); it != prop_setters_.end()) {
    ImagePropSetter setter = it->second;
    (this->*setter)(value);
  }
}

void UINewImage::OnDrawBehind(OH_Drawing_Canvas* canvas,
                              ArkUI_NodeHandle node) {
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

void UINewImage::OnImageLoadSuccess(float image_width, float image_height) {
  image_width_ = image_width;
  image_height_ = image_height;
  if (context_) {
    context_->PostTaskOnUIThread([weak_self = weak_from_this()] {
      auto self = weak_self.lock();
      if (!self) {
        return;
      }
      auto image = std::static_pointer_cast<UINewImage>(self);
      image->AutoSizeIfNeeded();
    });

    if ((event_flags_ & image::kFlagImageLoadEvent) != 0 &&
        !enable_report_info_) {
      auto dict = lepus::Dictionary::Create();
      dict->SetValue(image::kLoadEventImageHeight, image_height_);
      dict->SetValue(image::kLoadEventImageWidth, image_width_);
      CustomEvent event{Sign(), image::kLoadEventName, "detail",
                        lepus_value(dict)};
      context_->SendEvent(event);
    }
  }
}

void UINewImage::OnImageMonitorInfo(const ImageMonitorInfo& data) {
  if (enable_image_load_callback_ &&
      base::BeginsWith(src_, image::kHttpPrefix)) {
    // Report image monitoring information to the native side
    auto image_info = lepus::Dictionary::Create();
    image_info->SetValue("type",
                         static_cast<int32_t>(pub::LynxResourceType::kImage));
    image_info->SetValue("src", src_);
    image_info->SetValue("loadStart", data.load_start);
    image_info->SetValue("loadFinish", data.load_finish);
    image_info->SetValue("cost", data.load_finish - data.load_start);
    image_info->SetValue(image::kLoadEventImageHeight, image_height_);
    image_info->SetValue(image::kLoadEventImageWidth, image_width_);
    image_info->SetValue("viewWidth", width_);
    image_info->SetValue("viewHeight", height_);
    image_info->SetValue("origin", static_cast<int32_t>(data.origin));
    UIBase::OnResourceLoadCallback(lepus_value(image_info));
  }
  if (context_) {
    // Report image monitoring information to the frontend
    if ((event_flags_ & image::kFlagImageLoadEvent) != 0 &&
        enable_report_info_) {
      auto dict = lepus::Dictionary::Create();
      dict->SetValue("src", src_);
      dict->SetValue(image::kLoadEventImageHeight, image_height_);
      dict->SetValue(image::kLoadEventImageWidth, image_width_);
      dict->SetValue("view_height", height_);
      dict->SetValue("view_width", width_);
      dict->SetValue("memory_cost", image_height_ * image_width_ * 4);
      dict->SetValue("load_start", data.load_start);
      dict->SetValue("load_finish", data.load_finish);
      dict->SetValue("cost", data.load_finish - data.load_start);
      dict->SetValue("origin", static_cast<int32_t>(data.origin));
      CustomEvent event{Sign(), image::kLoadEventName, "detail",
                        lepus_value(dict)};
      context_->SendEvent(event);
    }
  }
}

void UINewImage::OnImageLoadFailure(int error_code,
                                    const std::string& error_msg) {
  LOGE("Load image failed error_code: " << error_code
                                        << ", error_msg: " << error_msg);
  if (enable_image_load_callback_ &&
      base::BeginsWith(src_, image::kHttpPrefix)) {
    auto image_info = lepus::Dictionary::Create();
    image_info->SetValue("type",
                         static_cast<int32_t>(pub::LynxResourceType::kImage));
    image_info->SetValue("src", src_);
    image_info->SetValue("errCode", error_code);
    image_info->SetValue("errMsg", error_msg);
    UIBase::OnResourceLoadCallback(lepus_value(image_info));
  }

  if ((event_flags_ & image::kFlagImageErrorEvent) != 0 && context_ &&
      has_src_) {
    auto dict = lepus::Dictionary::Create();
    dict->SetValue(image::kErrorEventCode, error_code);
    dict->SetValue(image::kErrorEventMsg, error_msg);
    CustomEvent event{Sign(), image::kErrorEventName, "detail",
                      lepus_value(dict)};
    context_->SendEvent(event);
  }
}

void UINewImage::OnAnimationStart() {
  if ((event_flags_ & image::kFlagImageStartPlayEvent) != 0) {
    CustomEvent event{Sign(), image::kStartPlayEventName, "detail",
                      lepus_value()};
    context_->SendEvent(event);
  }
}

void UINewImage::OnAnimationRepeat() {
  if ((event_flags_ & image::kFlagImageCurrentLoopEvent) != 0) {
    CustomEvent event{Sign(), image::kCurrentLoopEventName, "detail",
                      lepus_value()};
    context_->SendEvent(event);
  }
}

void UINewImage::OnAnimationFinish() {
  if ((event_flags_ & image::kFlagImageFinalLoopEvent) != 0) {
    CustomEvent event{Sign(), image::kFinalLoopEventName, "detail",
                      lepus_value()};
    context_->SendEvent(event);
  }
}

UINewImage::UINewImage(LynxContext* context, int sign, const std::string& tag)
    : UIBase(context, ARKUI_NODE_UNDEFINED, sign, tag),
      mode_(image::kModeScaleToFill) {
  image_node_ = image_service->CreateImageNode();
  InitNode(image_node_->GetNodeHandle());
  InitAccessibilityAttrs(LynxAccessibilityMode::kEnable, "image");

  if (context_->GetUIOwner()) {
    LynxImageConfig* config = context_->GetUIOwner()->GetLynxImageConfig();
    if (config && config->GetEnableImageLoadCallback()) {
      enable_image_load_callback_ = true;
    }
  }
}

ArkUI_ObjectFit UINewImage::ConvertMode(const std::string& mode) {
  if (mode == image::kModeAspectFit) {
    return ARKUI_OBJECT_FIT_CONTAIN;
  }
  if (mode == image::kModeAspectFill) {
    return ARKUI_OBJECT_FIT_COVER;
  }
  if (mode == image::kModeCenter) {
    return ARKUI_OBJECT_FIT_NONE;
  }
  return ARKUI_OBJECT_FIT_FILL;
}

void UINewImage::UpdateImageSource(const lepus::Value& value) {
  const auto& value_str = value.StdString();
  if (src_ != value_str) {
    src_ = value_str;
    dirty_flags_ |= image::kFlagSrcChanged;
  }
}

void UINewImage::UpdateImageMode(const lepus::Value& value) {
  const auto& value_str = value.StdString();
  if (mode_ != value_str) {
    mode_ = value_str;
    dirty_flags_ |= image::kFlagSrcChanged;
  }
}

void UINewImage::UpdateLayout(float left, float top, float width, float height,
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

  if (image_view_width_ != width) {
    image_view_width_ = width;
    dirty_flags_ |= image::kFlagFrameSizeChanged;
  }
  if (image_view_height_ != height) {
    image_view_height_ = height;
    dirty_flags_ |= image::kFlagFrameSizeChanged;
  }
}

UINewImage::~UINewImage() {
  Destroy();
  image_node_.reset();
  // Released by image_node_, shouldn't invoke DisposeNode again
  node_ = nullptr;
  if (color_filter_) {
    OH_Drawing_ColorFilterDestroy(color_filter_);
  }
}

void UINewImage::AutoSizeIfNeeded() {
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

void UINewImage::UpdateAutoSize(const lepus::Value& value) {
  auto_size_ = value.Bool();
}

void UINewImage::UpdateBlurRadius(const lepus::Value& value) {
  CSSStringParser parser = CSSStringParser::FromLepusString(value, {});
  CSSValue radius;
  parser.ParseLengthTo(radius);
  if (!radius.IsEmpty()) {
    NodeManager::Instance().SetAttributeWithNumberValue(
        Node(), NODE_BLUR, radius.AsNumber() * context_->ScaledDensity());
  }
}

void UINewImage::InitAnimationListener() {
  std::weak_ptr weak_self = std::static_pointer_cast<AnimationListener>(
      std::static_pointer_cast<UINewImage>(shared_from_this()));
  image_node_->InitAnimationListener(weak_self);
}

void UINewImage::StartAnimation(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  image_node_->StartAnimation();
  callback(LynxGetUIResult::SUCCESS, lepus_value(""));
}

void UINewImage::StopAnimation(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  image_node_->StopAnimation();
  callback(LynxGetUIResult::SUCCESS, lepus_value(""));
}

void UINewImage::PauseAnimation(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  image_node_->PauseAnimation();
  callback(LynxGetUIResult::SUCCESS, lepus_value(""));
}

void UINewImage::ResumeAnimation(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  image_node_->ResumeAnimation();
  callback(LynxGetUIResult::SUCCESS, lepus_value(""));
}

std::string UINewImage::GetRedirectUrl(const std::string& url) {
  if (url.empty() || base::BeginsWith(url, image::kBase64Scheme) ||
      base::BeginsWith(url, image::kLocalScheme) ||
      base::BeginsWith(url, image::kResourceScheme)) {
    return url;
  }
  auto resource_loader = context_->GetResourceLoader();
  if (!resource_loader) {
    return url;
  }
  pub::LynxResourceRequest request{url, pub::LynxResourceType::kImage};
  std::string redirect_url = resource_loader->ShouldRedirectUrl(request);
  if (!redirect_url.empty() && redirect_url[0] == '/') {
    char* result = nullptr;
    auto code = OH_FileUri_GetUriFromPath(redirect_url.data(),
                                          redirect_url.size(), &result);
    if (code == ERR_OK && result != nullptr) {
      redirect_url = result;
      free(result);
    } else {
      LOGE("GetRedirectUrl failed, code: "
           << code << ", url: " << url << ", redirect_url: " << redirect_url);
    }
  }
  return redirect_url;
}

void UINewImage::LoadImage() {
  // When using a processor, the view size is used as the cache key.
  // Requests can be deferred until the size is available to avoid the processor
  // returning an empty pixelmap.
  if (effect_flags_ != 0 && (width_ <= 0 || height_ <= 0) && !auto_size_) {
    LOGE("LoadImage empty size, src: " << src_);
    return;
  }
  if (skip_redirection_) {
    LoadImageFromService(src_, placeholder_);
    return;
  }

  std::string final_src = GetRedirectUrl(src_);
  std::string final_placeholder = GetRedirectUrl(placeholder_);
  LoadImageFromService(final_src, final_placeholder);
}

void UINewImage::OnNodeReady() {
  UIBase::OnNodeReady();
  if (!init_listener_) {
    std::weak_ptr weak_self = std::static_pointer_cast<ImageLoadListener>(
        std::static_pointer_cast<UINewImage>(shared_from_this()));
    image_node_->InitImageLoadListener(weak_self);
    init_listener_ = true;
  }

  if ((dirty_flags_ & (image::kFlagSrcChanged | image::kFlagPlaceholderChanged |
                       image::kFlagEffectChanged)) != 0 ||
      (effect_flags_ != 0 &&
       (dirty_flags_ & image::kFlagFrameSizeChanged) != 0)) {
    if (!defer_src_invalidation_) {
      NodeManager::Instance().ResetAttribute(Node(), NODE_IMAGE_SRC);
    }
    LoadImage();
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
    color_filter_ =
        OH_Drawing_ColorFilterCreateBlendMode(tint_color_, BLEND_MODE_SRC_IN);
    ArkUI_AttributeItem item{.object = color_filter_};
    NodeManager::Instance().SetAttribute(Node(), NODE_IMAGE_COLOR_FILTER,
                                         &item);
  }
  if ((dirty_flags_ & image::kFlagPaddingChanged) != 0) {
    if (effect_flags_ == 0) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          Node(), NODE_PADDING, padding_top_, padding_right_, padding_bottom_,
          padding_left_);
    } else {
      NodeManager::Instance().ResetAttribute(Node(), NODE_PADDING);
    }
  }
  dirty_flags_ = 0;
}
void UINewImage::InvokeMethod(
    const std::string& method, const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (auto it = image_ui_method_map_.find(method);
      it != image_ui_method_map_.end()) {
    (this->*it->second)(args, std::move(callback));
  } else {
    UIBase::InvokeMethod(method, args, std::move(callback));
  }
}

void UINewImage::UpdateAutoPlay(const lepus::Value& value) {
  autoplay_ = value.Bool();
  image_node_->UpdateAutoPlay(autoplay_);
}

void UINewImage::UpdateLoopCount(const lepus::Value& value) {
  loop_count_ = static_cast<int32_t>(value.Number());
  image_node_->UpdateLoopCount(loop_count_);
}

void UINewImage::UpdatePlaceholder(const lepus::Value& value) {
  const auto& value_str = value.StdString();
  if (placeholder_ != value_str) {
    placeholder_ = value_str;
    NodeManager::Instance().ResetAttribute(Node(), NODE_IMAGE_ALT);
    dirty_flags_ |= image::kFlagPlaceholderChanged;
  }
}

void UINewImage::SetEvents(const std::vector<lepus::Value>& events) {
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

  if (event_flags_ & image::kFlagImageAnimationEvent) {
    InitAnimationListener();
  }
}

void UINewImage::LoadImageFromService(const std::string& url,
                                      const std::string& placeholder) {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  OH_ArkUI_NodeUtils_AddCustomProperty(node_, "rawSrc", src_.c_str());
  OH_ArkUI_NodeUtils_AddCustomProperty(node_, "src", url.c_str());
  OH_ArkUI_NodeUtils_AddCustomProperty(node_, "placeholder",
                                       placeholder.c_str());
  OH_ArkUI_NodeUtils_AddCustomProperty(node_, "mode", mode_.c_str());
  OH_ArkUI_NodeUtils_AddCustomProperty(node_, "skip-redirection",
                                       skip_redirection_ ? "true" : "false");
  OH_ArkUI_NodeUtils_AddCustomProperty(node_, "auto-size",
                                       auto_size_ ? "true" : "false");
#endif
  has_src_ = !url.empty();
  if (url.empty() && placeholder.empty()) {
    LOGE("LoadImageFromService empty source return");
    return;
  }
  ImageRequestInfo info{
      .url = url,
      .placeholder = placeholder,
  };
  using ImageEffect = LynxImageEffectProcessor::ImageEffect;
  std::vector<std::unique_ptr<ImageProcessor>> processors;
  if ((effect_flags_ & image::kFlagEffectCapInsets) && width_ > 0 &&
      height_ > 0) {
    LynxImageEffectProcessor::CapInsetParams cap_insets_params{
        cap_insets_[3], cap_insets_[0],   cap_insets_[1],
        cap_insets_[2], cap_inset_scale_, GenerateCommonViewParams(),
    };
    processors.emplace_back(std::make_unique<LynxImageEffectProcessor>(
        ImageEffect::kCapInsets, cap_insets_params));
  }
  if ((effect_flags_ & image::kFlagEffectDropShadow) && width_ > 0 &&
      height_ > 0) {
    LynxImageEffectProcessor::DropShadowParams shadow_params{
        shadow_radius_, shadow_color_, shadow_offset_x_, shadow_offset_y_,
        GenerateCommonViewParams()};
    processors.emplace_back(std::make_unique<LynxImageEffectProcessor>(
        ImageEffect::kDropShadow, shadow_params));
  }
  info.downsampling = downsampling_;
  info.mode = ConvertMode(mode_);
  info.processors = std::move(processors);
  image_node_->FetchImage(std::move(info));
}

void UINewImage::UpdateCapInsets(const lepus::Value& value) {
  const auto& value_str = value.StdString();
  std::vector<std::string> cap_insets_str;
  base::SplitString(value_str, ' ', cap_insets_str);
  cap_insets_.clear();
  dirty_flags_ |= image::kFlagEffectChanged;
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
    effect_flags_ |= image::kFlagEffectCapInsets;
  } else {
    effect_flags_ &= ~image::kFlagEffectCapInsets;
  }
}

void UINewImage::UpdateCapInsetScale(const lepus::Value& value) {
  if (value.IsNumber()) {
    cap_inset_scale_ = static_cast<float>(value.Number());
  } else if (value.IsString()) {
    const auto& value_str = value.StdString();
    base::StringToFloat(value_str, cap_inset_scale_);
  }
}

void UINewImage::UpdateSkipRedirection(const lepus::Value& value) {
  skip_redirection_ = value.Bool();
}

void UINewImage::UpdateDownsampling(const lepus::Value& value) {
  downsampling_ = value.Bool();
}

void UINewImage::UpdateDeferSrcInvalidation(const lepus::Value& value) {
  defer_src_invalidation_ = value.Bool();
}

void UINewImage::SetImageRendering(const lepus::Value& value) {
  UIBase::SetImageRendering(value);
  dirty_flags_ |= image::kFlagImageRenderingChanged;
}

void UINewImage::UpdateTintColor(const lepus::Value& value) {
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

void UINewImage::UpdateEnableReportInfo(const lepus::Value& value) {
  enable_report_info_ = value.Bool();
}

void UINewImage::UpdateDropShadow(const lepus::Value& value) {
  dirty_flags_ |= image::kFlagEffectChanged;
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
      effect_flags_ |= image::kFlagEffectDropShadow;
      return;
    }
  }
  effect_flags_ &= ~image::kFlagEffectDropShadow;
}

LynxImageEffectProcessor::CommonViewParams
UINewImage::GenerateCommonViewParams() {
  return {width_,
          height_,
          padding_left_,
          padding_top_,
          padding_right_,
          padding_bottom_,
          context_->ScaledDensity()};
}

bool UINewImage::NeedMonitorInfo() {
  return enable_report_info_ || enable_image_load_callback_;
}
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
