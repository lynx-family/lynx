// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_FLATTEN_IMAGE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_FLATTEN_IMAGE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/image_shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_helper.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/image_drawable.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {

class UIFlattenImage : public UIBase,
                       public ImageDrawable::ImageAnimationListener {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag) {
    return new UIFlattenImage(context, sign, tag);
  }

  void OnPropUpdate(const std::string& name,
                    const lepus::Value& value) override;

  const std::string& GetSrc() { return src_; };

  const std::string& GetPlaceholder() { return place_holder_; };

 protected:
  UIFlattenImage(LynxContext* context, int sign, const std::string& tag);
  ~UIFlattenImage() override = default;
  void UpdateLayout(float left, float top, float width, float height,
                    const float* paddings, const float* margins,
                    const float* sticky, float max_height,
                    uint32_t node_index) override;
  void OnNodeEvent(ArkUI_NodeEvent* event) override;
  void SetEvents(const std::vector<lepus::Value>& events) override;
  void UpdateImageSource(const lepus::Value& value);
  void SetImageAttribute(const std::string& value, bool is_base64, bool is_src);
  void SetImageRendering(const lepus::Value& value) override;
  void OnNodeReady() override;
  void OnDraw(OH_Drawing_Canvas* canvas, ArkUI_NodeHandle node) override;
  void OnDrawBehind(OH_Drawing_Canvas* canvas, ArkUI_NodeHandle node) override;
  void InvokeMethod(const std::string& method, const lepus::Value& args,
                    base::MoveOnlyClosure<void, int32_t, const lepus::Value&>
                        callback) override;

  void onAnimationStart() override;
  void onAnimationRepeat() override;
  void onAnimationStop() override;

 private:
  std::string src_;
  std::string place_holder_;
  std::string mode_;
  std::unique_ptr<ImageDrawable> src_image_drawable_;
  std::unique_ptr<ImageDrawable> placeholder_image_drawable_;
  using UIMethod = void (UIFlattenImage::*)(
      const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  static std::unordered_map<std::string, UIMethod> image_ui_method_map_;
  bool auto_size_{false};
  bool auto_play_{true};
  int32_t loop_count_{0};
  bool skip_redirection_{false};
  float image_width_{0.f};
  float image_height_{0.f};
  float cap_inset_scale_{1.f};
  float shadow_offset_x_{0.f};
  float shadow_offset_y_{0.f};
  float shadow_radius_{0.f};
  float image_padding_left_{0.f};
  float image_padding_top_{0.f};
  float image_padding_right_{0.f};
  float image_padding_bottom_{0.f};
  uint32_t shadow_color_{0};
  uint32_t tint_color_{0};
  std::vector<float> cap_insets_;
  bool defer_src_invalidation_{false};
  uint64_t dirty_flags_{0};
  uint64_t event_flags_{0};
  static std::unordered_map<std::string,
                            void (UIFlattenImage::*)(const lepus::Value& value)>
      prop_setters_;
  LynxImageEffectProcessor::ImageEffect effect_type_{
      LynxImageEffectProcessor::ImageEffect::kNone};
  ImageDrawable::ImageMode ConvertMode(const std::string& mode);
  void UpdateImageMode(const lepus::Value& value);
  void UpdatePlaceholder(const lepus::Value& value);
  void UpdateAutoSize(const lepus::Value& value);
  void UpdateBlurRadius(const lepus::Value& value);
  void UpdateCapInsets(const lepus::Value& value);
  void UpdateCapInsetScale(const lepus::Value& value);
  void UpdateDeferSrcInvalidation(const lepus::Value& value);
  void UpdateTintColor(const lepus::Value& value);
  void UpdateDropShadow(const lepus::Value& value);
  void UpdateSkipRedirection(const lepus::Value& value);
  void SetImageModeAttribute(const std::string& value);
  void UpdateAutoPlay(const lepus::Value& value);
  void UpdateLoopCount(const lepus::Value& value);
  void StartAnimation(
      const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  void StopAnimation(
      const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  void PauseAnimation(
      const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  void ResumeAnimation(
      const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  void HandleImageSuccessCallback(float image_width, float image_height);
  void HandleImageFailCallback(float error_code, const std::string& error_msg);
  void AutoSizeIfNeeded();
  using ImageResourceHandler =
      void (UIFlattenImage::*)(pub::LynxPathResponse& response);
  void LoadImageResource(const std::string& url, ImageResourceHandler handler);
  void HandleImageSrcResponse(pub::LynxPathResponse& response);
  void HandleImagePlaceholderResponse(pub::LynxPathResponse& response);
  LynxImageEffectProcessor::CommonViewParams GenerateCommonViewParams();
  void HandleImageWithProcessor(
      const std::string& url, bool is_base64,
      LynxImageEffectProcessor::ImageEffect effect_type,
      const LynxImageEffectProcessor::EffectParams& params, bool is_src);
  void LoadImageFromURL(bool is_src);
  bool hasAnimationEvent();
  void Render(OH_Drawing_Canvas* canvas) const;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_FLATTEN_IMAGE_H_
