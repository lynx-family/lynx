// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_config_decoder.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "core/public/prop_bundle.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/js/runtime_constant.h"
#include "core/runtime/lepus/json_parser.h"
#include "core/services/event_report/event_tracker.h"
#include "core/template_bundle/template_codec/binary_decoder/binary_decoder_trace_event_def.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_config_constant_auto_gen.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_config_decoder.h"

namespace lynx {
namespace tasm {
/// Upload global feature switches in PageConfig with common data about lynx
/// view. If you add a new  global feature switch, you should add it to report
/// event.
static constexpr const char* kLynxSDKGlobalFeatureSwitchEvent =
    "lynxsdk_global_feature_switch_statistic";

bool LynxBinaryConfigDecoder::DecodePageConfig(
    std::string config_str, std::shared_ptr<PageConfig>& page_config) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              BINARY_BASE_TEMPLATE_READER_DECODE_PAGE_CONFIG,
              [&config_str](lynx::perfetto::EventContext ctx) {
                auto* debug = ctx.event()->add_debug_annotations();
                debug->set_name("pageConfig");
                debug->set_legacy_json_value(config_str);
              });
  rapidjson::Document doc;
  if (doc.Parse(config_str.c_str()).HasParseError()) {
    LOGE("DecodePageConfig Error!");
    return false;
  }

  // Most of the read/write and decode code for page_config has been
  // automatically generated through IDL files. Here, the automatically
  // generated page_config decode method is called first.
  LynxConfigDecoder::DecodePageConfig(page_config, doc, target_sdk_version_);

  if (doc.HasMember(TEMPLATE_BUNDLE_MODULE_MODE) &&
      doc[TEMPLATE_BUNDLE_MODULE_MODE].IsInt()) {
    int bundleModuleModeInt = doc[TEMPLATE_BUNDLE_MODULE_MODE].GetInt();
    PackageInstanceBundleModuleMode bundleModuleMode =
        static_cast<PackageInstanceBundleModuleMode>(bundleModuleModeInt);
    if (bundleModuleMode ==
        PackageInstanceBundleModuleMode::RETURN_BY_FUNCTION_MODE) {
      page_config->SetBundleModuleMode(
          PackageInstanceBundleModuleMode::RETURN_BY_FUNCTION_MODE);
    } else {
      page_config->SetBundleModuleMode(
          PackageInstanceBundleModuleMode::EVAL_REQUIRE_MODE);
    }
  } else {
    page_config->SetBundleModuleMode(
        PackageInstanceBundleModuleMode::EVAL_REQUIRE_MODE);
  }

  if (doc.HasMember(TEMPLATE_BUNDLE_APP_DSL) &&
      doc[TEMPLATE_BUNDLE_APP_DSL].IsInt()) {
    page_config->SetDSL(
        static_cast<PackageInstanceDSL>(doc[TEMPLATE_BUNDLE_APP_DSL].GetInt()));
  }

  if (doc.HasMember(config::kQuirksMode) && doc[config::kQuirksMode].IsBool()) {
    if (!doc[config::kQuirksMode].GetBool()) {
      page_config.get()->SetQuirksModeByVersion(kQuirksModeDisableVersion);
    }
  } else if (doc.HasMember(config::kQuirksMode) &&
             doc[config::kQuirksMode].IsString()) {
    page_config.get()->SetQuirksModeByVersion(
        base::Version(doc[config::kQuirksMode].GetString()));
  } else if ((lynx::tasm::Config::IsHigherOrEqual(target_sdk_version_,
                                                  kQuirksModeDisableVersion))) {
    page_config.get()->SetQuirksModeByVersion(
        base::Version(target_sdk_version_));
  }

  if (doc.HasMember(config::kCustomCSSInheritanceList) &&
      doc[config::kCustomCSSInheritanceList].IsArray()) {
    std::unordered_set<CSSPropertyID> inherit_list;
    const auto& names_array = doc[config::kCustomCSSInheritanceList].GetArray();
    inherit_list.reserve(names_array.Size());
    for (const auto& entry : names_array) {
      if (entry.IsString()) {
        inherit_list.insert(
            tasm::CSSProperty::GetPropertyID(entry.GetString()));
      }
    }
    page_config->SetCustomCSSInheritList(std::move(inherit_list));
  }

  if (lynx::tasm::Config::IsHigherOrEqual(target_sdk_version_,
                                          LYNX_VERSION_2_0) &&
      compile_options_.default_overflow_visible_) {
    page_config->SetDefaultOverflowVisible(true);
  }

