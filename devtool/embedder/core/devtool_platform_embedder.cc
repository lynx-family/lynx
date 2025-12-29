// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/core/devtool_platform_embedder.h"

#include <memory>
#include <vector>

#include "core/public/devtool/lynx_devtool_proxy.h"
#include "devtool/base_devtool/native/public/devtool_status.h"
#include "devtool/embedder/core/env_embedder.h"
#include "devtool/embedder/core/inspector_owner_embedder.h"
#include "devtool/embedder/core/page_reload_helper_embedder.h"
#include "devtool/embedder/core/screen_cast_helper_embedder.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/lynx_devtool/base/screen_metadata.h"
#include "devtool/lynx_devtool/element/element_inspector.h"
#include "devtool/lynx_devtool/js_debug/js/inspector_java_script_debugger_impl.h"

namespace lynx {

namespace devtool {

class DevtoolPlatformImpl : public lynx::devtool::DevToolPlatformFacade {
 public:
  explicit DevtoolPlatformImpl(
      const std::shared_ptr<DevtoolPlatformEmbedder>& embedder) {
    weak_embedder_ = embedder;
  }

  int FindNodeIdForLocation(float x, float y,
                            std::string screen_shot_mode) override {
    auto embedder = weak_embedder_.lock();
    CHECK_NULL_AND_LOG_RETURN_VALUE(embedder, "embedder is null", 0);
    return embedder->FindNodeIdForLocation(x, y);
  }

  std::string GetDebugInfoByUrl(const std::string& url) override {
    return DevToolStatus::NO_DEBUG_INFO_FOUND_BY_URL;
  }

  void ScrollIntoView(int node_id) override {}

  void StartScreenCast(lynx::devtool::ScreenshotRequest request) override {
    auto embedder = weak_embedder_.lock();
    CHECK_NULL_AND_LOG_RETURN(embedder, "embedder is null");
    embedder->StartCasting(request.quality_, request.max_width_,
                           request.max_height_);
  }

  void StopScreenCast() override {
    auto embedder = weak_embedder_.lock();
    CHECK_NULL_AND_LOG_RETURN(embedder, "embedder is null");
    embedder->StopCasting();
  }

  void OnAckReceived() override {}

  void GetLynxScreenShot() override {
    auto embedder = weak_embedder_.lock();
    CHECK_NULL_AND_LOG_RETURN(embedder, "embedder is null");
    embedder->GetLynxScreenShot();
  }

  void SetDevToolSwitch(const std::string& key, bool value) override {
    EnvEmbedder::SetSwitch(key, value);
  }

  void EmulateTouch(std::shared_ptr<lynx::devtool::MouseEvent> input) override {
    auto embedder = weak_embedder_.lock();
    CHECK_NULL_AND_LOG_RETURN(embedder, "embedder is null");
    embedder->EmulateTouch(input);
  }

  std::vector<float> GetRectToWindow() const override { return {}; }

  std::string GetLynxVersion() const override { return ""; }

  void OnReceiveTemplateFragment(const std::string& data, bool eof) override {}
  std::vector<int32_t> GetViewLocationOnScreen() const override { return {}; }

  void SendEventToVM(const std::string& vm_type, const std::string& event_name,
                     const std::string& data) override {}

  lynx::lepus::Value* GetLepusValueFromTemplateData() override {
    // TODO(yaoyuchi): add implement
    return nullptr;
  }

  std::string GetTemplateJsInfo(int32_t offset, int32_t size) override {
    return "";
  }

  std::vector<double> GetBoxModel(Element* element) override {
    return GetBoxModelInGeneralPlatform(element);
  }

  std::vector<float> GetTransformValue(
      int identifier,
      const std::vector<float>& pad_border_margin_layout) override {
    auto embedder = weak_embedder_.lock();
    CHECK_NULL_AND_LOG_RETURN_VALUE(embedder, "embedder is null", {});
    return embedder->GetTransformValue(identifier, pad_border_margin_layout);
  }

  void PageReload(bool ignore_cache, const std::string& template_binary,
                  const std::string& reload_url,
                  bool from_template_fragments = false,
                  int32_t template_size = 0) override {
    auto embedder = weak_embedder_.lock();
    CHECK_NULL_AND_LOG_RETURN(embedder, "embedder is null");
    embedder->Reload(ignore_cache, template_binary, reload_url,
                     from_template_fragments, template_size);
  }

