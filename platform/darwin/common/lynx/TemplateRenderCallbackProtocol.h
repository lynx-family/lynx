// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_TEMPLATERENDERCALLBACKPROTOCOL_H_
#define DARWIN_COMMON_LYNX_TEMPLATERENDERCALLBACKPROTOCOL_H_

#import <Foundation/Foundation.h>
#import "LynxErrorReceiverProtocol.h"
#import "LynxTemplateBundle.h"
#include "core/renderer/page_config.h"

namespace lynx {
namespace tasm {
class TemplateData;
}
}  // namespace lynx

/**
 * This protocol is implemented by LynxTemplateRender
 */

NS_ASSUME_NONNULL_BEGIN

@class LynxTheme;
@class LynxContext;

@protocol TemplateRenderCallbackProtocol <LynxErrorReceiverProtocol>

@required
/**
 * Notify that data has been updated after updating data on LynxView,
 * but the view may not be updated.
 */
- (void)onDataUpdated;

/**
 * Notify that page has been changed.
 */
- (void)onPageChanged:(BOOL)isFirstScreen;
/**
 * Notify that tasm has finished.
 */
- (void)onTasmFinishByNative;
/**
 * Notify that content has been successful loaded. This method
 * is called once for each load content request.
 */
- (void)onTemplateLoaded:(NSString *)url;
/**
 * Notify the JS Runtime is  ready.
 */
- (void)onRuntimeReady;

/// Deprecated: prefer onErrorOccurred with parameter LynxError.
/// TODO(yanghuiwen):Old interface will be removed in version 2.12
- (void)onErrorOccurred:(NSInteger)code message:(NSString *)errMessage;
/**
 * Dispatch module method request to LynxTemplateRender
 * @param method method name
 * @param module module name
 * @param code error code
 */
- (void)didInvokeMethod:(NSString *)method inModule:(NSString *)module errorCode:(int32_t)code;

- (void)onTimingSetup:(NSDictionary *)timingInfo;

- (void)onTimingUpdate:(NSDictionary *)timingInfo updateTiming:(NSDictionary *)updateTiming;

- (void)onPerformanceEvent:(NSDictionary *)originDict;
/**
 * Notify the performance data statistics after the first load is completed
 */
- (void)onFirstLoadPerf:(NSDictionary *)perf;
/**
 * Notify the performance statistics after page update
 */
- (void)onUpdatePerfReady:(NSDictionary *)perf;
/**
 * Notify the performance statistics after dynamic component is loaded or updated
 */
- (void)onDynamicComponentPerf:(NSDictionary *)perf;

- (void)setPageConfig:(const std::shared_ptr<lynx::tasm::PageConfig> &)pageConfig;
- (void)setTiming:(uint64_t)timestamp
              key:(NSString *)key
       pipelineID:(nullable NSString *)pipelineID;

/**
 * Get translated resource
 */
- (NSString *)translatedResourceWithId:(NSString *)resId themeKey:(NSString *)key;
/**
 * Get internationalization resource
 * @param channel channel
 * @param url fallback url
 */
- (void)getI18nResourceForChannel:(NSString *)channel withFallbackUrl:(NSString *)url;

/**
 * Asynchronous trigger lepus bridge to invoke function from event handler
 * @param data function param
 * @param callbackID callback id
 */
- (void)invokeLepusFunc:(NSDictionary *)data callbackID:(int32_t)callbackID;

- (void)onCallJSBFinished:(NSDictionary *)info;
- (void)onJSBInvoked:(NSDictionary *)info;

/**
 * Notify lynx to receive message event from lepus or js context.
 * @param event the message event
 */
- (void)onReceiveMessageEvent:(NSDictionary *)event;

/**
 * Performance data
 */
- (long)initStartTiming;
- (long)initEndTiming;

@optional
- (BOOL)enableAirStrictMode;

- (void)invokeUIMethod:(NSString *_Nonnull)method_string
                params:(NSDictionary *_Nonnull)params
              callback:(int)callback
                toNode:(int)node;

/**
 * Notify that SSR page has hydrated successfully . This method
 * is called once for each load content request.
 */
- (void)onSSRHydrateFinished:(NSString *)url;

- (void)onTemplateBundleReady:(LynxTemplateBundle *)bundle;

- (void)setLocalTheme:(LynxTheme *)theme;

- (LynxContext *)getLynxContext;

- (NSMutableDictionary<NSString *, id> *)getLepusModulesClasses;

- (void)onReloadTemplate:(const std::vector<uint8_t> &)data
                 withURL:(const std::string &)url
                initData:(const std::shared_ptr<lynx::tasm::TemplateData> &)init_data;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_TEMPLATERENDERCALLBACKPROTOCOL_H_
