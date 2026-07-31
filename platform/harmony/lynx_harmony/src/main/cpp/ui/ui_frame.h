// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_FRAME_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_FRAME_H_

#include <arkui/native_node.h>
#include <node_api.h>

#include <memory>
#include <optional>
#include <string>

#include "base/include/value/base_value.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_view.h"

namespace lynx {
namespace tasm {
class LynxTemplateBundle;

namespace harmony {

class UIOwner;

class UIFrame : public UIView {
 public:
  static constexpr const char* kTag = "frame";

  static UIBase* Make(LynxContext* context, int sign, const std::string& tag) {
    return new UIFrame(context, sign, tag);
  }

  static napi_value OnHostReady(napi_env env, napi_callback_info info);
  static napi_value OnIntrinsicSizeChanged(napi_env env, napi_callback_info info);

  void SetFrameAppBundle(std::shared_ptr<tasm::LynxTemplateBundle> bundle);
  void UpdateProps(PropBundleHarmony* props) override;
  void UpdateLayout(float left, float top, float width, float height, const float* paddings,
                    const float* margins, const float* sticky, float max_height,
                    uint32_t node_index) override;
  void OnDestroy() override;

  void HandleHostReady();
  void HandleIntrinsicSizeChanged(float width, float height);
  void BuildLayoutChangeEventDetail(lepus::Dictionary& detail);

 private:
  struct FrameHostCreationResult {
    napi_ref host_ref{nullptr};
    ArkUI_NodeHandle node{nullptr};
  };

  UIFrame(LynxContext* context, int sign, const std::string& tag);
  ~UIFrame() override;

  FrameHostCreationResult CreateFrameHostFromCreator();
  void OnPropUpdate(const std::string& name, const lepus::Value& value) override;
  void CreateFrameHost();
  void DisposeFrameHost();
  void TryLoadBundle();
  void UpdateHostConfiguration();
  void UpdateHostViewport();
  void UpdateHostMetaDataIfNeeded();
  std::optional<float> ParsePresetLength(const lepus::Value& value) const;
  bool CallHostMethod(const char* method, size_t argc, napi_value* argv,
                      napi_value* result = nullptr);
  napi_value CreateNativeFrameValue();
  napi_value CreateMetaDataValue(bool has_value, const lepus::Value& value);
  void ResetLoadedState();
  static std::shared_ptr<UIFrame> GetFrameFromNapiThis(napi_env env, napi_value js_this);

  napi_env env_{nullptr};
  napi_ref frame_host_ref_{nullptr};
  ArkUI_NodeHandle frame_host_node_{nullptr};

  std::string src_;
  std::shared_ptr<tasm::LynxTemplateBundle> pending_bundle_;
  lepus::Value frame_data_;
  lepus::Value global_props_;
  bool has_frame_data_{false};
  bool has_global_props_{false};
  bool frame_data_dirty_{false};
  bool global_props_dirty_{false};
  bool auto_width_{false};
  bool auto_height_{false};
  std::optional<float> preset_width_;
  std::optional<float> preset_height_;
  bool enable_multi_async_thread_{true};
  bool props_updated_{false};
  bool child_context_ready_{false};
  bool loaded_{false};
  bool destroyed_{false};
  bool has_content_layout_{false};
  bool has_intrinsic_size_{false};
  float content_width_{0.f};
  float content_height_{0.f};
  float intrinsic_width_{0.f};
  float intrinsic_height_{0.f};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_FRAME_H_