  void Navigate(const std::string& url) override {
    auto embedder = weak_embedder_.lock();
    CHECK_NULL_AND_LOG_RETURN(embedder, "embedder is null");
    embedder->Navigate(url);
  }

  void OnConsoleMessage(const std::string& message) override {
    auto embedder = weak_embedder_.lock();
    CHECK_NULL_AND_LOG_RETURN(embedder, "embedder is null");
    embedder->OnConsoleMessage(message);
  }

  void OnConsoleObject(const std::string& detail, int callback_id) override {
    auto embedder = weak_embedder_.lock();
    CHECK_NULL_AND_LOG_RETURN(embedder, "embedder is null");
    embedder->OnConsoleObject(detail, callback_id);
  }

  std::string GetLepusDebugInfo(const std::string& url) override {
    auto embedder = weak_embedder_.lock();
    CHECK_NULL_AND_LOG_RETURN_VALUE(embedder, "embedder is null", "");
    std::string debug_info;
    embedder->GetLepusDebugInfo(url, debug_info);
    return debug_info;
  }

 private:
  std::weak_ptr<DevtoolPlatformEmbedder> weak_embedder_;
};

DevtoolPlatformEmbedder::DevtoolPlatformEmbedder() : reload_helper_(nullptr) {}

void DevtoolPlatformEmbedder::Init(
    devtool::LynxDevToolProxy* proxy,
    const std::shared_ptr<InspectorOwnerEmbedder>& owner) {
  devtool_platform_facade_ =
      std::make_shared<DevtoolPlatformImpl>(shared_from_this());
  weak_owner_ = owner;
  cast_helper_ =
      std::make_unique<ScreenCastHelperEmbedder>(proxy, shared_from_this());
  reload_helper_ = std::make_unique<PageReloadHelperEmbedder>(proxy);
  proxy_ = proxy;
  debug_info_helper_ = std::make_unique<DebugInfoHelper>();
}

DevtoolPlatformEmbedder::~DevtoolPlatformEmbedder() {}

int DevtoolPlatformEmbedder::FindNodeIdForLocation(float x, float y) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(proxy_, "proxy_ is null", 0);
  return proxy_->GetNodeForLocation(x, y);
}

void DevtoolPlatformEmbedder::StartCasting(int32_t quality, int32_t max_width,
                                           int32_t max_height) {
  CHECK_NULL_AND_LOG_RETURN(cast_helper_, "cast_helper_ is null");
  cast_helper_->StartCasting(quality, max_width, max_height);
}

void DevtoolPlatformEmbedder::StopCasting() {
  CHECK_NULL_AND_LOG_RETURN(cast_helper_, "cast_helper_ is null");
  cast_helper_->StopCasting();
}

void DevtoolPlatformEmbedder::ContinueCasting() {
  CHECK_NULL_AND_LOG_RETURN(cast_helper_, "cast_helper_ is null");
  cast_helper_->ContinueCasting();
}

void DevtoolPlatformEmbedder::PauseCasting() {
  CHECK_NULL_AND_LOG_RETURN(cast_helper_, "cast_helper_ is null");
  cast_helper_->PauseCasting();
}

void DevtoolPlatformEmbedder::GetLynxScreenShot() {
  CHECK_NULL_AND_LOG_RETURN(cast_helper_, "cast_helper_ is null");
  cast_helper_->GetLynxScreenShot();
}

void DevtoolPlatformEmbedder::OnLoadTemplate(
    const std::string& url, const std::vector<uint8_t>& tem,
    const std::shared_ptr<tasm::TemplateData>& init_data) {
  CHECK_NULL_AND_LOG_RETURN(reload_helper_, "reload_helper_ is null");
  reload_helper_->OnLoadTemplate(url, tem, init_data);
}

void DevtoolPlatformEmbedder::Reload(bool ignore_cache,
                                     const std::string& template_binary,
                                     const std::string& reload_url,
                                     bool from_template_fragments,
                                     int32_t template_size) {
  CHECK_NULL_AND_LOG_RETURN(reload_helper_, "reload_helper_ is null");
  reload_helper_->Reload(ignore_cache, template_binary, reload_url,
                         from_template_fragments, template_size);
}

void DevtoolPlatformEmbedder::Navigate(const std::string& url) {
  CHECK_NULL_AND_LOG_RETURN(reload_helper_, "reload_helper_ is null");
  reload_helper_->Navigate(url);
}

std::string DevtoolPlatformEmbedder::GetTemplateUrl() {
  return reload_helper_ ? reload_helper_->GetURL() : "___UNKNOWN___";
}

std::shared_ptr<tasm::TemplateData> DevtoolPlatformEmbedder::getTemplateDate() {
  return reload_helper_ ? reload_helper_->GetTemplateData() : nullptr;
}

void DevtoolPlatformEmbedder::SendScreenCast(
    const std::string& data, const devtool::ScreenMetadata& metadata) {
  if (devtool_platform_facade_) {
    std::shared_ptr<lynx::devtool::ScreenMetadata> devtool_metadata =
        std::make_shared<lynx::devtool::ScreenMetadata>();
    devtool_metadata->offset_top_ = metadata.offset_top_;
    devtool_metadata->page_scale_factor_ = metadata.page_scale_factor_;
    devtool_metadata->device_width_ = metadata.device_width_;
    devtool_metadata->device_height_ = metadata.device_height_;
    devtool_metadata->scroll_off_set_x_ = metadata.scroll_off_set_x_;
    devtool_metadata->scroll_off_set_y_ = metadata.scroll_off_set_y_;
    devtool_metadata->timestamp_ = metadata.timestamp_;
    devtool_platform_facade_->SendPageScreencastFrameEvent(data,
                                                           devtool_metadata);
  }
}

void DevtoolPlatformEmbedder::SendScreenCapture(const std::string& data) {
  if (devtool_platform_facade_) {
    devtool_platform_facade_->SendLynxScreenshotCapturedEvent(data);
  }
}

void DevtoolPlatformEmbedder::SendConsoleEvent(const std::string& message,
                                               int32_t level,
                                               int64_t time_stamp) {
  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  devtool_platform_facade_->SendConsoleEvent({message, level, time_stamp});
}

void DevtoolPlatformEmbedder::DispatchScreencastVisibilityChanged(bool status) {
  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  devtool_platform_facade_->SendPageScreencastVisibilityChangedEvent(status);
}

std::vector<float> DevtoolPlatformEmbedder::GetTransformValue(
    int id, const std::vector<float>& pad_border_margin_layout) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(proxy_, "proxy_ is null", {});
  return proxy_->GetTransformValue(id, pad_border_margin_layout);
}

