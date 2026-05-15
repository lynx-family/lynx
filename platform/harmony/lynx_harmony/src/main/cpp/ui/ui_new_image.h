// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_NEW_IMAGE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_NEW_IMAGE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "platform/harmony/lynx_harmony/src/main/cpp/public/image_service.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_helper.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {

class UINewImage : public UIBase,
                   public ImageLoadListener,
                   public AnimationListener {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag) {
    return new UINewImage(context, sign, tag);
  }

  void OnPropUpdate(const std::string& name,
                    const lepus::Value& value) override;

  void OnDrawBehind(OH_Drawing_Canvas* canvas, ArkUI_NodeHandle node) override;

  void OnImageLoadSuccess(float image_width, float image_height) override;
  void OnImageMonitorInfo(const ImageMonitorInfo& data) override;
  void OnImageLoadFailure(int error_code,
                          const std::string& error_msg) override;
  void OnAnimationStart() override;
  void OnAnimationRepeat() override;
  void OnAnimationFinish() override;
  bool NeedMonitorInfo() override;

 protected:
  UINewImage(LynxContext* context, int sign, const std::string& tag);
  ~UINewImage() override;
  void UpdateLayout(float left, float top, float width, float height,
                    const float* paddings, const float* margins,
                    const float* sticky, float max_height,
                    uint32_t node_index) override;
  void SetEvents(const std::vector<lepus::Value>& events) override;
  void SetImageRendering(const lepus::Value& value) override;
  void OnNodeReady() override;
  bool HasContent() override { return true; }
  void InvokeMethod(const std::string& method, const lepus::Value& args,
                    base::MoveOnlyClosure<void, int32_t, const lepus::Value&>
                        callback) override;

 private:
  std::string src_;
  std::string redirected_src_;
  std::string placeholder_;
  std::string mode_;
  bool auto_size_{false};
  bool downsampling_{false};
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
  float image_view_width_{0.f};
  float image_view_height_{0.f};
  uint32_t shadow_color_{0};
  uint32_t tint_color_{0};
  OH_Drawing_ColorFilter* color_filter_{nullptr};
  std::vector<float> cap_insets_;
  bool defer_src_invalidation_{false};
  uint32_t dirty_flags_{0};
  uint32_t event_flags_{0};
  uint8_t effect_flags_{0};
  bool init_listener_{false};
  bool autoplay_{true};
  int32_t loop_count_{0};
  std::unique_ptr<ImageNode> image_node_;
  bool has_src_{false};
  bool enable_image_load_callback_{false};  // for native side
  bool enable_report_info_{false};          // for frontend
  bool enable_redirect_url_{false};

  static std::unordered_map<std::string,
                            void (UINewImage::*)(const lepus::Value& value)>
      prop_setters_;
  ArkUI_ObjectFit ConvertMode(const std::string& mode);
  void UpdateImageSource(const lepus::Value& value);
  void UpdateAutoPlay(const lepus::Value& value);
  void UpdateLoopCount(const lepus::Value& value);
  void UpdateImageMode(const lepus::Value& value);
  void UpdatePlaceholder(const lepus::Value& value);
  void UpdateAutoSize(const lepus::Value& value);
  void UpdateBlurRadius(const lepus::Value& value);
  void UpdateCapInsets(const lepus::Value& value);
  void UpdateCapInsetScale(const lepus::Value& value);
  void UpdateDeferSrcInvalidation(const lepus::Value& value);
  void UpdateTintColor(const lepus::Value& value);
  void UpdateDropShadow(const lepus::Value& value);
  void UpdateDownsampling(const lepus::Value& value);
  void UpdateEnableReportInfo(const lepus::Value& value);
  void AutoSizeIfNeeded();
  void LoadImage();
  void LoadImageFromService(const std::string& url,
                            const std::string& placeholder,
                            std::vector<std::string> fallback_urls = {});
  void LoadImageWithTransform(const std::string& url,
                              const std::string& placeholder);

  LynxImageEffectProcessor::CommonViewParams GenerateCommonViewParams();
  void InitAnimationListener();
  using UIMethod = void (UINewImage::*)(
      const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  static std::unordered_map<std::string, UIMethod> image_ui_method_map_;

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
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_NEW_IMAGE_H_