  if (lynx::tasm::Config::IsHigherOrEqual(target_sdk_version_,
                                          LYNX_VERSION_2_2) &&
      compile_options_.default_display_linear_) {
    page_config->SetDefaultDisplayLinear(true);
  }

  if (lynx::tasm::Config::IsHigherOrEqual(target_sdk_version_,
                                          LYNX_VERSION_2_3)) {
    page_config->SetEnableZIndex(true);
  }

  if (doc.HasMember(config::kEnableMultiTouchParamsCompatible) &&
      doc[config::kEnableMultiTouchParamsCompatible].IsBool()) {
    page_config->SetEnableMultiTouchParamsCompatible(
        doc[config::kEnableMultiTouchParamsCompatible].GetBool());
  } else {
    page_config->SetEnableMultiTouchParamsCompatible(
        LynxEnv::GetInstance().EnableMultiTouch());
  }

  page_config->SetTargetSDKVersion(target_sdk_version_);
  page_config->SetEnableLepusNG(is_lepusng_binary_);

  // engineVersion > 2.1 && enableKeepPageData ON.
  page_config->SetEnableSavePageData(
      Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                              FEATURE_NEW_RENDER_PAGE) &&
      compile_options_.enable_keep_page_data);

  page_config->SetEnableLynxAir(compile_options_.enable_lynx_air_);
  page_config->SetEnableFiberArch(compile_options_.enable_fiber_arch_);
  page_config->SetEnableCSSParser(enable_css_parser_);
  page_config->SetAbSettingDisableCSSLazyDecode(
      absetting_disable_css_lazy_decode_);

  // if enable_event_refactor == enabled, set page config's
  // enable_event_refactor_ as true.
  page_config->SetEnableEventRefactor(
      compile_options_.enable_event_refactor_ == FE_OPTION_ENABLE ||
      compile_options_.enable_event_refactor_ == FE_OPTION_UNDEFINED);

  page_config->SetEnableCSSInvalidation(
      compile_options_.enable_css_invalidation_);

  page_config->SetLynxAirMode(
      CompileOptionAirMode(compile_options_.lynx_air_mode_));

  if (compile_options_.force_calc_new_style_ != FE_OPTION_UNDEFINED) {
    page_config->SetForceCalcNewStyle(compile_options_.force_calc_new_style_ !=
                                      FE_OPTION_DISABLE);
  } else {
    if (doc.HasMember(config::kForceCalcNewStyleKey) &&
        doc[config::kForceCalcNewStyleKey].IsBool()) {
      page_config.get()->SetForceCalcNewStyle(
          doc[config::kForceCalcNewStyleKey].GetBool());
    }
  }

  /**
   * @name: extraInfo
   * @description: user defined extra info.
   * @note: None
   * @platform: Both
   * @supportVersion: 2.9
   **/
  if (doc.HasMember(config::kUserDefinedExtraInfo) &&
      doc[config::kUserDefinedExtraInfo].IsObject()) {
    page_config->SetExtraInfo(
        lepus::jsonValueTolepusValue(doc[config::kUserDefinedExtraInfo]));
  }

  if (doc.HasMember(config::kRemoveDescendantSelectorScope) &&
      doc[config::kRemoveDescendantSelectorScope].IsBool()) {
    page_config->SetRemoveDescendantSelectorScope(
        doc[config::kRemoveDescendantSelectorScope].GetBool());
  } else if (compile_options_.enable_fiber_arch_) {
    // Fiber arch, descendant selector only works in component scope by default
    page_config->SetRemoveDescendantSelectorScope(false);
  }

  page_config->SetEnableStandardCSSSelector(
      compile_options_.enable_css_selector_);

  // enableLynxScrollFluency
  if (doc.HasMember(config::kEnableLynxScrollFluency)) {
    if (doc[config::kEnableLynxScrollFluency].IsBool()) {
      page_config->SetEnableScrollFluencyMonitor(
          doc[config::kEnableLynxScrollFluency].GetBool() ? 1 : 0);
    } else if (doc[config::kEnableLynxScrollFluency].IsDouble()) {
      page_config->SetEnableScrollFluencyMonitor(
          doc[config::kEnableLynxScrollFluency].GetDouble());
    } else if (doc[config::kEnableLynxScrollFluency].IsInt()) {
      page_config->SetEnableScrollFluencyMonitor(
          doc[config::kEnableLynxScrollFluency].GetInt());
    }
  }

  UpdateCSSConfigs(page_config);

  config_helper_.HandlePageConfig(doc, page_config);
  page_config->SetOriginalConfig(std::move(config_str));
  ReportGlobalFeatureSwitch(page_config);
  return true;
}

