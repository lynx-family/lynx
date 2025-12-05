// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

#import <Lynx/LynxPerformance.h>
#import <Lynx/LynxTemplateBundle.h>
#import <Lynx/LynxViewClientV2.h>

#if TARGET_OS_IOS
#import <Lynx/LynxEventDetail.h>
#import <Lynx/LynxImageFetcher.h>

#import <Lynx/LynxResourceFetcher.h>
#import <Lynx/LynxScrollListener.h>
#else
@class LynxEventDetail;
#endif

@class LynxView;
@class LynxModule;
@class LynxConfigInfo;

@protocol LynxTimingListener <NSObject>

@optional
- (void)lynxView:(LynxView *)lynxView onSetup:(NSDictionary *)info;
- (void)lynxView:(LynxView *)lynxView
        onUpdate:(NSDictionary *)info
          timing:(NSDictionary *)updateTiming;

@end

@protocol LynxJSBTimingListener <NSObject>

@optional
- (void)lynxView:(LynxView *)lynxView onCallJSBFinished:(NSDictionary *)info;
- (void)lynxView:(LynxView *)lynxView onJSBInvoked:(NSDictionary *)jsbInfo;

@end

/**
 * @apidoc
 * @brief A client that provides callbacks for `LynxView`'s lifecycle and other events.
 */
@protocol
    LynxViewLifecycle <NSObject, LynxTimingListener, LynxJSBTimingListener, LynxViewBaseLifecycle>

@optional

+ (void)lynxView:(LynxView *)lynxView
    reportResourceInfo:(NSDictionary *)info
             eventType:(NSString *)eventType;

// issue: #1510
/**
 * @apidoc
 * @brief Called when module method invocation completed.
 * @param view The LynxView instance.
 * @param method Method name.
 * @param module Module name.
 * @param code Error code if module method invocation failed.
 */
- (void)lynxView:(LynxView *)view
    didInvokeMethod:(NSString *)method
           inModule:(NSString *)module
          errorCode:(int)code;

/**
 * @apidoc
 * @brief Called when page start loading.
 * @param view The LynxView instance.
 * @note This method is called once for each content loading request.
 */
- (void)lynxViewDidStartLoading:(LynxView *)view;

/**
 * @apidoc
 * @brief Called when page load finish.
 * @param view The LynxView instance.
 * @param url The page URL.
 * @note This method is called once for each content loading request.
 */
- (void)lynxView:(LynxView *)view didLoadFinishedWithUrl:(NSString *)url;

/**
 * Report lynx config info after that content has been successful loaded on LynxView.
 * This method is called once for each load content request.
 *
 * @param view LynxView
 * @param info LynxConfigInfo
 */
- (void)lynxView:(LynxView *)view
    didLoadFinishedWithConfigInfo:(LynxConfigInfo *)info
    __deprecated_msg("This callback will not be invoked, use `didLoadFinishedWithUrl` instead");
/**
 * @apidoc
 * @brief Called when first screen layout completed.
 * @param view The LynxView instance.
 * @note You can get performance during loading process of the LynxView at this time.
 */
- (void)lynxViewDidFirstScreen:(LynxView *)view;

/**
 * @apidoc
 * @brief Called when page update.
 * @param view The LynxView instance.
 */
- (void)lynxViewDidPageUpdate:(LynxView *)view;

/**
 * @apidoc
 * @brief Called when JS environment preparation completed.
 * @param view The LynxView instance.
 */
- (void)lynxViewDidConstructJSRuntime:(LynxView *)view;

/**
 * @apidoc
 * @brief Called when data update, but the view may not be updated.
 * @param view The LynxView instance.
 */
- (void)lynxViewDidUpdate:(LynxView *)view;

- (void)lynxViewDidChangeIntrinsicContentSize:(LynxView *)view;

/**
 * @apidoc
 * @brief Called when native layout finish in ui or most_on_tasm mode, or diff finish in
 * multi_thread mode.
 * @param view The LynxView instance.
 */
