// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/android/tasm_platform_invoker_android.h"

#include <memory>
#include <utility>
#include <vector>

#include "core/base/android/java_only_map.h"
// TODO(heshan):Temporarily utilize the JNI methods of TemplateAssembler.
//              Introduce TasmPlatformInvoker.java as a replacement
//              subsequently.
#include "base/include/platform/android/jni_convert_helper.h"
#include "core/build/gen/TasmPlatformInvoker_jni.h"
#include "core/renderer/dom/android/lepus_message_consumer.h"
#include "core/renderer/utils/android/value_converter_android.h"

namespace lynx {
namespace shell {

namespace {

// TODO(heshan): Currently duplicated in native_facade_android.cc.
//               Refactor into a shared method in the future.
lepus::Value ConvertJavaData(JNIEnv* env, jobject j_data, jint length) {
  if (j_data == nullptr || length <= 0) {
    return lepus::Value();
  }

  char* data = static_cast<char*>(env->GetDirectBufferAddress(j_data));
  if (data == nullptr) {
    return lepus::Value();
  }

  lynx::tasm::LepusDecoder decoder;
  return decoder.DecodeMessage(data, length);
}

}  // namespace

namespace {

constexpr const char* kPageVersion = "pageVersion";
constexpr const char* kAutoExpose = "autoExpose";
constexpr const char* kEnableEventThrough = "enableEventThrough";
constexpr const char* kUseNewImage = "useNewImage";
constexpr const char* kAsyncRedirectUrl = "asyncRedirect";
constexpr const char* kSyncImageAttach = "syncImageAttach";
constexpr const char* kUseImagePostProcessor = "useImagePostProcessor";
constexpr const char* kEnableCheckLocalImage = "enableCheckLocalImage";
constexpr const char* kEnableAsyncRequestImage = "enableAsyncRequestImage";
constexpr const char* kPageType = "pageType";
constexpr const char* kCliVersion = "cliVersion";
constexpr const char* kCustomData = "customData";
constexpr const char* kUseNewSwiper = "useNewSwiper";
constexpr const char* kEnableAsyncInitVideoEngine =
    "enableAsyncInitVideoEngine";
constexpr const char* kEnableNewGesture = "enableNewGesture";
constexpr const char* kTargetSdkVersion = "targetSdkVersion";
constexpr const char* kLepusVersion = "lepusVersion";
constexpr const char* kEnableLepusNg = "enableLepusNG";
constexpr const char* kRadonMode = "radonMode";
constexpr const char* kTapSlop = "tapSlop";
constexpr const char* kDefaultOverflowVisible = "defaultOverflowVisible";
constexpr const char* kEnableLynxScrollFluency = "enableLynxScrollFluency";

constexpr const char* kEnableCreateViewAsync = "enableCreateViewAsync";
constexpr const char* kEnableVsyncAlignedFlush = "enableVsyncAlignedFlush";
constexpr const char* kCssAlignWithLegacyW3c = "cssAlignWithLegacyW3c";
constexpr const char* kEnableAccessibilityElement =
    "enableAccessibilityElement";
constexpr const char* kEnableOverlapForAccessibilityElement =
    "enableOverlapForAccessibilityElement";
constexpr const char* kEnableNewAccessibility = "enableNewAccessibility";
constexpr const char* kEnableA11yIDMutationObserver =
    "enableA11yIDMutationObserver";
constexpr const char* kEnableA11y = "enableA11y";
constexpr const char* kReactVersion = "reactVersion";
constexpr const char* kEnableTextRefactor = "enableTextRefactor";
constexpr const char* kEnableTextOverflow = "enableTextOverflow";
constexpr const char* kEnableTextBoringLayout = "enableTextBoringLayout";
constexpr const char* kEnableNewClipMode = "enableNewClipMode";
constexpr const char* kKeyboardCallbackUseRelativeHeight =
    "keyboardCallbackPassRelativeHeight";
constexpr const char* kEnableCSSParser = "enableCSSParser";
constexpr const char* kEnableEventRefactor = "enableEventRefactor";
constexpr const char* kEnableDisexposureWhenLynxHidden =
    "enableDisexposureWhenLynxHidden";
static constexpr const char* const kEnableExposureWhenLayout =
    "enableExposureWhenLayout";
constexpr const char* kEnableNewIntersectionObserver =
    "enableNewIntersectionObserver";
constexpr const char* kIncludeFontPadding = "includeFontPadding";
constexpr const char* kObserverFrameRate = "observerFrameRate";
constexpr const char* kEnableExposureUIMargin = "enableExposureUIMargin";
constexpr const char* kLongPressDuration = "longPressDuration";
constexpr const char* kMapContainerType = "mapContainerType";
constexpr const char* kPageFlatten = "pageFlatten";
constexpr const char* kAbSettingDisableCSSLazyDecode =
    "abSettingDisableCssLazyDecode";
constexpr const char* kUser = "user";
constexpr const char* kGit = "git";
constexpr const char* kFilePath = "filePath";
constexpr const char* kTrailNewImage = "trailNewImage";
constexpr const char* kEnableNewImage = "enableNewImage";
constexpr const char* kEnableFiber = "enableFiber";
constexpr const char* kEnableMultiTouch = "enableMultiTouch";
constexpr const char* kEnableFlattenTranslateZ = "enableFlattenTranslateZ";

}  // namespace

void TasmPlatformInvokerAndroid::RegisterJni(JNIEnv* env) {
  (void)RegisterNativesImpl(env);
}

void TasmPlatformInvokerAndroid::OnPageConfigDecoded(
    const std::shared_ptr<tasm::PageConfig>& config) {
  base::android::JavaOnlyMap java_config;
  // put config that platform needed.
  java_config.PushBoolean(kAutoExpose, config->GetAutoExpose());
  java_config.PushString(kPageVersion, config->GetVersion());
  java_config.PushBoolean(kEnableEventThrough, config->GetEnableEventThrough());
  tasm::TernaryBool use_new_image = config->GetUseNewImage();
  if (use_new_image != tasm::TernaryBool::UNDEFINE_VALUE) {
    java_config.PushBoolean(kUseNewImage,
                            use_new_image == tasm::TernaryBool::TRUE_VALUE);
  }

  auto version = lynx::base::Version(config->GetTargetSDKVersion());

  java_config.PushBoolean(
      kAsyncRedirectUrl,
      config->GetAsyncRedirectUrl() == lynx::tasm::TernaryBool::TRUE_VALUE);
  java_config.PushBoolean(kSyncImageAttach, config->GetSyncImageAttach());
  java_config.PushBoolean(kEnableCheckLocalImage,
                          config->GetEnableCheckLocalImage());
  java_config.PushBoolean(kEnableAsyncRequestImage,
                          config->GetEnableAsyncRequestImage());
  java_config.PushBoolean(kUseImagePostProcessor,
                          config->GetUseImagePostProcessor());
  java_config.PushString(kPageType, tasm::GetDSLName(config->GetDSL()));
  java_config.PushString(kCliVersion, config->GetCliVersion());
  java_config.PushString(kCustomData, config->GetCustomData());
  java_config.PushBoolean(kUseNewSwiper, config->GetUseNewSwiper());
  java_config.PushBoolean(kEnableAsyncInitVideoEngine,
                          config->GetEnableAsyncInitTTVideoEngine());
  java_config.PushString(kTargetSdkVersion, config->GetTargetSDKVersion());
  java_config.PushBoolean(kEnableNewGesture, config->GetEnableNewGesture());
  java_config.PushString(kLepusVersion, config->GetLepusVersion());
  java_config.PushBoolean(kEnableLepusNg, config->GetEnableLepusNG());
  java_config.PushString(kRadonMode, config->GetRadonMode());
  java_config.PushString(kTapSlop, config->GetTapSlop());
  java_config.PushBoolean(kDefaultOverflowVisible,
                          config->GetDefaultOverflowVisible());
  java_config.PushDouble(kEnableLynxScrollFluency,
                         config->GetEnableScrollFluencyMonitor());
  java_config.PushBoolean(kEnableCreateViewAsync,
                          config->GetEnableCreateViewAsync());
  java_config.PushBoolean(kEnableVsyncAlignedFlush,
                          config->GetEnableVsyncAlignedFlush());
  java_config.PushBoolean(kCssAlignWithLegacyW3c,
                          config->GetCSSAlignWithLegacyW3C());
  java_config.PushBoolean(kEnableAccessibilityElement,
                          config->GetEnableAccessibilityElement());
  java_config.PushBoolean(kEnableOverlapForAccessibilityElement,
                          config->GetEnableOverlapForAccessibilityElement());
  java_config.PushBoolean(kEnableNewAccessibility,
                          config->GetEnableNewAccessibility());
  java_config.PushBoolean(kEnableA11yIDMutationObserver,
                          config->GetEnableA11yIDMutationObserver());
  java_config.PushBoolean(kEnableA11y, config->GetEnableA11y());
  java_config.PushString(kReactVersion, config->GetReactVersion());

  java_config.PushBoolean(kEnableTextRefactor, config->GetEnableTextRefactor());
  java_config.PushBoolean(kEnableTextOverflow, config->GetEnableTextOverflow());
  tasm::TernaryBool enable_text_boring_layout =
      config->GetEnableTextBoringLayout();
  if (enable_text_boring_layout != tasm::TernaryBool::UNDEFINE_VALUE) {
    java_config.PushBoolean(
        kEnableTextBoringLayout,
        enable_text_boring_layout == tasm::TernaryBool::TRUE_VALUE);
  }
  java_config.PushBoolean(kEnableNewClipMode, config->GetEnableNewClipMode());
  java_config.PushBoolean(kKeyboardCallbackUseRelativeHeight,
                          config->GetKeyboardCallbackUseRelativeHeight());

  java_config.PushBoolean(kEnableCSSParser, config->GetEnableCSSParser());
  java_config.PushBoolean(kEnableEventRefactor,
                          config->GetEnableEventRefactor());
  java_config.PushBoolean(kEnableDisexposureWhenLynxHidden,
                          config->GetEnableDisexposureWhenLynxHidden());
  java_config.PushBoolean(kEnableExposureWhenLayout,
                          config->GetEnableExposureWhenLayout());
  java_config.PushBoolean(kEnableNewIntersectionObserver,
                          config->GetEnableNewIntersectionObserver());
  java_config.PushInt(kObserverFrameRate, config->GetObserverFrameRate());
  java_config.PushBoolean(kEnableExposureUIMargin,
                          config->GetEnableExposureUIMargin());
  java_config.PushInt(kLongPressDuration, config->GetLongPressDuration());
  java_config.PushInt(kMapContainerType, config->GetMapContainerType());
  if (config->GetIncludeFontPadding() != 0) {
    java_config.PushBoolean(kIncludeFontPadding,
                            config->GetIncludeFontPadding() == 1);
  } else {
    // for history reason
    java_config.PushBoolean(
        kIncludeFontPadding,
        version >= LYNX_VERSION_2_4 && version < LYNX_VERSION_2_9);
  }
  java_config.PushBoolean(kPageFlatten, config->GetGlobalFlattern());
  java_config.PushBoolean(kEnableFlattenTranslateZ,
                          version >= LYNX_VERSION_2_5);
  java_config.PushString(kAbSettingDisableCSSLazyDecode,
                         config->GetAbSettingDisableCSSLazyDecode());
  java_config.PushBoolean(kEnableNewImage, config->GetEnableNewImage());
  java_config.PushBoolean(
      kTrailNewImage,
      config->GetTrailNewImage() == lynx::tasm::TernaryBool::TRUE_VALUE);
  java_config.PushBoolean(kEnableFiber, config->GetEnableFiberArch());
  java_config.PushBoolean(kEnableMultiTouch, config->GetEnableMultiTouch());

  Java_TasmPlatformInvoker_onPageConfigDecoded(
      base::android::AttachCurrentThread(), jni_object_.Get(),
      java_config.jni_object());
}

std::string TasmPlatformInvokerAndroid::TranslateResourceForTheme(
    const std::string& res_id, const std::string& theme_key) {
  if (res_id.empty()) {
    return std::string();
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  auto j_res_id =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, res_id);
  auto j_theme_key =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, theme_key);
  auto j_ret = Java_TasmPlatformInvoker_translateResourceForTheme(
      env, jni_object_.Get(), j_res_id.Get(), j_theme_key.Get());
  return base::android::JNIConvertHelper::ConvertToString(env, j_ret.Get());
}

lepus::Value TasmPlatformInvokerAndroid::TriggerLepusMethod(
    const std::string& method_name, const lepus::Value& args) {
  if (args.IsTable() && args.Table()->size() > 0) {
    JNIEnv* env = base::android::AttachCurrentThread();
    base::android::JavaOnlyMap jni_hashmap =
        tasm::android::ValueConverterAndroid::ConvertLepusToJavaOnlyMap(args);
    auto java_method = base::android::JNIConvertHelper::ConvertToJNIStringUTF(
        env, method_name);
    base::android::ScopedLocalJavaRef<jobject> j_data =
        Java_TasmPlatformInvoker_triggerLepusBridge(env, jni_object_.Get(),
                                                    java_method.Get(),
                                                    jni_hashmap.jni_object());
    if (j_data.Get()) {
      jlong length = env->GetDirectBufferCapacity(j_data.Get());
      if (length > 0) {
        return ConvertJavaData(env, j_data.Get(), length);
      }
    }
  }
  return lepus::Value();
}

void TasmPlatformInvokerAndroid::TriggerLepusMethodAsync(
    const std::string& method_name, const lepus::Value& args) {
  if (args.IsTable() && args.Table()->size() > 0) {
    JNIEnv* env = base::android::AttachCurrentThread();
    base::android::JavaOnlyMap jni_hashmap =
        tasm::android::ValueConverterAndroid::ConvertLepusToJavaOnlyMap(args);
    auto java_method = base::android::JNIConvertHelper::ConvertToJNIStringUTF(
        env, method_name);
    Java_TasmPlatformInvoker_triggerLepusBridgeAsync(
        env, jni_object_.Get(), java_method.Get(), jni_hashmap.jni_object());
  }
}

void TasmPlatformInvokerAndroid::GetI18nResource(
    const std::string& channel, const std::string& fallback_url) {
  JNIEnv* env = base::android::AttachCurrentThread();
  auto java_channel =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, channel);
  auto java_fallback_url =
      base::android::JNIConvertHelper::ConvertToJNIStringUTF(env, fallback_url);
  Java_TasmPlatformInvoker_getI18nResourceByNative(
      env, jni_object_.Get(), java_channel.Get(), java_fallback_url.Get());
}

}  // namespace shell
}  // namespace lynx