void LynxBinaryConfigDecoder::UpdateCSSConfigs(
    const std::shared_ptr<PageConfig>& page_config) {
  auto configs =
      CSSParserConfigs::GetCSSParserConfigsByComplierOptions(compile_options_);
  page_config->SetCSSParserConfigs(configs);
}

bool LynxBinaryConfigDecoder::DecodeComponentConfig(
    const std::string& config_str,
    std::shared_ptr<ComponentConfig>& component_config) {
  rapidjson::Document doc;
  if (doc.Parse(config_str.c_str()).HasParseError()) {
    LOGE("DecodeComponentConfig Error");
    return false;
  }

  if (doc.HasMember(config::kEnableRemoveComponentExtraData) &&
      doc[config::kEnableRemoveComponentExtraData].IsBool()) {
    // only set when has this member defaults to undefined
    component_config->SetEnableRemoveExtraData(
        doc[config::kEnableRemoveComponentExtraData].GetBool());
  }

  if (doc.HasMember(config::kRemoveComponentElement) &&
      doc[config::kRemoveComponentElement].IsBool()) {
    component_config->SetRemoveComponentElement(
        doc[config::kRemoveComponentElement].GetBool());
  }
  return true;
}

/// TODO(limeng.amer): move to report thread.
/// Upload global feature switches in PageConfig with common data about lynx
/// view. If you add a new  global feature switch, you should add it to report
/// event.
void LynxBinaryConfigDecoder::ReportGlobalFeatureSwitch(
    const std::shared_ptr<PageConfig>& page_config) {
  if (!tasm::LynxEnv::GetInstance().EnableGlobalFeatureSwitchStatistic()) {
    return;
  }
  report::EventTracker::OnEvent([page_config](report::MoveOnlyEvent& event) {
    event.SetName(kLynxSDKGlobalFeatureSwitchEvent);
    event.SetProps(config::kImplicit, page_config->GetGlobalImplicit());
    event.SetProps(config::kEnableAsyncDisplay,
                   page_config->GetEnableAsyncDisplay());
    event.SetProps(config::kEnableViewReceiveTouch,
                   page_config->GetEnableViewReceiveTouch());
    event.SetProps(config::kEnableEventThrough,
                   page_config->GetEnableEventThrough());
    event.SetProps(config::kRemoveComponentElement,
                   page_config->GetRemoveComponentElement());
    event.SetProps(config::kEnableCSSInheritance,
                   page_config->GetEnableCSSInheritance());
    event.SetProps(config::kEnableListNewArchitecture,
                   page_config->GetListNewArchitecture());
    event.SetProps(config::kEnableReactOnlyPropsId,
                   page_config->GetEnableReactOnlyPropsId());
    event.SetProps(config::kEnableCircularDataCheck,
                   page_config->GetGlobalCircularDataCheck());
    event.SetProps(config::kEnableReduceInitDataCopy,
                   page_config->GetEnableReduceInitDataCopy());
    event.SetProps(config::kUnifyVWVHBehavior, page_config->GetUnifyVWVH());
    event.SetProps(config::kEnableComponentLayoutOnly,
                   page_config->GetEnableComponentLayoutOnly());
    event.SetProps(config::kAutoExpose, page_config->GetAutoExpose());
    event.SetProps(config::kAbsoluteInContentBound,
                   page_config->GetAbsoluteInContentBound());
    event.SetProps(config::kLongPressDuration,
                   page_config->GetLongPressDuration());
    event.SetProps(config::kObserverFrameRate,
                   page_config->GetObserverFrameRate());
    event.SetProps(config::kEnableExposureUIMargin,
                   page_config->GetEnableExposureUIMargin());
    event.SetProps(config::kFlatten, page_config->GetGlobalFlattern());
    event.SetProps(config::kForceCalcNewStyleKey,
                   page_config->GetForceCalcNewStyle());
    event.SetProps(config::kEnableComponentNullProp,
                   page_config->GetEnableComponentNullProp());
    event.SetProps(config::kRemoveDescendantSelectorScope,
                   page_config->GetRemoveDescendantSelectorScope());
    event.SetProps(config::kEnableComponentAsyncDecode,
                   page_config->GetEnableComponentAsyncDecode());
  });
}

}  // namespace tasm
}  // namespace lynx