- (void)lynxViewOnTasmFinishByNative:(LynxView *)view;

/**
 * Notify the host application of a image request and allow the application
 * to redirect the url and return. If the return value is null, LynxView will
 * continue to load image from the origin url as usual. Otherwise, the redirect
 * url will be used.
 *
 * This method will be called on any thread.
 *
 * The following scheme is supported in LynxView:
 * 1. Http scheme: http or https
 * 2. File scheme: sources in app sandbox
 * Notice: Image in asset catalog is not support now.
 *
 * @param url the url that ready for loading image
 * @return A url string that fit int with the support scheme list or null
 */
- (NSURL *)shouldRedirectImageUrl:(NSURL *)url
    __attribute__((deprecated("Use loadImage:size:completion: to load image.")));

/**
 * The callback is only called when template provider failed to fetch template resource.
 * The callback is deprecated. Developer should use `lynxView:didRecieveError:` with errorCode
 * `LynxErrorCodeTemplateProvider`.
 */
- (void)lynxView:(LynxView *)view
    didLoadFailedWithUrl:(NSString *)url
                   error:(NSError *)error
    __attribute__((deprecated("Use `lynxView:didRecieveError:`.")));

- (void)lynxView:(LynxView *)view didRecieveError:(NSError *)error;

- (void)lynxView:(LynxView *)view didReceiveFirstLoadPerf:(LynxPerformance *)perf;

/**
 * @apidoc
 * @brief Performance data statistics callback after the interface update is completed.
 * @note The timing of the callback is not fixed due to differences in rendering threads, and
 * should not be used as a starting point for any business side. The callback is in the main
 * thread.
 * @param view The LynxView instance.
 */
- (void)lynxView:(LynxView *)view didReceiveUpdatePerf:(LynxPerformance *)perf;

/**
 * Callback for dynamic component performance statistics after the first load or interface update is
 * completed. NOTE: The callback timing is not fixed due to differences in rendering threads and
 * should not be used as a starting point for any business side. The callback is executed on the
 * main thread.
 */
- (void)lynxView:(LynxView *)view
    didReceiveDynamicComponentPerf:(NSDictionary *)perf
    API_DEPRECATED("Will be provided by TimingObserver", ios(6.0, API_TO_BE_DEPRECATED));

/**
 * @apidoc
 * @brief Return the used component tagName.
 * @param view The LynxView instance.
 * @param componentSet The set of component tagName.
 */
- (void)lynxView:(LynxView *)view didReportComponentInfo:(NSSet<NSString *> *)componentSet;

/**
 * @apidoc
 * @brief Report Lynx events that sended to frontend.
 *
 * @param event LynxEvent that will send to frontend, including eventName, lynxview, etc.
 */
- (void)onLynxEvent:(LynxEventDetail *)event;

/**
 * @apidoc
 * @brief Called when JSBridge invoked.
 * @param info Piper's information. `url: NSString`: Lynx's url; `module-name: NSString`:
 * module's name; `method-name: NSString`: method's name; `params: NSArray`: (Optional) other
 * necessary parameters.
 */
- (void)onPiperInvoked:(NSDictionary *)info;

- (void)onPiperResponsed:(NSDictionary *)info;

/**
 * @apidoc
 * @brief Provide a reusable TemplateBundle after template is decode.
 * @note This callback is disabled by default, and you can enable it through the
 * enableRecycleTemplateBundle option in LynxLoadMeta.
 * @param bundle the recycled template bundle, it is nonnull but could be invalid
 */
- (void)onTemplateBundleReady:(LynxTemplateBundle *)bundle;

@end

#if TARGET_OS_IOS
__attribute__((deprecated(
    "lifecycle functions in LynxViewClient will be moved to protocol LynxViewLifeCycle")))
@protocol
    LynxViewClient<LynxImageFetcher, LynxResourceFetcher, LynxViewLifecycle, LynxScrollListener>

@end
#endif