void DevtoolPlatformEmbedder::EmulateTouch(
    std::shared_ptr<lynx::devtool::MouseEvent> input) {
  CHECK_NULL_AND_LOG_RETURN(proxy_, "proxy_ is null");
  proxy_->EmulateTouch(input->type_, input->x_, input->y_, input->button_,
                       input->delta_x_, input->delta_y_, input->modifiers_,
                       input->click_count_);
}

void DevtoolPlatformEmbedder::FlushConsoleMessages() {
  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  auto debugger = devtool_platform_facade_->GetJSDebugger().lock();
  CHECK_NULL_AND_LOG_RETURN(debugger, "debugger is null");
  debugger->FlushConsoleMessages();
}

void DevtoolPlatformEmbedder::GetConsoleObject(const std::string& object_id,
                                               bool need_stringify,
                                               int callback_id) {
  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  auto debugger = devtool_platform_facade_->GetJSDebugger().lock();
  CHECK_NULL_AND_LOG_RETURN(debugger, "debugger is null");
  debugger->GetConsoleObject(object_id, need_stringify, callback_id);
}

void DevtoolPlatformEmbedder::OnConsoleMessage(const std::string& message) {
  auto sp = weak_owner_.lock();
  if (sp) {
    sp->OnConsoleMessage(message);
  }
}

void DevtoolPlatformEmbedder::OnConsoleObject(const std::string& detail,
                                              int callback_id) {
  auto sp = weak_owner_.lock();
  if (sp) {
    sp->OnConsoleObject(detail, callback_id);
  }
}

void DevtoolPlatformEmbedder::GetLepusDebugInfo(const std::string& url,
                                                std::string& debug_info) {
  CHECK_NULL_AND_LOG_RETURN(debug_info_helper_, "debug_info_helper_ is null");
  debug_info_helper_->GetLepusDebugInfo(url, debug_info);
}

}  // namespace devtool
}  // namespace lynx
