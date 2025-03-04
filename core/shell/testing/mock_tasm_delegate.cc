// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/testing/mock_tasm_delegate.h"

#include "base/include/debug/lynx_error.h"
#include "core/animation/constants.h"
#include "core/runtime/piper/js/runtime_constant.h"
#include "core/runtime/vm/lepus/json_parser.h"

namespace lynx {
namespace tasm {
namespace test {

void MockTasmDelegate::OnDataUpdated() { ss_ << "OnDataUpdated\n"; }
void MockTasmDelegate::OnTasmFinishByNative() {}
void MockTasmDelegate::OnTemplateLoaded(const std::string& url) {
  // URL is ignored.
  ss_ << "OnTemplateLoaded:"
      << "\n";
}
void MockTasmDelegate::OnSSRHydrateFinished(const std::string& url) {
  // URL is ignored.
  ss_ << "OnSSRHydrateFinished:"
      << "\n";
}
void MockTasmDelegate::OnErrorOccurred(base::LynxError error) {
  ss_ << "OnErrorOccurred:" << error.error_code_ << " " << error.error_message_
      << "\n";
}

void MockTasmDelegate::TriggerLepusngGc(base::closure func) {}

void MockTasmDelegate::OnDynamicComponentPerfReady(
    const lepus::Value& perf_info){};

void MockTasmDelegate::OnConfigUpdated(const lepus::Value& data) {
  ss_ << "OnConfigUpdated ";
  data.PrintValue(ss_);
  ss_ << std::endl;
  UpdateMockDelegateThemeConfig(data);
}
void MockTasmDelegate::OnPageConfigDecoded(
    const std::shared_ptr<tasm::PageConfig>& config) {
  ss_ << "OnPageConfigDecoded ";
  if (config) {
    config->PrintPageConfig(ss_);
  }
  ss_ << std::endl;
}

// synchronous
std::string MockTasmDelegate::TranslateResourceForTheme(
    const std::string& res_id, const std::string& theme_key) {
  if (!light_color_map_) {
    light_color_map_ =
        std::make_unique<std::unordered_map<std::string, std::string>>();
    light_color_map_->insert({"Color_brand_1", "#F04142"});
    light_color_map_->insert({"Color_grey_1", "#222222"});
    light_color_map_->insert({"Color_grey_2", "#505050"});
    light_color_map_->insert({"Color_grey_3", "#707070"});
    light_color_map_->insert({"Color_grey_4", "#999999"});
    light_color_map_->insert({"Color_grey_5", "#CACACA"});
    light_color_map_->insert({"Color_grey_6", "#D8D8D8"});
    light_color_map_->insert({"Color_grey_7", "#E8E8E8"});
    light_color_map_->insert({"Color_grey_8", "#F2F2F2"});
    light_color_map_->insert({"Color_grey_9", "#F8F8F8"});
    light_color_map_->insert({"Color_grey_10", "#FAFAFA"});
    light_color_map_->insert({"Color_white_1", "#FFFFFF"});
    light_color_map_->insert({"Color_bg_1", "#FFFFFF"});
    light_color_map_->insert({"Color_bg_2", "#FFFFFF"});
    light_color_map_->insert({"Color_bg_3", "#FFFFFF"});
    light_color_map_->insert({"Color_bg_4", "#FFFFFF"});
    light_color_map_->insert({"Color_bg_5", "#F2F2F2"});
    light_color_map_->insert({"Color_bg_6", "#F2F2F2"});
    light_color_map_->insert({"Color_black_1", "#000000"});
    light_color_map_->insert({"Color_black_2", "#000000"});
    light_color_map_->insert({"Color_red_0", "#FFF2F2"});
    light_color_map_->insert({"Color_red_2", "#FFABAB"});
    light_color_map_->insert({"Color_red_4", "#FF5E5E"});
    light_color_map_->insert({"Color_red_8", "#A82E2E"});
    light_color_map_->insert({"Color_peach_0", "#FFE8F3"});
    light_color_map_->insert({"Color_peach_4", "#FA4BA0"});
    light_color_map_->insert({"Color_peach_5", "#F2338F"});
    light_color_map_->insert({"Color_rose_0", "#FFEBFA"});
    light_color_map_->insert({"Color_rose_4", "#F54CCD"});
    light_color_map_->insert({"Color_rose_5", "#EB28BD"});
    light_color_map_->insert({"Color_purplish_red_0", "#FCEBFF"});
    light_color_map_->insert({"Color_purplish_red_4", "#E145FF"});
    light_color_map_->insert({"Color_purplish_red_5", "#DB24FF"});
    light_color_map_->insert({"Color_violet_0", "#F3E8FF"});
    light_color_map_->insert({"Color_violet_4", "#AE66FF"});
    light_color_map_->insert({"Color_violet_5", "#8F2BFF"});
    light_color_map_->insert({"Color_violet_4", "#AE66FF"});
    light_color_map_->insert({"Color_purple_0", "#F0E8FF"});
    light_color_map_->insert({"Color_purple_4", "#8A4DFF"});
    light_color_map_->insert({"Color_purple_5", "#742BFF"});
    light_color_map_->insert({"Color_blue_0", "#EAE8FF"});
    light_color_map_->insert({"Color_blue_4", "#6654FF"});
    light_color_map_->insert({"Color_blue_5", "#5A47FF"});
    light_color_map_->insert({"Color_ultramarine_0", "#E6F0FF"});
    light_color_map_->insert({"Color_ultramarine_4", "#3D89FF"});
    light_color_map_->insert({"Color_ultramarine_5", "#1A74FF"});
    light_color_map_->insert({"Color_ultramarine_9", "#0E408C"});
    light_color_map_->insert({"Color_acid_blue_0", "#EBF4FA"});
    light_color_map_->insert({"Color_acid_blue_4", "#3AA5F0"});
    light_color_map_->insert({"Color_acid_blue_5", "#0289E6"});
    light_color_map_->insert({"Color_aqua_green_0", "#E9F7F7"});
    light_color_map_->insert({"Color_aqua_green_4", "#39C4C4"});
    light_color_map_->insert({"Color_aqua_green_5", "#00ABAB"});
    light_color_map_->insert({"Color_forest_0", "#EDF7F2"});
    light_color_map_->insert({"Color_forest_4", "#3BBF7D"});
    light_color_map_->insert({"Color_forest_5", "#00AA54"});
    light_color_map_->insert({"Color_green_0", "#F1FAF0"});
    light_color_map_->insert({"Color_green_4", "#4EC244"});
    light_color_map_->insert({"Color_green_5", "#2DA822"});
    light_color_map_->insert({"Color_olive_0", "#F5FAED"});
    light_color_map_->insert({"Color_olive_4", "#8ECC29"});
    light_color_map_->insert({"Color_olive_5", "#70B500"});
    light_color_map_->insert({"Color_yellow_0", "#FCF9EB"});
    light_color_map_->insert({"Color_yellow_4", "#FCDC3F"});
    light_color_map_->insert({"Color_yellow_5", "#FCD307"});
    light_color_map_->insert({"Color_medium_yellow_0", "#FFF9EB"});
    light_color_map_->insert({"Color_medium_yellow_4", "#FFC740"});
    light_color_map_->insert({"Color_medium_yellow_5", "#FFBA12"});
    light_color_map_->insert({"Color_orange_0", "#FFF7F2"});
    light_color_map_->insert({"Color_orange_4", "#FF8E4F"});
    light_color_map_->insert({"Color_orange_5", "#FF7528"});
    light_color_map_->insert({"Color_golden_0", "#FFF5E9"});
    light_color_map_->insert({"Color_golden_4", "#C0833B"});
    light_color_map_->insert({"Color_golden_6", "#996D39"});
  }
  if (!dark_color_map_) {
    dark_color_map_ =
        std::make_unique<std::unordered_map<std::string, std::string>>();
    dark_color_map_->insert({"Color_acid_blue_0", "#001E33"});
    dark_color_map_->insert({"Color_acid_blue_4", "#3AA5F0"});
    dark_color_map_->insert({"Color_acid_blue_5", "#3AA5F0"});
    dark_color_map_->insert({"Color_aqua_green_0", "#003333"});
    dark_color_map_->insert({"Color_aqua_green_4", "#39C4C4"});
    dark_color_map_->insert({"Color_aqua_green_5", "#39C4C4"});
    dark_color_map_->insert({"Color_bg_1", "#000000"});
    dark_color_map_->insert({"Color_bg_2", "#121212"});
    dark_color_map_->insert({"Color_bg_3", "#282828"});
    dark_color_map_->insert({"Color_bg_4", "#383838"});
    dark_color_map_->insert({"Color_bg_5", "#000000"});
    dark_color_map_->insert({"Color_bg_6", "#383838"});
    dark_color_map_->insert({"Color_black_1", "#000000"});
    dark_color_map_->insert({"Color_black_2", "#474747"});
    dark_color_map_->insert({"Color_blue_0", "#120E33"});
    dark_color_map_->insert({"Color_blue_4", "#6654FF"});
    dark_color_map_->insert({"Color_blue_5", "#6654FF"});
    dark_color_map_->insert({"Color_brand_1", "#FF5E5E"});
    dark_color_map_->insert({"Color_forest_0", "#00331A"});
    dark_color_map_->insert({"Color_forest_4", "#3BBF7D"});
    dark_color_map_->insert({"Color_forest_5", "#3BBF7D"});
    dark_color_map_->insert({"Color_golden_0", "#33210D"});
    dark_color_map_->insert({"Color_golden_4", "#C79254"});
    dark_color_map_->insert({"Color_golden_6", "#C79254"});
    dark_color_map_->insert({"Color_green_0", "#0E330A"});
    dark_color_map_->insert({"Color_green_4", "#4EC244"});
    dark_color_map_->insert({"Color_green_5", "#4EC244"});
    dark_color_map_->insert({"Color_grey_1", "#C1C1C1"});
    dark_color_map_->insert({"Color_grey_10", "#171717"});
    dark_color_map_->insert({"Color_grey_2", "#999999"});
    dark_color_map_->insert({"Color_grey_3", "#8F8F8F"});
    dark_color_map_->insert({"Color_grey_4", "#787878"});
    dark_color_map_->insert({"Color_grey_5", "#474747"});
    dark_color_map_->insert({"Color_grey_6", "#383838"});
    dark_color_map_->insert({"Color_grey_7", "#282828"});
    dark_color_map_->insert({"Color_grey_8", "#1F1F1F"});
    dark_color_map_->insert({"Color_grey_9", "#1A1A1A"});
    dark_color_map_->insert({"Color_medium_yellow_0", "#332503"});
    dark_color_map_->insert({"Color_medium_yellow_4", "#FFC740"});
    dark_color_map_->insert({"Color_medium_yellow_5", "#FFC740"});
    dark_color_map_->insert({"Color_olive_0", "#203300"});
    dark_color_map_->insert({"Color_olive_4", "#8ECC29"});
    dark_color_map_->insert({"Color_olive_5", "#8ECC29"});
    dark_color_map_->insert({"Color_orange_0", "#331808"});
    dark_color_map_->insert({"Color_orange_4", "#FF8E4F"});
    dark_color_map_->insert({"Color_orange_5", "#FF8E4F"});
    dark_color_map_->insert({"Color_peach_0", "#330B1F"});
    dark_color_map_->insert({"Color_peach_4", "#FA4BA0"});
    dark_color_map_->insert({"Color_peach_5", "#FA4BA0"});
    dark_color_map_->insert({"Color_purple_0", "#170933"});
    dark_color_map_->insert({"Color_purple_4", "#8A4DFF"});
    dark_color_map_->insert({"Color_purple_5", "#8A4DFF"});
    dark_color_map_->insert({"Color_purplish_red_0", "#2C0933"});
    dark_color_map_->insert({"Color_purplish_red_4", "#E145FF"});
    dark_color_map_->insert({"Color_purplish_red_5", "#E145FF"});
    dark_color_map_->insert({"Color_red_0", "#331313"});
    dark_color_map_->insert({"Color_red_2", "#802A2A"});
    dark_color_map_->insert({"Color_red_4", "#FF5E5E"});
    dark_color_map_->insert({"Color_red_8", "#FFABAB"});
    dark_color_map_->insert({"Color_rose_0", "#330929"});
    dark_color_map_->insert({"Color_rose_4", "#F54CCD"});
    dark_color_map_->insert({"Color_rose_5", "#F54CCD"});
    dark_color_map_->insert({"Color_ultramarine_0", "#051733"});
    dark_color_map_->insert({"Color_ultramarine_4", "#3D89FF"});
    dark_color_map_->insert({"Color_ultramarine_5", "#3D89FF"});
    dark_color_map_->insert({"Color_ultramarine_9", "#1356BD"});
    dark_color_map_->insert({"Color_violet_0", "#1D0933"});
    dark_color_map_->insert({"Color_violet_4", "#AE66FF"});
    dark_color_map_->insert({"Color_violet_5", "#AE66FF"});
    dark_color_map_->insert({"Color_white_1", "#FFFFFF"});
    dark_color_map_->insert({"Color_yellow_0", "#332B02"});
    dark_color_map_->insert({"Color_yellow_4", "#FCDC3F"});
    dark_color_map_->insert({"Color_yellow_5", "#FCDC3F"});
  }
  if (theme_config_ && theme_config_->count("brightness") &&
      theme_config_->at("brightness") == "night") {
    if (dark_color_map_->count(res_id)) {
      return dark_color_map_->at(res_id);
    }
    return std::string("");
  }
  if (light_color_map_->count(res_id)) {
    return light_color_map_->at(res_id);
  }
  return std::string("");
}

void MockTasmDelegate::GetI18nResource(const std::string& channel,
                                       const std::string& fallback_url) {
  if (channel.empty()) {
    return;
  }
  std::string::size_type idx = channel.find("zh");
  if (idx != std::string::npos) {
    return;
  } else {
    idx = channel.find("en");
    if (idx != std::string::npos) {
      return;
    }
  }
  return;
}

void MockTasmDelegate::OnI18nResourceChanged(const std::string& res) {
  ss_ << "OnTemplateLoaded:" << res;
}

void MockTasmDelegate::UpdateMockDelegateThemeConfig(const lepus::Value& data) {
  if (!data.IsTable() || data.Table()->size() == 0) {
    return;
  }
  if (!theme_config_) {
    theme_config_ =
        std::make_unique<std::unordered_map<std::string, std::string>>();
  }
  for (const auto& prop : *(data.Table())) {
    if (!prop.first.IsEqual("theme") || !prop.second.IsTable()) {
      continue;
    }
    for (const auto& sub_prop : *(prop.second.Table())) {
      if (sub_prop.second.IsString()) {
        (*(theme_config_.get()))[sub_prop.first.str()] =
            sub_prop.second.StdString();
      }
    }
  }
}

void MockTasmDelegate::ResetThemeConfig() { theme_config_.reset(); }

void MockTasmDelegate::OnJSSourcePrepared(
    tasm::TasmRuntimeBundle bundle, const lepus::Value& global_props,
    const std::string& page_name, tasm::PackageInstanceDSL dsl,
    tasm::PackageInstanceBundleModuleMode bundle_module_mode,
    const std::string& url, const tasm::PipelineOptions& pipeline_options) {
  ss_ << "OnCardDecoded " << bundle.name << std::endl;
  ss_ << "OnJSSourcePrepared " << page_name << static_cast<int>(dsl) << " "
      << static_cast<int>(bundle_module_mode) << "\n";
}

void MockTasmDelegate::CallJSApiCallback(piper::ApiCallBack callback) {
  ss_ << "CallJSApiCallback " << callback.id();
}
void MockTasmDelegate::CallJSApiCallbackWithValue(piper::ApiCallBack callback,
                                                  const lepus::Value& value,
                                                  bool persist) {
  ss_ << "CallJSApiCallbackWithValue " << callback.id();
  value.PrintValue(ss_);
  ss_ << std::endl;
}
void MockTasmDelegate::RemoveJSApiCallback(piper::ApiCallBack callback) {
  ss_ << "RemoveJSApiCallback " << callback.id() << std::endl;
}

void MockTasmDelegate::CallPlatformCallbackWithValue(
    const std::shared_ptr<shell::PlatformCallBackHolder>& callback,
    const lepus::Value& value) {
  ss_ << "CallPlatformCallbackWithValue " << callback->id();
  value.PrintValue(ss_);
  ss_ << std::endl;
}

void MockTasmDelegate::RemovePlatformCallback(
    const std::shared_ptr<shell::PlatformCallBackHolder>& callback) {
  ss_ << "RemovePlatformCallback " << callback->id() << std::endl;
}

void MockTasmDelegate::CallJSFunction(const std::string& module_id,
                                      const std::string& method_id,
                                      const lepus::Value& arguments,
                                      bool force_call_despite_app_state) {
  ss_ << "CallJSFunction " << module_id << " " << method_id << " ";
  arguments.PrintValue(ss_);
  ss_ << std::endl;
}
void MockTasmDelegate::OnDataUpdatedByNative(tasm::TemplateData data,
                                             const bool reset) {
  ss_ << "OnDataUpdatedByNative: ";
  data.GetValue().PrintValue(ss_);
  ss_ << std::endl;
}
void MockTasmDelegate::OnJSAppReload(
    tasm::TemplateData init_data,
    const tasm::PipelineOptions& pipeline_options) {
  ss_ << "OnJSAppReload ";
  init_data.GetValue().PrintValue(ss_);
  ss_ << std::endl;
}
void MockTasmDelegate::OnLifecycleEvent(const lepus::Value& args) {
  ss_ << "OnLifecycleEvent ";
  args.PrintValue(ss_);
  ss_ << std::endl;
}

void MockTasmDelegate::PrintMsgToJS(const std::string& level,
                                    const std::string& msg) {
  ss_ << "PrintMsgToJS " << level << " " << msg << std::endl;
}

// timestamp & identifier will change when exec ut, so need to delete them when
// recording.
constexpr const static char* kTimeStamp = "timestamp";
constexpr const static char* kChangedTouches = "changedTouches";
constexpr const static char* kTouches = "touches";
constexpr const static char* kIdentifier = "identifier";
constexpr const static char* kUid = "uid";
constexpr const static char* kCurrentTarget = "currentTarget";
constexpr const static char* kTarget = "target";
void RemoveChangedItems(lepus::Value& info) {
  if (!info.IsTable()) {
    return;
  }
  const auto& table = info.Table();
  if (table->Contains(kTimeStamp)) {
    info.Table()->Erase(kTimeStamp);
  }
  if (table->Contains(kChangedTouches) &&
      table->GetValue(kChangedTouches).IsArray() &&
      table->GetValue(kChangedTouches).Array()->size() >= 1) {
    auto& ary = table->GetValue(kChangedTouches).Array()->get(0);
    if (ary.IsTable() && ary.Table()->Contains(kIdentifier)) {
      ary.Table()->Erase(kIdentifier);
    }
  }
  if (table->Contains(kTouches) && table->GetValue(kTouches).IsArray() &&
      table->GetValue(kTouches).Array()->size() >= 1) {
    auto& ary = table->GetValue(kTouches).Array()->get(0);
    if (ary.IsTable() && ary.Table()->Contains(kIdentifier)) {
      ary.Table()->Erase(kIdentifier);
    }
  }
  if (table->Contains(kTarget) && table->GetValue(kTarget).IsTable()) {
    auto obj = table->GetValue(kTarget).Table();
    if (obj->Contains(kUid)) {
      obj->Erase(kUid);
    }
  }
  if (table->Contains(kCurrentTarget) &&
      table->GetValue(kCurrentTarget).IsTable()) {
    auto obj = table->GetValue(kCurrentTarget).Table();
    if (obj->Contains(kUid)) {
      obj->Erase(kUid);
    }
  }
}

// LynxEngine::Delegate
void MockTasmDelegate::OnComponentDecoded(tasm::TasmRuntimeBundle bundle) {
  ss_ << "OnComponentDecoded " << bundle.name << std::endl;
}

void MockTasmDelegate::OnCardConfigDataChanged(const lepus::Value& data) {
  ss_ << "OnCardConfigDataChanged " << std::endl;
}

lepus::Value MockTasmDelegate::TriggerLepusMethod(
    const std::string& method_id, const lepus::Value& arguments) {
  lepus_method_id_ = method_id;
  lepus_method_arguments_ = arguments;
  ss_ << "TriggerLepusMethod" << std::endl;
  return lepus::Value();
}

void MockTasmDelegate::TriggerLepusMethodAsync(const std::string& method_id,
                                               const lepus::Value& arguments,
                                               bool is_air) {
  lepus_method_id_ = method_id;
  lepus_method_arguments_ = arguments;
  ss_ << "TriggerLepusMethodAsync" << std::endl;
}

void MockTasmDelegate::RequestVsync(
    uintptr_t id, base::MoveOnlyClosure<void, int64_t, int64_t> callback) {
  ss_ << "RequestVsync" << std::endl;
}

void MockTasmDelegate::SendAnimationEvent(const char* type, int tag,
                                          const lepus::Value& dict) {
  animation_event_type_ = type;
  if (std::strcmp(type, animation::kKeyframeStartEventName) == 0) {
    animation_start_event_count_++;
  } else if (std::strcmp(type, animation::kKeyframeEndEventName) == 0) {
    animation_end_event_count_++;
  } else if (std::strcmp(type, animation::kKeyframeCancelEventName) == 0) {
    animation_cancel_event_count_++;
  } else if (std::strcmp(type, animation::kKeyframeIterationEventName) == 0) {
    animation_iteration_event_count_++;
  }
  animation_event_params_ = dict;
}

void MockTasmDelegate::SendNativeCustomEvent(const std::string& name, int tag,
                                             const lepus::Value& param_value,
                                             const std::string& param_name) {
  ss_ << "SendNativeCustomEvent" << std::endl;
}

void MockTasmDelegate::ClearAnimationEvent() {
  animation_event_type_ = nullptr;
  animation_start_event_count_ = 0;
  animation_end_event_count_ = 0;
  animation_cancel_event_count_ = 0;
  animation_iteration_event_count_ = 0;
}

void MockTasmDelegate::RecycleTemplateBundle(
    std::unique_ptr<tasm::LynxBinaryRecyclerDelegate> recycler) {
  recycler->CompleteDecode();
  bundle_ = recycler->GetCompleteTemplateBundle();
}

event::DispatchEventResult MockTasmDelegate::DispatchMessageEvent(
    runtime::MessageEvent event) {
  if (event.type() == runtime::kMessageEventTypeOnNativeAppReady) {
    EXPECT_TRUE(event.message().IsNil());
    ss_ << "OnNativeAppReady\n";
  } else if (event.type() ==
             runtime::kMessageEventTypeNotifyGlobalPropsUpdated) {
    ss_ << "NotifyGlobalPropsUpdated " << std::endl;
  } else if (event.type() ==
             runtime::kMessageEventTypeOnDynamicJSSourcePrepared) {
    EXPECT_TRUE(event.message().IsString());
    ss_ << "OnDynamicJSSourcePrepared " << event.message().StdString()
        << std::endl;
  } else if (event.type() == runtime::kMessageEventTypeOnComponentActivity) {
    EXPECT_TRUE(event.message().IsArray());
    auto args = event.message().Array();
    EXPECT_EQ(6, args->size());
    EXPECT_TRUE(args->get(0).IsString());
    const auto& action = args->get(0).StdString();
    EXPECT_TRUE(args->get(1).IsString());
    EXPECT_TRUE(args->get(2).IsString());
    EXPECT_TRUE(args->get(3).IsString());
    const auto& path = args->get(3).StdString();
    EXPECT_TRUE(args->get(4).IsString());
    const auto& entry_name = args->get(4).StdString();
    // TODO: transfer component id to component name.
    ss_ << "OnComponentActivity " << action << " " << path << " " << entry_name
        << std::endl;
  } else if (event.type() == runtime::kMessageEventTypeOnReactComponentRender) {
    EXPECT_TRUE(event.message().IsArray());
    auto args = event.message().Array();
    EXPECT_EQ(4, args->size());
    EXPECT_TRUE(args->get(0).IsString());
    EXPECT_TRUE(args->get(3).IsBool());
    ss_ << "OnReactComponentRender " << args->get(0).StdString() << std::endl;
  } else if (event.type() ==
             runtime::kMessageEventTypeOnReactComponentDidUpdate) {
    EXPECT_TRUE(event.message().IsString());
    const auto& id = event.message().StdString();
    ss_ << "OnReactComponentDidUpdate " << id << std::endl;
  } else if (event.type() ==
             runtime::kMessageEventTypeOnReactComponentDidCatch) {
    EXPECT_TRUE(event.message().IsArray());
    auto args = event.message().Array();
    EXPECT_EQ(2, args->size());
    EXPECT_TRUE(args->get(0).IsString());
    const auto& id = args->get(0).StdString();
    auto error = args->get(1);
    ss_ << "OnReactComponentDidCatch " << id << " " << error << std::endl;
  } else if (event.type() ==
             runtime::kMessageEventTypeOnComponentSelectorChanged) {
    EXPECT_TRUE(event.message().IsArray());
    auto args = event.message().Array();
    EXPECT_EQ(2, args->size());
    EXPECT_TRUE(args->get(0).IsString());
    ss_ << "OnComponentSelectorChanged " << std::endl;
  } else if (event.type() == runtime::KMessageEventTypeOnReactCardDidUpdate) {
    ss_ << "OnReactCardDidUpdate" << std::endl;
  } else if (event.type() ==
             runtime::kMessageEventTypeOnReactComponentCreated) {
    EXPECT_TRUE(event.message().IsArray());
    auto args = event.message().Array();
    EXPECT_EQ(7, args->size());
    EXPECT_TRUE(args->get(0).IsString());
    const auto& entry_name = args->get(0).StdString();
    EXPECT_TRUE(args->get(1).IsString());
    const auto& path = args->get(1).StdString();
    EXPECT_TRUE(args->get(2).IsString());
    const auto& id = args->get(2).StdString();
    EXPECT_TRUE(args->get(5).IsString());
    const auto& parent_id = args->get(5).StdString();
    EXPECT_TRUE(args->get(6).IsBool());
    ss_ << "OnReactComponentCreated " << entry_name << " " << path << " " << id
        << " " << parent_id << std::endl;
  } else if (event.type() ==
             runtime::kMessageEventTypeOnComponentPropertiesChanged) {
    EXPECT_TRUE(event.message().IsArray());
    auto args = event.message().Array();
    EXPECT_EQ(2, args->size());
    EXPECT_TRUE(args->get(0).IsString());
    ss_ << "OnComponentPropertiesChanged " << std::endl;
  } else if (event.type() ==
             runtime::kMessageEventTypeOnComponentDataSetChanged) {
    EXPECT_TRUE(event.message().IsArray());
    auto args = event.message().Array();
    EXPECT_EQ(2, args->size());
    EXPECT_TRUE(args->get(0).IsString());
    ss_ << "OnComponentDataSetChanged " << std::endl;
  } else if (event.type() == runtime::kMessageEventTypeOnReactCardRender) {
    EXPECT_TRUE(event.message().IsArray());
    auto args = event.message().Array();
    EXPECT_EQ(3, args->size());
    EXPECT_TRUE(args->get(1).IsBool());
    EXPECT_TRUE(args->get(2).IsBool());
    ss_ << "OnReactCardRender" << std::endl;
  } else if (event.type() ==
             runtime::kMessageEventTypeOnReactComponentUnmount) {
    EXPECT_TRUE(event.message().IsString());
    const auto& id = event.message().StdString();
    ss_ << "OnReactComponentUnmount " << id << std::endl;
  } else if (event.type() == runtime::kMessageEventTypeSendPageEvent) {
    EXPECT_TRUE(event.message().IsArray());
    auto args = event.message().Array();
    EXPECT_EQ(3, args->size());
    EXPECT_TRUE(args->get(0).IsString());
    EXPECT_TRUE(args->get(1).IsString());
    const auto& page_name = args->get(0).StdString();
    const auto& handler = args->get(1).StdString();
    auto info = lepus_value::Clone(args->get(2));
    ss_ << "SendPageEvent " << page_name << " " << handler << " ";
    RemoveChangedItems(const_cast<lepus::Value&>(info));
    lepusValueToJSONString(ss_, info, true);
    ss_ << std::endl;
  } else if (event.type() == runtime::kMessageEventTypePublishComponentEvent) {
    EXPECT_TRUE(event.message().IsArray());
    auto args = event.message().Array();
    EXPECT_EQ(3, args->size());
    EXPECT_TRUE(args->get(0).IsString());
    EXPECT_TRUE(args->get(1).IsString());
    const auto& component_id = args->get(0).StdString();
    const auto& handler = args->get(1).StdString();
    auto info = lepus_value::Clone(args->get(2));
    ss_ << "PublishComponentEvent " << component_id << " " << handler << " ";
    RemoveChangedItems(const_cast<lepus::Value&>(info));
    lepusValueToJSONString(ss_, info, true);
    ss_ << std::endl;
  } else if (event.type() == runtime::kMessageEventTypeSendGlobalEvent) {
    EXPECT_TRUE(event.message().IsArray());
    auto args = event.message().Array();
    EXPECT_EQ(2, args->size());
    EXPECT_TRUE(args->get(0).IsString());
    const auto& name = args->get(0).StdString();
    auto info = lepus_value::Clone(args->get(1));
    ss_ << "SendGlobalEvent " << name << " ";
    RemoveChangedItems(const_cast<lepus::Value&>(info));
    lepusValueToJSONString(ss_, info, true);
    ss_ << std::endl;
  } else if (event.type() ==
             runtime::kMessageEventTypeCallJSFunctionInLepusEvent) {
    EXPECT_TRUE(event.message().IsArray());
    auto args = event.message().Array();
    EXPECT_EQ(3, args->size());
    EXPECT_TRUE(args->get(0).IsString());
    EXPECT_TRUE(args->get(1).IsString());
    const auto& component_id = args->get(0).StdString();
    const auto& name = args->get(1).StdString();
    auto params = lepus_value::Clone(args->get(2));
    ss_ << "CallJSFunctionInLepusEvent " << component_id << " " << name << " ";
    RemoveChangedItems(const_cast<lepus::Value&>(params));
    lepusValueToJSONString(ss_, params, true);
    ss_ << std::endl;
  }
  return event::DispatchEventResult::kNotCanceled;
}

void MockTasmDelegate::OnGlobalPropsUpdated(const lepus::Value& props) {
  ss_ << "NotifyGlobalPropsUpdated " << std::endl;
}

}  // namespace test

}  // namespace tasm

}  // namespace lynx
