// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_ANDROID_DEVTOOL_PLATFORM_ANDROID_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_ANDROID_DEVTOOL_PLATFORM_ANDROID_H_

#include <jni.h>

#include <memory>
#include <mutex>

#include "base/include/platform/android/scoped_java_ref.h"
#include "devtool/lynx_devtool/agent/devtool_platform_facade.h"

namespace lynx {
namespace devtool {
class DevToolPlatformAndroid : public DevToolPlatformFacade {
 public:
  static bool RegisterJNIUtils(JNIEnv* env);
  DevToolPlatformAndroid(JNIEnv* env, jobject owner);

  void ScrollIntoView(int node_id) override;
  int FindNodeIdForLocation(float x, float y,
                            std::string screen_shot_mode) override;
  void StartScreenCast(ScreenshotRequest request) override;
  void OnAckReceived() override;
  void StopScreenCast() override;
  void GetLynxScreenShot() override;
  std::vector<double> GetBoxModel(tasm::Element* element) override;
  std::vector<float> GetTransformValue(
      int id, const std::vector<float>& pad_border_margin_layout) override;

  lynx::lepus::Value* GetLepusValueFromTemplateData() override;
  std::string GetTemplateJsInfo(int32_t offset, int32_t size) override;

  std::string GetUINodeInfo(int id) override;
  std::string GetLynxUITree() override;
  int SetUIStyle(int id, std::string name, std::string content) override;

  void EmulateTouch(std::shared_ptr<lynx::devtool::MouseEvent> input) override;

  std::vector<float> GetRectToWindow() const override;
  std::string GetLynxVersion() const override;
  void OnReceiveTemplateFragment(const std::string& data, bool eof) override;
  void SetDevToolSwitch(const std::string& key, bool value) override;
  std::vector<int32_t> GetViewLocationOnScreen() const override;
  void SendEventToVM(const std::string& vm_type, const std::string& event_name,
                     const std::string& data) override;
  void OnConsoleMessage(const std::string& message) override;
  void OnConsoleObject(const std::string& detail, int callback_id) override;
  std::string GetLepusDebugInfo(const std::string& url) override;
  void SetLepusDebugInfoUrl(const std::string& url) override;

  void Destroy();

  void PageReload(bool ignore_cache, std::string template_bin = "",
                  bool from_template_fragments = false,
                  int32_t template_size = 0) override;
  void Navigate(const std::string& url) override;

 private:
  std::mutex mutex_;
  lynx::base::android::ScopedWeakGlobalJavaRef<jobject> weak_android_delegate_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_ANDROID_DEVTOOL_PLATFORM_ANDROID_H_
