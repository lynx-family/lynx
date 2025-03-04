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
#include "core/runtime/piper/js/runtime_constant.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/services/event_report/event_tracker.h"

namespace lynx {
namespace tasm {
static constexpr const char* const kVersion = "version";
static constexpr const char* const kFlatten = "flatten";
static constexpr const char* const kImplicit = "implicit";
static constexpr const char* const kLepusCheck = "lepusStrict";
static constexpr const char* const kLepusQuickjsStackSize =
    "lepusQuickjsStacksize";
static constexpr const char* const kNullPropAsUndef = "lepusNullPropAsUndef";
static constexpr const char* const kDataStrictMode = "dataStrictMode";
static constexpr const char* const kAbsoluteInContentBound =
    "absoluteInContentBound";
static constexpr const char* const kQuirksMode = "quirksMode";
static constexpr const char* const kEnableAsyncDisplay = "enableAsyncDisplay";
static constexpr const char* const kEnableImageDownsampling =
    "enableImageDownsampling";
static constexpr const char* const kEnableFixedNew = "enableFixedNew";
static constexpr const char* const kEnableNewImage = "enableNewImage";
static constexpr const char* const kLogBoxImageSizeWarningThreshold =
    "redBoxImageSizeWarningThreshold";
static constexpr const char* const kEnableTextNonContiguousLayout =
    "enableTextNonContiguousLayout";
static constexpr const char* const kEnableViewReceiveTouch =
    "enableViewReceiveTouch";
static constexpr const char* const kEnableEventThrough = "enableEventThrough";
static constexpr const char* const kRemoveComponentElement =
    "removeComponentElement";
static constexpr const char* const kStrictPropType = "strictPropType";
static constexpr const char* const kEnableCSSInheritance =
    "enableCSSInheritance";
static constexpr const char* const kCustomCSSInheritanceList =
    "customCSSInheritanceList";
static constexpr const char* const kCSSAlignWithLegacyW3C =
    "cssAlignWithLegacyW3C";
static constexpr const char* const kUseNewImage = "useNewImage";
static constexpr const char* const kSyncImageAttach = "syncImageAttach";
static constexpr const char* const kUseImagePostProcessor =
    "useImagePostProcessor";
static constexpr const char* const kUseNewSwiper = "useNewSwiper";
static constexpr const char* const kEnableAsyncInitVideoEngine =
    "enableAsyncInitVideoEngine";
static constexpr const char* const kCliVersion = "cli";
static constexpr const char* const kReactVersion = "reactVersion";
static constexpr const char* const kCustomData = "customData";
static constexpr const char* const kEnableComponentLifecycleAlignWebview =
    "enableComponentLifecycleAlignWebview";
static constexpr const char* const kEnableListNewArchitecture =
    "enableListNewArchitecture";
static constexpr const char* const kEnableListNewContainer =
    "enableNewListContainer";
static constexpr const char* const kEnableListPlug = "enableListPlug";
static constexpr const char* const kEnableListMoveOperation =
    "enableListMoveOperation";
static constexpr const char* const kEnableCSSStrictMode = "enableCSSStrictMode";
static constexpr const char* const kTapSlop = "tapSlop";
static constexpr const char* const kDefaultTapSlop = "50px";
static constexpr const char* const kEnableCreateViewAsync =
    "enableCreateViewAsync";
static constexpr const char* const kEnableVsyncAlignedFlush =
    "enableVsyncAlignedFlush";
static constexpr const char* const kEnableAccessibilityElement =
    "enableAccessibilityElement";
static constexpr const char* const kEnableOverlapForAccessibilityElement =
    "enableOverlapForAccessibilityElement";
static constexpr const char* const kEnableNewAccessibility =
    "enableNewAccessibility";
static constexpr const char* const kEnableNewLayoutOnly = "enableNewLayoutOnly";
static constexpr const char* const kEnableReactOnlyPropsId =
    "enableReactOnlyPropsId";
static constexpr const char* const kEnableGlobalComponentMap =
    "enableGlobalComponentMap";
static constexpr const char* const kEnableTextRefactor = "enableTextRefactor";
static constexpr const char* const kEnableTextOverflow = "enableTextOverflow";
static constexpr const char* const kEnableTextBoringLayout =
    "enableTextBoringLayout";
static constexpr const char* const kEnableNewClipMode = "enableNewClipMode";
static constexpr const char* const kAutoResumeAnimation = "AutoResumeAnimation";
static constexpr const char* const kEnableNewTransformOrigin =
    "enableNewTransformOrigin";
static constexpr const char* const kEnableCircularDataCheck =
    "enableCircularDataCheck";
static constexpr const char* const kEnableTextLayerRender =
    "enableTextLayerRender";
static constexpr const char* const kEnableReduceInitDataCopy =
    "enableReduceInitDataCopy";
static constexpr const char* const kUnifyVWVHBehavior = "unifyVWVHBehavior";
static constexpr const char* const kFontScaleEffectiveOnlyOnSp =
    "fontScaleEffectiveOnlyOnSp";
static constexpr const char* const kEnableSimultaneousTap =
    "enableSimultaneousTap";
static constexpr const char* const kEnableComponentLayoutOnly =
    "enableComponentLayoutOnly";
static constexpr const char* const kExtendedLayoutOnlyOpt =
    "extendedLayoutOnlyOpt";
static constexpr const char* const kEnableTouchRefactor = "enableTouchRefactor";
static constexpr const char* const kEnableEndGestureAtLastFingerUp =
    "enableEndGestureAtLastFingerUp";
static constexpr const char* const kDisableLongpressAfterScroll =
    "disableLongpressAfterScroll";
static constexpr const char* const kKeyboardCallbackPassRelativeHeight =
    "keyboardCallbackPassRelativeHeight";
static constexpr const char* const kEnableNewIntersectionObserver =
    "enableNewIntersectionObserver";
static constexpr const char* const kObserverFrameRate = "observerFrameRate";
static constexpr const char* const kEnableCheckDataWhenUpdatePage =
    "enableCheckDataWhenUpdatePage";
static constexpr const char* const kForceCalcNewStyleKey = "forceCalcNewStyle";
static constexpr const char* const kIncludeFontPadding = "includeFontPadding";
static constexpr const char* const kEnableBackgroundShapeLayer =
    "enableBackgroundShapeLayer";
static constexpr const char* const kCompileRender = "compileRender";
static constexpr const char* const kEnableLynxResourceServiceProvider =
    "enableLynxResourceServiceProvider";
static constexpr const char* const kEnableTextLanguageAlignment =
    "enableTextLanguageAlignment";
static constexpr const char* const kEnableXTextLayoutReused =
    "enableXTextLayoutReused";
static constexpr const char* const kEnableRemoveComponentExtraData =
    "enableRemoveComponentExtraData";
static constexpr const char* const kEnableExposureUIMargin =
    "enableExposureUIMargin";
static constexpr const char* const kEnableNewGesture = "enableNewGesture";
static constexpr const char* const kLongPressDuration = "longPressDuration";
static constexpr const char* const kMapContainerType = "mapContainerType";
static constexpr const char* const kEnableCheckLocalImage =
    "enableCheckLocalImage";
static constexpr const char* const kEnableAsyncRequestImage =
    "enableAsyncRequestImage";
static constexpr const char* kUserDefinedExtraInfo = "extraInfo";
static constexpr const char* kLepusGCThreshold = "lepusGCThreshold";
static constexpr const char* kEnableComponentNullProp =
    "enableComponentNullProp";
static constexpr const char* kEnableCascadePseudo = "enableCascadePseudo";
static constexpr const char* kRemoveDescendantSelectorScope =
    "removeDescendantSelectorScope";
static constexpr const char* kAutoExpose = TEMPLATE_AUTO_EXPOSE;
static constexpr const char* kEnableUseContextPool = "enableUseContextPool";
static constexpr const char* kEnableUseMapBuffer = "enableUseMapBuffer";
static constexpr const char* kEnableFiberElementForRadonDiff =
    "enableFiberElementForRadonDiff";
static constexpr const char* kEnableLynxScrollFluency =
    "enableLynxScrollFluency";
static constexpr const char* kPreferredFps = "preferredFps";
static constexpr const char* kEnableCSSLazyImport = "enableCSSLazyImport";
static constexpr const char* kEnableNewAnimator = "enableNewAnimator";
static constexpr const char* kDisableQuickTracingGC = "disableQuickTracingGC";

/// Upload global feature switches in PageConfig with common data about lynx
/// view. If you add a new  global feature switch, you should add it to report
/// event.
static constexpr const char* kLynxSDKGlobalFeatureSwitchEvent =
    "lynxsdk_global_feature_switch_statistic";

static constexpr const char* kEnableReloadLifecycle = "enableReloadLifecycle";

// @name: enableA11y
// @description: Enable Android A11y
// @platform: Android
// @supportVersion: 2.10
// TODO(dingwang): Default value should be set to true in the future.
static constexpr const char* const kEnableA11y = "enableA11y";

/**
 * @name: enableA11yIDMutationObserver
 * @description: Enable MutationObserver for accessibility
 * @platform: Both
 * @supportVersion: 2.8
 **/
static constexpr const char* const kEnableA11yIDMutationObserver =
    "enableA11yIDMutationObserver";

/**
 * @name: enableCheckExposureOptimize
 * @description: Enable exposure detection optimization
 * @platform: Both
 * @supportVersion: 2.10
 **/
static constexpr const char* const kEnableCheckExposureOptimize =
    "enableCheckExposureOptimize";

/**
 * @name: enableDisexposureWhenLynxHidden
 * @description: Enable send disexposure events when lynxview is hidden
 * @platform: Android
 * @supportVersion: 2.10
 **/
static constexpr const char* const kEnableDisexposureWhenLynxHidden =
    "enableDisexposureWhenLynxHidden";

/**
 * @name: enableExposureWhenLayout
 * @description: Enable exposure check when LynxView is layoutRequest
 * @platform: Android
 * @supportVersion: 2.11
 **/
static constexpr const char* const kEnableExposureWhenLayout =
    "enableExposureWhenLayout";

/**
 * @name: enableAirDetectRemovedKeysWhenUpdateData
 * @description: enable Air Mode detect removed keys when updating data from
 *native
 * @platform: Both
 * @supportVersion: 2.11
 **/
static constexpr const char* const kEnableAirDetectRemovedKeysWhenUpdateData =
    "enableAirDetectRemovedKeysWhenUpdateData";

/**
 * @name: enableJSDataProcessor
 * @description: Enable data processor on JS thread
 * @platform: Both
 * @supportVersion: 2.12
 **/
static constexpr const char* const kEnableJSDataProcessor =
    "enableJSDataProcessor";

/**
 * @name: enableMultiTouch
 * @description: Enable support multi-finger events
 * @platform: Both
 * @supportVersion: 2.12
 **/
static constexpr const char* const kEnableMultiTouch = "enableMultiTouch";

/**
 * @name: enableComponentAsyncDecode
 * @description: Enable dynamic components to be decoded in child threads before
 *they are delivered into tasm in async-loading.
 * @platform: Both
 * @supportVersion: 2.13
 **/
static constexpr const char kEnableComponentAsyncDecode[] =
    "enableComponentAsyncDecode";

/**
 * @name: enableJsBindingApiThrowException
 * @description: Enable js binding api throw exception
 * @platform: Both
 * @supportVersion: 2.14
 **/
static constexpr const char* const kEnableJsBindingApiThrowException =
    "enableJsBindingApiThrowException";
/**
 * @name: enableICU
 * @description: Enable Bind PRIMJS-ICU
 * @platform: Both
 * @supportVersion: 2.17
 **/
static constexpr const char* const kEnableBindICU = "enableICU";

/**
 * @name: enableQueryComponentSync
 * @description: Enable query component sync in background runtime
 * @platform: Both
 * @supportVersion: 3.0
 **/
static constexpr const char* const kEnableQueryComponentSync =
    "enableQueryComponentSync";

/**
 * @name: pipelineSchedulerConfig
 * @description: Scheduler config for pipeline, including
 * enableParallelElement/list-framework batch render and other scheduler config
 * @platform: Both
 * @supportVersion: 3.1
 */
static constexpr const char* const kPipelineSchedulerConfig =
    "pipelineSchedulerConfig";

/**
 * @name: enableNativeList
 * @description: Indicates whether use c++ list.
 * @supportVersion: 3.2
 */
static constexpr const char* const kEnableNativeList = "enableNativeList";

bool LynxBinaryConfigDecoder::DecodePageConfig(
    const std::string& config_str, std::shared_ptr<PageConfig>& page_config) {
  rapidjson::Document doc;
  if (doc.Parse(config_str.c_str()).HasParseError()) {
    LOGE("DecodePageConfig Error!");
    return false;
  }

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

  if (doc.HasMember(kVersion) && doc[kVersion].IsString()) {
    page_config->SetVersion(doc[kVersion].GetString());
  }
  if (doc.HasMember(kFlatten) && doc[kFlatten].IsBool()) {
    page_config->SetGlobalFlattern(doc[kFlatten].GetBool());
  }

  if (doc.HasMember(kEnableA11yIDMutationObserver) &&
      doc[kEnableA11yIDMutationObserver].IsBool()) {
    page_config->SetEnableA11yIDMutationObserver(
        doc[kEnableA11yIDMutationObserver].GetBool());
  }

  if (doc.HasMember(kEnableA11y) && doc[kEnableA11y].IsBool()) {
    page_config->SetEnableA11y(doc[kEnableA11y].GetBool());
  }

  if (doc.HasMember(kEnableCascadePseudo) &&
      doc[kEnableCascadePseudo].IsBool()) {
    page_config->SetEnableCascadePseudo(doc[kEnableCascadePseudo].GetBool());
  }

  if (lynx::tasm::Config::IsHigherOrEqual(target_sdk_version_,
                                          LYNX_VERSION_2_0)) {
    page_config->SetGlobalImplicit(false);
  }

  if (doc.HasMember(kImplicit) && doc[kImplicit].IsBool()) {
    page_config->SetGlobalImplicit(doc[kImplicit].GetBool());
  }

  if (doc.HasMember(kLepusCheck) && doc[kLepusCheck].IsBool()) {
    page_config->SetEnableLepusStrictCheck(doc[kLepusCheck].GetBool());
  }

  if (doc.HasMember(kLepusQuickjsStackSize) &&
      doc[kLepusQuickjsStackSize].IsUint()) {
    page_config->SetLepusQuickjsStackSize(
        doc[kLepusQuickjsStackSize].GetUint());
  }

  if (doc.HasMember(kNullPropAsUndef) && doc[kNullPropAsUndef].IsBool()) {
    page_config->SetEnableLepusNullPropAsUndef(doc[kNullPropAsUndef].GetBool());
  } else if (lynx::tasm::Config::IsHigherOrEqual(target_sdk_version_,
                                                 LYNX_VERSION_1_6)) {
    page_config->SetEnableLepusNullPropAsUndef(true);
  }

  if (doc.HasMember(TEMPLATE_BUNDLE_APP_DSL) &&
      doc[TEMPLATE_BUNDLE_APP_DSL].IsInt()) {
    page_config->SetDSL(
        static_cast<PackageInstanceDSL>(doc[TEMPLATE_BUNDLE_APP_DSL].GetInt()));
  }

  if (doc.HasMember(kAutoExpose) && doc[kAutoExpose].IsBool()) {
    page_config.get()->SetAutoExpose(doc[kAutoExpose].GetBool());
  }

  if (doc.HasMember(kDataStrictMode) && doc[kDataStrictMode].IsBool()) {
    page_config.get()->SetDataStrictMode(doc[kDataStrictMode].GetBool());
  }

  if (doc.HasMember(kEnableFixedNew) && doc[kEnableFixedNew].IsBool()) {
    page_config.get()->SetEnableFixedNew(doc[kEnableFixedNew].GetBool());
  }

  if (doc.HasMember(kAbsoluteInContentBound) &&
      doc[kAbsoluteInContentBound].IsBool()) {
    page_config.get()->SetAbsoluteInContentBound(
        doc[kAbsoluteInContentBound].GetBool());
  }

  if (doc.HasMember(kQuirksMode) && doc[kQuirksMode].IsBool()) {
    if (!doc[kQuirksMode].GetBool()) {
      page_config.get()->SetQuirksModeByVersion(kQuirksModeDisableVersion);
    }
  } else if (doc.HasMember(kQuirksMode) && doc[kQuirksMode].IsString()) {
    page_config.get()->SetQuirksModeByVersion(
        base::Version(doc[kQuirksMode].GetString()));
  } else if ((lynx::tasm::Config::IsHigherOrEqual(target_sdk_version_,
                                                  kQuirksModeDisableVersion))) {
    page_config.get()->SetQuirksModeByVersion(
        base::Version(target_sdk_version_));
  }

  if (doc.HasMember(kEnableAsyncDisplay) && doc[kEnableAsyncDisplay].IsBool()) {
    page_config.get()->SetEnableAsyncDisplay(
        doc[kEnableAsyncDisplay].GetBool());
  }

  if (doc.HasMember(kEnableImageDownsampling) &&
      doc[kEnableImageDownsampling].IsBool()) {
    page_config.get()->SetEnableImageDownsampling(
        doc[kEnableImageDownsampling].GetBool());
  }

  if (doc.HasMember(kEnableNewImage) && doc[kEnableNewImage].IsBool()) {
    page_config.get()->SetEnableNewImage(doc[kEnableNewImage].GetBool());
  }

  if (doc.HasMember(kEnableTextLanguageAlignment) &&
      doc[kEnableTextLanguageAlignment].IsBool()) {
    page_config.get()->SetEnableTextLanguageAlignment(
        doc[kEnableTextLanguageAlignment].GetBool());
  }
  if (doc.HasMember(kEnableXTextLayoutReused) &&
      doc[kEnableXTextLayoutReused].IsBool()) {
    page_config.get()->SetEnableXTextLayoutReused(
        doc[kEnableXTextLayoutReused].GetBool());
  }

  if (doc.HasMember(kLogBoxImageSizeWarningThreshold) &&
      doc[kLogBoxImageSizeWarningThreshold].IsInt()) {
    page_config.get()->SetLogBoxImageSizeWarningThreshold(
        doc[kLogBoxImageSizeWarningThreshold].GetInt());
  }

  if (doc.HasMember(kEnableTextNonContiguousLayout) &&
      doc[kEnableTextNonContiguousLayout].IsBool()) {
    page_config->SetEnableTextNonContiguousLayout(
        doc[kEnableTextNonContiguousLayout].GetBool());
  }

  if (doc.HasMember(kEnableViewReceiveTouch) &&
      doc[kEnableViewReceiveTouch].IsBool()) {
    page_config.get()->SetEnableViewReceiveTouch(
        doc[kEnableViewReceiveTouch].GetBool());
  }

  if (doc.HasMember(kEnableEventThrough) && doc[kEnableEventThrough].IsBool()) {
    page_config->SetEnableEventThrough(doc[kEnableEventThrough].GetBool());
  }

  if (doc.HasMember(kRemoveComponentElement) &&
      doc[kRemoveComponentElement].IsBool()) {
    page_config->SetRemoveComponentElement(
        doc[kRemoveComponentElement].GetBool());
  }

  if (doc.HasMember(kStrictPropType) && doc[kStrictPropType].IsBool()) {
    page_config->SetStrictPropType(doc[kStrictPropType].GetBool());
  }

  if (doc.HasMember(kEnableCSSInheritance) &&
      doc[kEnableCSSInheritance].IsBool()) {
    page_config->SetEnableCSSInheritance(doc[kEnableCSSInheritance].GetBool());
  }

  if (doc.HasMember(kCustomCSSInheritanceList) &&
      doc[kCustomCSSInheritanceList].IsArray()) {
    std::unordered_set<CSSPropertyID> inherit_list;
    const auto& names_array = doc[kCustomCSSInheritanceList].GetArray();
    inherit_list.reserve(names_array.Size());
    for (const auto& entry : names_array) {
      if (entry.IsString()) {
        inherit_list.insert(
            tasm::CSSProperty::GetPropertyID(entry.GetString()));
      }
    }
    page_config->SetCustomCSSInheritList(std::move(inherit_list));
  }

  if (doc.HasMember(kCSSAlignWithLegacyW3C) &&
      doc[kCSSAlignWithLegacyW3C].IsBool()) {
    page_config->SetCSSAlignWithLegacyW3C(
        doc[kCSSAlignWithLegacyW3C].GetBool());
  }

  if (doc.HasMember(kUseNewImage) && doc[kUseNewImage].IsBool()) {
    page_config->SetUseNewImage(doc[kUseNewImage].GetBool()
                                    ? TernaryBool::TRUE_VALUE
                                    : TernaryBool::FALSE_VALUE);
  }

  if (doc.HasMember(kSyncImageAttach) && doc[kSyncImageAttach].IsBool()) {
    page_config->SetSyncImageAttach(doc[kSyncImageAttach].GetBool());
  }

  if (doc.HasMember(kUseImagePostProcessor) &&
      doc[kUseImagePostProcessor].IsBool()) {
    page_config->SetUseImagePostProcessor(
        doc[kUseImagePostProcessor].GetBool());
  }

  if (doc.HasMember(kUseNewSwiper) && doc[kUseNewSwiper].IsBool()) {
    page_config->SetUseNewSwiper(doc[kUseNewSwiper].GetBool());
  }

  if (doc.HasMember(kEnableAsyncInitVideoEngine) &&
      doc[kEnableAsyncInitVideoEngine].IsBool()) {
    page_config->SetEnableAsyncInitTTVideoEngine(
        doc[kEnableAsyncInitVideoEngine].GetBool());
  }

  if (doc.HasMember(kCliVersion) && doc[kCliVersion].IsString()) {
    page_config->SetCliVersion(doc[kCliVersion].GetString());
  }

  if (doc.HasMember(kReactVersion) && doc[kReactVersion].IsString()) {
    page_config->SetReactVersion(doc[kReactVersion].GetString());
  }

  if (doc.HasMember(kCustomData) && doc[kCustomData].IsString()) {
    page_config->SetCustomData(doc[kCustomData].GetString());
  }

  if (doc.HasMember(kEnableComponentLifecycleAlignWebview) &&
      doc[kEnableComponentLifecycleAlignWebview].IsBool()) {
    page_config->SetEnableComponentLifecycleAlignWebview(
        doc[kEnableComponentLifecycleAlignWebview].GetBool());
  }

  if (doc.HasMember(kEnableListNewArchitecture) &&
      doc[kEnableListNewArchitecture].IsBool()) {
    page_config->SetListNewArchitecture(
        doc[kEnableListNewArchitecture].GetBool());
  }

  if (doc.HasMember(kEnableListNewContainer) &&
      doc[kEnableListNewContainer].IsBool()) {
    page_config->SetEnableNewListContainer(
        doc[kEnableListNewContainer].GetBool());
  }

  if (doc.HasMember(kEnableListPlug) && doc[kEnableListPlug].IsBool()) {
    page_config->SetEnableListPlug(doc[kEnableListPlug].GetBool());
  }

  if (doc.HasMember(kEnableListMoveOperation) &&
      doc[kEnableListMoveOperation].IsBool()) {
    page_config->SetEnableListMoveOperation(
        doc[kEnableListMoveOperation].GetBool());
  }

  if (doc.HasMember(kEnableCSSStrictMode) &&
      doc[kEnableCSSStrictMode].IsBool()) {
    page_config->SetEnableCSSStrictMode(doc[kEnableCSSStrictMode].GetBool());
  }

  if (doc.HasMember(kTapSlop) && doc[kTapSlop].IsString()) {
    page_config->SetTapSlop(doc[kTapSlop].GetString());
  } else {
    page_config->SetTapSlop(kDefaultTapSlop);
  }

  if (doc.HasMember(kEnableCreateViewAsync) &&
      doc[kEnableCreateViewAsync].IsBool()) {
    page_config->SetEnableCreateViewAsync(
        doc[kEnableCreateViewAsync].GetBool());
  }

  if (doc.HasMember(kEnableVsyncAlignedFlush) &&
      doc[kEnableVsyncAlignedFlush].IsBool()) {
    page_config->SetEnableVsyncAlignedFlush(
        doc[kEnableVsyncAlignedFlush].GetBool());
  }

  if (doc.HasMember(kEnableAccessibilityElement) &&
      doc[kEnableAccessibilityElement].IsBool()) {
    page_config->SetEnableAccessibilityElement(
        doc[kEnableAccessibilityElement].GetBool());
  }

  if (doc.HasMember(kEnableOverlapForAccessibilityElement) &&
      doc[kEnableOverlapForAccessibilityElement].IsBool()) {
    page_config->SetEnableOverlapForAccessibilityElement(
        doc[kEnableOverlapForAccessibilityElement].GetBool());
  }

  if (doc.HasMember(kEnableNewAccessibility) &&
      doc[kEnableNewAccessibility].IsBool()) {
    page_config->SetEnableNewAccessibility(
        doc[kEnableNewAccessibility].GetBool());
  }
  if (doc.HasMember(kEnableNewLayoutOnly) &&
      doc[kEnableNewLayoutOnly].IsBool()) {
    page_config->SetEnableNewLayoutOnly(doc[kEnableNewLayoutOnly].GetBool());
  }

  if (doc.HasMember(kEnableReactOnlyPropsId) &&
      doc[kEnableReactOnlyPropsId].IsBool()) {
    page_config->SetEnableReactOnlyPropsId(
        doc[kEnableReactOnlyPropsId].GetBool());
  }

  if (doc.HasMember(kEnableGlobalComponentMap) &&
      doc[kEnableGlobalComponentMap].IsBool()) {
    page_config->SetEnableGlobalComponentMap(
        doc[kEnableGlobalComponentMap].GetBool());
  }

  if (doc.HasMember(kEnableRemoveComponentExtraData) &&
      doc[kEnableRemoveComponentExtraData].IsBool()) {
    page_config->SetEnableRemoveComponentExtraData(
        doc[kEnableRemoveComponentExtraData].GetBool());
  }

  if (doc.HasMember(kEnableTextRefactor) && doc[kEnableTextRefactor].IsBool()) {
    page_config->SetEnableTextRefactor(doc[kEnableTextRefactor].GetBool());
  } else if (lynx::tasm::Config::IsHigherOrEqual(target_sdk_version_,
                                                 LYNX_VERSION_2_2)) {
    page_config->SetEnableTextRefactor(true);
  }

  if (doc.HasMember(kEnableTextOverflow) && doc[kEnableTextOverflow].IsBool()) {
    page_config->SetEnableTextOverflow(doc[kEnableTextOverflow].GetBool());
  } else if (lynx::tasm::Config::IsHigherOrEqual(target_sdk_version_,
                                                 LYNX_VERSION_2_8)) {
    page_config->SetEnableTextOverflow(true);
  }

  if (doc.HasMember(kEnableTextBoringLayout) &&
      doc[kEnableTextBoringLayout].IsBool()) {
    page_config->SetEnableTextBoringLayout(
        doc[kEnableTextBoringLayout].GetBool() ? TernaryBool::TRUE_VALUE
                                               : TernaryBool::FALSE_VALUE);
  }

  if (doc.HasMember(kEnableNewClipMode) && doc[kEnableNewClipMode].IsBool()) {
    page_config->SetEnableNewClipMode(doc[kEnableNewClipMode].GetBool());
  } else if (lynx::tasm::Config::IsHigherOrEqual(target_sdk_version_,
                                                 LYNX_VERSION_2_10)) {
    page_config->SetEnableNewClipMode(true);
  }

  if (doc.HasMember(kAutoResumeAnimation) &&
      doc[kAutoResumeAnimation].IsBool()) {
    page_config->SetGlobalAutoResumeAnimation(
        doc[kAutoResumeAnimation].GetBool());
  } else if (lynx::tasm::Config::IsHigherOrEqual(target_sdk_version_,
                                                 LYNX_VERSION_2_3)) {
    page_config->SetGlobalAutoResumeAnimation(true);
  }

  if (doc.HasMember(kEnableNewTransformOrigin) &&
      doc[kEnableNewTransformOrigin].IsBool()) {
    page_config->SetGlobalEnableNewTransformOrigin(
        doc[kEnableNewTransformOrigin].GetBool());
  } else if (lynx::tasm::Config::IsHigherOrEqual(target_sdk_version_,
                                                 LYNX_VERSION_2_6)) {
    page_config->SetGlobalEnableNewTransformOrigin(true);
  }

  if (doc.HasMember(kEnableCircularDataCheck) &&
      doc[kEnableCircularDataCheck].IsBool()) {
    page_config->SetGlobalCircularDataCheck(
        doc[kEnableCircularDataCheck].GetBool());
  }

  if (doc.HasMember(kEnableTextLayerRender) &&
      doc[kEnableTextLayerRender].IsBool()) {
    page_config->SetEnableTextLayerRender(doc[kEnableTextLayerRender].GetBool()
                                              ? TernaryBool::TRUE_VALUE
                                              : TernaryBool::FALSE_VALUE);
  }

  if (doc.HasMember(kEnableReduceInitDataCopy) &&
      doc[kEnableReduceInitDataCopy].IsBool()) {
    page_config->SetEnableReduceInitDataCopy(
        doc[kEnableReduceInitDataCopy].GetBool());
  }

  if (doc.HasMember(kUnifyVWVHBehavior) && doc[kUnifyVWVHBehavior].IsBool()) {
    page_config->SetUnifyVWVH(doc[kUnifyVWVHBehavior].GetBool());
  } else if (lynx::tasm::Config::IsHigherOrEqual(target_sdk_version_,
                                                 LYNX_VERSION_2_3)) {
    page_config->SetUnifyVWVH(true);
  }

  if (doc.HasMember(kFontScaleEffectiveOnlyOnSp) &&
      doc[kFontScaleEffectiveOnlyOnSp].IsBool()) {
    page_config->SetFontScaleSpOnly(doc[kFontScaleEffectiveOnlyOnSp].GetBool());
  }

  if (doc.HasMember(kEnableSimultaneousTap) &&
      doc[kEnableSimultaneousTap].IsBool()) {
    page_config->SetEnableSimultaneousTap(
        doc[kEnableSimultaneousTap].GetBool());
  }

  if (doc.HasMember(kEnableComponentLayoutOnly) &&
      doc[kEnableComponentLayoutOnly].IsBool()) {
    page_config->SetEnableComponentLayoutOnly(
        doc[kEnableComponentLayoutOnly].GetBool());
  } else if (lynx::tasm::Config::IsHigherOrEqual(target_sdk_version_,
                                                 LYNX_VERSION_2_6)) {
    page_config->SetEnableComponentLayoutOnly(true);
  }

  if (doc.HasMember(kExtendedLayoutOnlyOpt) &&
      doc[kExtendedLayoutOnlyOpt].IsBool()) {
    page_config->SetEnableExtendedLayoutOpt(
        doc[kExtendedLayoutOnlyOpt].GetBool());
  } else if (lynx::tasm::Config::IsHigherOrEqual(target_sdk_version_,
                                                 LYNX_VERSION_2_13)) {
    page_config->SetEnableExtendedLayoutOpt(true);
  }

  if (doc.HasMember(kEnableTouchRefactor) &&
      doc[kEnableTouchRefactor].IsBool()) {
    page_config->SetEnableTouchRefactor(doc[kEnableTouchRefactor].GetBool());
  }

  if (doc.HasMember(kEnableEndGestureAtLastFingerUp) &&
      doc[kEnableEndGestureAtLastFingerUp].IsBool()) {
    page_config->SetEnableEndGestureAtLastFingerUp(
        doc[kEnableEndGestureAtLastFingerUp].GetBool());
  }

  if (doc.HasMember(kDisableLongpressAfterScroll) &&
      doc[kDisableLongpressAfterScroll].IsBool()) {
    page_config->SetDisableLongpressAfterScroll(
        doc[kDisableLongpressAfterScroll].GetBool());
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

  if (doc.HasMember(kKeyboardCallbackPassRelativeHeight) &&
      doc[kKeyboardCallbackPassRelativeHeight].IsBool()) {
    page_config->SetKeyboardCallbackUseRelativeHeight(
        doc[kKeyboardCallbackPassRelativeHeight].GetBool());
  } else if (lynx::tasm::Config::IsHigherOrEqual(target_sdk_version_,
                                                 LYNX_VERSION_2_2)) {
    page_config->SetKeyboardCallbackUseRelativeHeight(true);
  }

  if (doc.HasMember(kEnableNewIntersectionObserver) &&
      doc[kEnableNewIntersectionObserver].IsBool()) {
    page_config->SetEnableNewIntersectionObserver(
        doc[kEnableNewIntersectionObserver].GetBool());
  }

  if (doc.HasMember(kObserverFrameRate) && doc[kObserverFrameRate].IsInt()) {
    page_config->SetObserverFrameRate(doc[kObserverFrameRate].GetInt());
  }

  if (doc.HasMember(kEnableCheckExposureOptimize) &&
      doc[kEnableCheckExposureOptimize].IsBool()) {
    page_config->SetEnableCheckExposureOptimize(
        doc[kEnableCheckExposureOptimize].GetBool());
  }

  if (doc.HasMember(kEnableDisexposureWhenLynxHidden) &&
      doc[kEnableDisexposureWhenLynxHidden].IsBool()) {
    page_config->SetEnableDisexposureWhenLynxHidden(
        doc[kEnableDisexposureWhenLynxHidden].GetBool());
  }

  if (doc.HasMember(kEnableExposureWhenLayout) &&
      doc[kEnableExposureWhenLayout].IsBool()) {
    page_config->SetEnableExposureWhenLayout(
        doc[kEnableExposureWhenLayout].GetBool());
  }

  if (doc.HasMember(kEnableAirDetectRemovedKeysWhenUpdateData) &&
      doc[kEnableAirDetectRemovedKeysWhenUpdateData].IsBool()) {
    page_config->SetEnableAirDetectRemovedKeysWhenUpdateData(
        doc[kEnableAirDetectRemovedKeysWhenUpdateData].GetBool());
  }

  if (doc.HasMember(kEnableExposureUIMargin) &&
      doc[kEnableExposureUIMargin].IsBool()) {
    page_config->SetEnableExposureUIMargin(
        doc[kEnableExposureUIMargin].GetBool());
  }

  // parse enable new gesture config
  if (doc.HasMember(kEnableNewGesture) && doc[kEnableNewGesture].IsBool()) {
    page_config->SetEnableNewGesture(doc[kEnableNewGesture].GetBool());
  }

  if (doc.HasMember(kLongPressDuration) && doc[kLongPressDuration].IsInt()) {
    page_config->SetLongPressDuration(doc[kLongPressDuration].GetInt());
  }

  if (doc.HasMember(kMapContainerType) && doc[kMapContainerType].IsInt()) {
    page_config->SetMapContainerType(doc[kMapContainerType].GetInt());
  }

  if (doc.HasMember(kEnableCheckLocalImage) &&
      doc[kEnableCheckLocalImage].IsBool()) {
    page_config->SetEnableCheckLocalImage(
        doc[kEnableCheckLocalImage].GetBool());
  }

  if (doc.HasMember(kEnableAsyncRequestImage) &&
      doc[kEnableAsyncRequestImage].IsBool()) {
    page_config->SetEnableAsyncRequestImage(
        doc[kEnableAsyncRequestImage].GetBool());
  }

  if (doc.HasMember(kEnableCheckDataWhenUpdatePage) &&
      doc[kEnableCheckDataWhenUpdatePage].IsBool()) {
    page_config->SetEnableCheckDataWhenUpdatePage(
        doc[kEnableCheckDataWhenUpdatePage].GetBool());
  }

  if (doc.HasMember(kEnableJSDataProcessor) &&
      doc[kEnableJSDataProcessor].IsBool()) {
    page_config->SetEnableDataProcessorOnJs(
        doc[kEnableJSDataProcessor].GetBool());
  }

  if (doc.HasMember(kEnableMultiTouch) && doc[kEnableMultiTouch].IsBool()) {
    page_config->SetEnableMultiTouch(doc[kEnableMultiTouch].GetBool());
  }

  if (doc.HasMember(kEnableNativeList) && doc[kEnableNativeList].IsBool()) {
    page_config->SetEnableNativeList(doc[kEnableNativeList].GetBool());
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
    if (doc.HasMember(kForceCalcNewStyleKey) &&
        doc[kForceCalcNewStyleKey].IsBool()) {
      page_config.get()->SetForceCalcNewStyle(
          doc[kForceCalcNewStyleKey].GetBool());
    }
  }

  // include font padding
  if (doc.HasMember(kIncludeFontPadding) && doc[kIncludeFontPadding].IsBool()) {
    page_config->SetIncludeFontPadding(doc[kIncludeFontPadding].GetBool());
  }

  if (doc.HasMember(kEnableBackgroundShapeLayer) &&
      doc[kEnableBackgroundShapeLayer].IsBool()) {
    page_config->SetEnableBackgroundShapeLayer(
        doc[kEnableBackgroundShapeLayer].GetBool());
  }

  // compile render
  if (doc.HasMember(kCompileRender) && doc[kCompileRender].IsBool()) {
    page_config.get()->SetCompileRender(doc[kCompileRender].GetBool());
  }

  /**
   * @name: enableLynxResourceServiceProvider
   * @description: Enable LynxResourceService to fetch external resource
   * @note: None
   * @platform: Both
   * @supportVersion: 2.8
   **/
  if (doc.HasMember(kEnableLynxResourceServiceProvider) &&
      doc[kEnableLynxResourceServiceProvider].IsBool()) {
    page_config->SetEnableLynxResourceServiceProvider(
        doc[kEnableLynxResourceServiceProvider].GetBool());
  }

  /**
   * @name: extraInfo
   * @description: user defined extra info.
   * @note: None
   * @platform: Both
   * @supportVersion: 2.9
   **/
  if (doc.HasMember(kUserDefinedExtraInfo) &&
      doc[kUserDefinedExtraInfo].IsObject()) {
    page_config->SetExtraInfo(
        lepus::jsonValueTolepusValue(doc[kUserDefinedExtraInfo]));
  }

  /**
   * @name: lepusGCThreshold
   * @description: the gc threshold of lepusNG.
   * @note: None
   * @platform: Both
   * @supportVersion: 2.9
   **/
  if (doc.HasMember(kLepusGCThreshold) && doc[kLepusGCThreshold].IsInt()) {
    page_config->SetLepusGCThreshold(doc[kLepusGCThreshold].GetInt());
  }

  /**
   * @name: enableComponentNullProp
   * @description: lepus support use null as component prop
   * @note: None
   * @platform: Both
   * @supportVersion: 2.9
   **/
  if (doc.HasMember(kEnableComponentNullProp) &&
      doc[kEnableComponentNullProp].IsBool()) {
    page_config->SetEnableComponentNullProp(
        doc[kEnableComponentNullProp].GetBool());
  }

  if (doc.HasMember(kRemoveDescendantSelectorScope) &&
      doc[kRemoveDescendantSelectorScope].IsBool()) {
    page_config->SetRemoveDescendantSelectorScope(
        doc[kRemoveDescendantSelectorScope].GetBool());
  } else if (compile_options_.enable_fiber_arch_) {
    // Fiber arch, descendant selector only works in component scope by default
    page_config->SetRemoveDescendantSelectorScope(false);
  }

  page_config->SetEnableStandardCSSSelector(
      compile_options_.enable_css_selector_);

  // enableComponentAsyncDecode
  if (doc.HasMember(kEnableComponentAsyncDecode) &&
      doc[kEnableComponentAsyncDecode].IsBool()) {
    page_config->SetEnableComponentAsyncDecode(
        doc[kEnableComponentAsyncDecode].GetBool());
  }

  // enableUseContextPool
  if (doc.HasMember(kEnableUseContextPool) &&
      doc[kEnableUseContextPool].IsBool()) {
    page_config->SetEnableUseContextPool(doc[kEnableUseContextPool].GetBool());
  }

  if (doc.HasMember(kEnableUseMapBuffer) && doc[kEnableUseMapBuffer].IsBool()) {
    page_config->SetEnableUseMapBuffer(doc[kEnableUseMapBuffer].GetBool()
                                           ? TernaryBool::TRUE_VALUE
                                           : TernaryBool::FALSE_VALUE);
  }

  if (doc.HasMember(kEnableFiberElementForRadonDiff) &&
      doc[kEnableFiberElementForRadonDiff].IsBool()) {
    page_config->SetEnableFiberElementForRadonDiff(
        doc[kEnableFiberElementForRadonDiff].GetBool()
            ? TernaryBool::TRUE_VALUE
            : TernaryBool::FALSE_VALUE);
  }

  // enableLynxScrollFluency
  if (doc.HasMember(kEnableLynxScrollFluency)) {
    if (doc[kEnableLynxScrollFluency].IsBool()) {
      page_config->SetEnableScrollFluencyMonitor(
          doc[kEnableLynxScrollFluency].GetBool() ? 1 : 0);
    } else if (doc[kEnableLynxScrollFluency].IsDouble()) {
      page_config->SetEnableScrollFluencyMonitor(
          doc[kEnableLynxScrollFluency].GetDouble());
    } else if (doc[kEnableLynxScrollFluency].IsInt()) {
      page_config->SetEnableScrollFluencyMonitor(
          doc[kEnableLynxScrollFluency].GetInt());
    }
  }

  // preferred_fps
  if (doc.HasMember(kPreferredFps) && doc[kPreferredFps].IsString()) {
    page_config->SetPreferredFps(doc[kPreferredFps].GetString());
  }

  // enableCSSLazyImport
  if (doc.HasMember(kEnableCSSLazyImport) &&
      doc[kEnableCSSLazyImport].IsBool()) {
    page_config->SetEnableCSSLazyImport(doc[kEnableCSSLazyImport].GetBool()
                                            ? TernaryBool::TRUE_VALUE
                                            : TernaryBool::FALSE_VALUE);
  }

  // enableNewAnimator
  if (doc.HasMember(kEnableNewAnimator) && doc[kEnableNewAnimator].IsBool()) {
    page_config->SetEnableNewAnimator(doc[kEnableNewAnimator].GetBool()
                                          ? TernaryBool::TRUE_VALUE
                                          : TernaryBool::FALSE_VALUE);
  } else {
    page_config->SetEnableNewAnimator(TernaryBool::UNDEFINE_VALUE);
  }

  // TODO(nihao.royal): removed in later versions
  if (doc.HasMember(kEnableReloadLifecycle) &&
      doc[kEnableReloadLifecycle].IsBool()) {
    page_config->SetEnableReloadLifecycle(
        doc[kEnableReloadLifecycle].GetBool());
  }

  // enableJsBindApiThrowException
  // user configuration takes priority, otherwise the default configuration is
  // set by target_sdk_version. Enable it if it is greater than version 2.14,
  // disable it if it is less than.
  page_config->SetEnableJsBindingApiThrowException(
      (doc.HasMember(kEnableJsBindingApiThrowException) &&
       doc[kEnableJsBindingApiThrowException].IsBool())
          ? doc[kEnableJsBindingApiThrowException].GetBool()
          : lynx::tasm::Config::IsHigherOrEqual(target_sdk_version_,
                                                LYNX_VERSION_2_14));

  // Parse page config dynamically
  // TODO(nihao.royal) enumerate doc and parse page config one by one, and unify
  // parameters of different types.
  page_config->ForEachBoolConfig([&doc](const std::string& name) {
    const char* const key = name.c_str();
    if (doc.HasMember(key) && doc[key].IsBool()) {
      return doc[key].GetBool() ? TernaryBool::TRUE_VALUE
                                : TernaryBool::FALSE_VALUE;
    }
    return TernaryBool::UNDEFINE_VALUE;
  });

  page_config->SetEnableElementAPITypeCheckThrowWarning(
      lynx::tasm::Config::IsHigherOrEqual(target_sdk_version_,
                                          LYNX_VERSION_2_16));

  UpdateCSSConfigs(page_config);

  // enableICU
  if (doc.HasMember(kEnableBindICU) && doc[kEnableBindICU].IsBool()) {
    page_config->SetEnableBindICU(doc[kEnableBindICU].GetBool());
  }

  // enableQueryComponentSync
  if (doc.HasMember(kEnableQueryComponentSync) &&
      doc[kEnableQueryComponentSync].IsBool()) {
    page_config->SetEnableQueryComponentSync(
        doc[kEnableQueryComponentSync].GetBool());
  }

  if (doc.HasMember(kPipelineSchedulerConfig) &&
      doc[kPipelineSchedulerConfig].IsUint64()) {
    page_config->SetPipelineSchedulerConfig(
        doc[kPipelineSchedulerConfig].GetUint64());
  }

  // enableMicrotaskPromisePolyfill
  if (doc.HasMember(runtime::kEnableMicrotaskPromisePolyfill) &&
      doc[runtime::kEnableMicrotaskPromisePolyfill].IsBool()) {
    page_config->SetEnableMicrotaskPromisePolyfill(
        doc[runtime::kEnableMicrotaskPromisePolyfill].GetBool());
  }

  // disableQuickTracingGC
  if (doc.HasMember(kDisableQuickTracingGC) &&
      doc[kDisableQuickTracingGC].IsBool()) {
    page_config->SetDisableQuickTracingGC(
        doc[kDisableQuickTracingGC].GetBool());
  }

  // enableSignalAPI
  if (doc.HasMember(kEnableSignalAPI) && doc[kEnableSignalAPI].IsBool()) {
    page_config->SetEnableSignalAPI(doc[kEnableSignalAPI].GetBool()
                                        ? TernaryBool::TRUE_VALUE
                                        : TernaryBool::FALSE_VALUE);
  }

  config_helper_.HandlePageConfig(doc, page_config);

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

  if (doc.HasMember(kEnableRemoveComponentExtraData) &&
      doc[kEnableRemoveComponentExtraData].IsBool()) {
    // only set when has this member defaults to undefined
    component_config->SetEnableRemoveExtraData(
        doc[kEnableRemoveComponentExtraData].GetBool());
  }

  if (doc.HasMember(kRemoveComponentElement) &&
      doc[kRemoveComponentElement].IsBool()) {
    component_config->SetRemoveComponentElement(
        doc[kRemoveComponentElement].GetBool());
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
    event.SetProps(kImplicit, page_config->GetGlobalImplicit());
    event.SetProps(kEnableAsyncDisplay, page_config->GetEnableAsyncDisplay());
    event.SetProps(kEnableViewReceiveTouch,
                   page_config->GetEnableViewReceiveTouch());
    event.SetProps(kEnableEventThrough, page_config->GetEnableEventThrough());
    event.SetProps(kRemoveComponentElement,
                   page_config->GetRemoveComponentElement());
    event.SetProps(kEnableCSSInheritance,
                   page_config->GetEnableCSSInheritance());
    event.SetProps(kEnableListNewArchitecture,
                   page_config->GetListNewArchitecture());
    event.SetProps(kEnableCSSStrictMode, page_config->GetEnableCSSStrictMode());
    event.SetProps(kEnableReactOnlyPropsId,
                   page_config->GetEnableReactOnlyPropsId());
    event.SetProps(kEnableCircularDataCheck,
                   page_config->GetGlobalCircularDataCheck());
    event.SetProps(kEnableReduceInitDataCopy,
                   page_config->GetEnableReduceInitDataCopy());
    event.SetProps(kUnifyVWVHBehavior, page_config->GetUnifyVWVH());
    event.SetProps(kEnableComponentLayoutOnly,
                   page_config->GetEnableComponentLayoutOnly());
    event.SetProps(kAutoExpose, page_config->GetAutoExpose());
    event.SetProps(kAbsoluteInContentBound,
                   page_config->GetAbsoluteInContentBound());
    event.SetProps(kLongPressDuration, page_config->GetLongPressDuration());
    event.SetProps(kObserverFrameRate, page_config->GetObserverFrameRate());
    event.SetProps(kEnableExposureUIMargin,
                   page_config->GetEnableExposureUIMargin());
    event.SetProps(kFlatten, page_config->GetGlobalFlattern());
    event.SetProps(kForceCalcNewStyleKey, page_config->GetForceCalcNewStyle());
    event.SetProps(kEnableLynxResourceServiceProvider,
                   page_config->GetEnableLynxResourceServiceProvider());
    event.SetProps(kEnableComponentNullProp,
                   page_config->GetEnableComponentNullProp());
    event.SetProps(kRemoveDescendantSelectorScope,
                   page_config->GetRemoveDescendantSelectorScope());
    event.SetProps(kEnableComponentAsyncDecode,
                   page_config->GetEnableComponentAsyncDecode());
  });
}

}  // namespace tasm
}  // namespace lynx
