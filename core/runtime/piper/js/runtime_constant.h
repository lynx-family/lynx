// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_PIPER_JS_RUNTIME_CONSTANT_H_
#define CORE_RUNTIME_PIPER_JS_RUNTIME_CONSTANT_H_

#include <string>

#include "base/include/string/string_utils.h"

namespace lynx {
namespace runtime {

constexpr const char kAppServiceJSName[] = "/app-service.js";
constexpr const char kLynxCoreJSName[] = "/lynx_core.js";
constexpr const char kLynxAssetsScheme[] = "lynx_assets";
constexpr const char kDynamicComponentJSPrefix[] = "dynamic-component/";
// The name of any js file bundled in a lynx template, except app-service.js,
// will start with `kLynxTemplateAssetsScheme`.
constexpr const char kLynxTemplateAssetsScheme[] = "lynx";

constexpr const char kMessageEventTypeNotifyGlobalPropsUpdated[] =
    "__NotifyGlobalPropsUpdated";

constexpr const char kMessageEventTypeOnNativeAppReady[] = "__OnNativeAppReady";
constexpr const char kMessageEventTypeOnLifecycleEvent[] = "__OnLifecycleEvent";
constexpr const char kMessageEventTypeOnReactComponentRender[] =
    "__OnReactComponentRender";
constexpr const char KMessageEventTypeOnReactCardDidUpdate[] =
    "__OnReactCardDidUpdate";
constexpr const char kMessageEventTypeOnReactCardRender[] =
    "__OnReactCardRender";
constexpr const char kMessageEventTypeOnAppFirstScreen[] = "__OnAppFirstScreen";
constexpr const char kMessageEventTypeOnDynamicJSSourcePrepared[] =
    "__OnDynamicJSSourcePrepared";
constexpr const char kMessageEventTypeOnAppEnterForeground[] =
    "__OnAppEnterForeground";
constexpr const char kMessageEventTypeOnAppEnterBackground[] =
    "__OnAppEnterBackground";
constexpr const char kMessageEventTypeOnComponentActivity[] =
    "__OnComponentActivity";
constexpr const char kMessageEventTypeForceGcJSIObjectWrapper[] =
    "__ForceGcJSIObjectWrapper";
constexpr const char kMessageEventTypeOnSsrScriptReady[] = "__OnSsrScriptReady";
constexpr const char kMessageEventTypeOnReactComponentDidUpdate[] =
    "__OnReactComponentDidUpdate";
constexpr const char kMessageEventTypeOnReactComponentDidCatch[] =
    "__OnReactComponentDidCatch";
constexpr const char kMessageEventTypeOnComponentSelectorChanged[] =
    "__OnComponentSelectorChanged";
constexpr const char kMessageEventTypeOnReactComponentCreated[] =
    "__OnReactComponentCreated";
constexpr const char kMessageEventTypeOnComponentPropertiesChanged[] =
    "__OnComponentPropertiesChanged";
constexpr const char kMessageEventTypeOnComponentDataSetChanged[] =
    "__OnComponentDataSetChanged";
constexpr const char kMessageEventTypeOnReactComponentUnmount[] =
    "__OnReactComponentUnmount";
constexpr const char kMessageEventTypeSendPageEvent[] = "__SendPageEvent";
constexpr const char kMessageEventTypeSendGlobalEvent[] = "__SendGlobalEvent";
constexpr const char kMessageEventTypePublishComponentEvent[] =
    "__PublishComponentEvent";
constexpr const char kMessageEventTypeCallJSFunctionInLepusEvent[] =
    "__CallJSFunctionInLepusEvent";
constexpr const char kMessageEventSetSessionStorageItem[] =
    "__SetSessionStorageItem";
constexpr const char kMessageEventGetSessionStorageItem[] =
    "__GetSessionStorageItem";
constexpr const char kMessageEventSubscribeSessionStorage[] =
    "__SubscribeSessionStorage";
constexpr const char kMessageEventUnSubscribeSessionStorage[] =
    "__UnSubscribeSessionStorage";

/**
 * @name: enableMicrotaskPromisePolyfill
 * @description: Use lynx.queueMicrotask polyfill Promise.
 * @platform: Both
 * @supportVersion: 3.1
 **/
constexpr const char kEnableMicrotaskPromisePolyfill[] =
    "enableMicrotaskPromisePolyfill";

/**
 * Check if the given url indicates lynx_core.js.
 * @param url the url to check
 * @return true if the given url indicates lynx_core.js.
 */
inline bool IsCoreJS(const std::string &url) {
  return !url.compare(kLynxCoreJSName);
}

/**
 * Check if the given url indicates a js file bundled in lynx sdk.
 * @param url the url to check
 * @return true if the given url indicates a js file bundled in lynx sdk.
 */
inline bool IsKernelJs(const std::string &url) { return IsCoreJS(url); }

/**
 * Check if the given url indicates app-service.js.
 * @param url the url to check
 * @return true if the given url indicates app-service.js.
 */
inline bool IsAppServiceJS(const std::string &url) {
  return !url.compare(kAppServiceJSName);
}

/**
 * Check if the given url indicates a js file bundled in lynx sdk.
 * @param url the url to check
 * @return true if the given url indicates a js file bundled in lynx sdk.
 */
inline bool IsLynxAssets(const std::string &url) {
  return base::BeginsWith(url, kLynxAssetsScheme);
}

/**
 * Check if the given url indicates a js file bundled in a lynx template.
 * @param url the url to check
 * @return true if the given url indicates a js file bundled in a lynx template.
 */
inline bool IsLynxTemplateAssets(const std::string &url) {
  return IsAppServiceJS(url) ||
         base::BeginsWith(url, std::string(kLynxTemplateAssetsScheme) + ":");
}

/**
 * Check if the given url indicates a app-service.js of a dynamic component.
 * @param url the url to check
 * @return true if the given url indicates a app-service.js of a dynamic
 * component.
 */
inline bool IsDynamicComponentServiceJS(const std::string &url) {
  return lynx::base::BeginsWith(url, kDynamicComponentJSPrefix) &&
         lynx::base::EndsWith(url, kAppServiceJSName);
}

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_PIPER_JS_RUNTIME_CONSTANT_H_
