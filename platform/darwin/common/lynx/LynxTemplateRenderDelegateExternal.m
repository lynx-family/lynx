// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "LynxTemplateRenderDelegateExternal.h"

@interface LynxTemplateRenderDelegateExternal () <LynxTemplateRenderDelegate>

@end

@implementation LynxTemplateRenderDelegateExternal

/**
 * Notify that data has been updated after updating data on LynxView,
 * but the view may not be updated.
 */
- (void)templateRenderOnDataUpdated:(LynxTemplateRender *)templateRender {
}

/**
 * Notify that page has been changed.
 * @param isFirstScreen this change is first screen or not.
 */
- (void)templateRender:(LynxTemplateRender *)templateRender onPageChanged:(BOOL)isFirstScreen {
}

/**
 * Notify that tasm has finished.
 */
- (void)templateRenderOnTasmFinishByNative:(LynxTemplateRender *)templateRender {
}

/**
 * Notify that content has been successful loaded. This method
 * is called once for each load content request.
 * @param url The url of the content
 */
- (void)templateRender:(LynxTemplateRender *)templateRender onTemplateLoaded:(NSString *)url {
}
/**
 * Notify the JS Runtime is  ready.
 */
- (void)templateRenderOnRuntimeReady:(LynxTemplateRender *)templateRender {
}

/**
 * Notify the performance data statistics after the first load is completed
 * @param perf performance data
 */
- (void)templateRender:(LynxTemplateRender *)templateRender
    onReceiveFirstLoadPerf:(LynxPerformance *)perf {
}
/**
 * Notify the performance statistics after page update
 */
- (void)templateRender:(LynxTemplateRender *)templateRender onUpdatePerf:(LynxPerformance *)perf {
}
/**
 * Notify the performance statistics after dynamic component is loaded or updated
 */
- (void)templateRender:(LynxTemplateRender *)templateRender
    onReceiveDynamicComponentPerf:(NSDictionary *)perf {
}
- (NSString *)templateRender:(LynxTemplateRender *)templateRender
    translatedResourceWithId:(NSString *)resId
                    themeKey:(NSString *)key {
  return nil;
}

/**
 * Request to invoke method called from js
 * @param method method name
 * @param module module name
 */
- (void)templateRender:(LynxTemplateRender *)templateRender
       didInvokeMethod:(NSString *)method
              inModule:(NSString *)module
             errorCode:(int)code {
}

/**
 * Dispatch error to delegate
 */
- (void)templateRender:(LynxTemplateRender *)templateRender onErrorOccurred:(LynxError *)error {
}

/**
 * Notify delegate to reset view and layer
 */
- (void)templateRenderOnResetViewAndLayer:(LynxTemplateRender *)templateRender {
}

/**
 * Notify delegate that template start load
 */
- (void)templateRenderOnTemplateStartLoading:(LynxTemplateRender *)templateRender {
}

/**
 * Notify delegate that first screen layout finish
 */
- (void)templateRenderOnFirstScreen:(LynxTemplateRender *)templateRender {
}

/**
 * Notify delegate that page update
 */
- (void)templateRenderOnPageUpdate:(LynxTemplateRender *)templateRender {
}

/**
 * Notify delegate to detach LynxTemplateRender
 */
- (void)templateRenderOnDetach:(LynxTemplateRender *)templateRender {
}

/**
 * JSB invoke
 */
- (void)templateRender:(LynxTemplateRender *)templateRender onCallJSBFinished:(NSDictionary *)info {
}
- (void)templateRender:(LynxTemplateRender *)templateRender onJSBInvoked:(NSDictionary *)info {
}

@end
