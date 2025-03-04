// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxError.h"
#import "LynxPageReloadHelper.h"
#import "LynxTemplateData.h"
#import "LynxTemplateRender.h"
#import "LynxView.h"

// Use system macro in header file to avoid host app can not recognize custom macro
#if TARGET_OS_IOS
#import "LynxBackgroundRuntime.h"
#import "LynxUIOwner.h"
#endif

NS_ASSUME_NONNULL_BEGIN

@interface LynxDevtool : NSObject

@property(nonatomic, readwrite) id<LynxBaseInspectorOwner> owner;

- (nonnull instancetype)initWithLynxView:(LynxView *)view debuggable:(BOOL)debuggable;

- (void)registerModule:(LynxTemplateRender *)render;

- (void)onLoadFromLocalFile:(NSData *)tem withURL:(NSString *)url initData:(LynxTemplateData *)data;

- (void)onLoadFromURL:(NSString *)url initData:(LynxTemplateData *)data postURL:(NSString *)postURL;

- (void)attachDebugBridge:(NSString *)url;

- (void)onLoadFromBundle:(LynxTemplateBundle *)bundle
                 withURL:(NSString *)url
                initData:(LynxTemplateData *)data;

- (void)onStandaloneRuntimeLoadFromURL:(NSString *)url;

#if TARGET_OS_IOS
- (void)onBackgroundRuntimeCreated:(LynxBackgroundRuntime *)runtime
                   groupThreadName:(NSString *)groupThreadName;
#endif

- (void)onTemplateAssemblerCreated:(intptr_t)ptr;

- (void)onEnterForeground;

- (void)onEnterBackground;

- (void)onLoadFinished;

- (void)handleLongPress;

- (void)showErrorMessage:(LynxError *)error;

- (void)attachLynxView:(LynxView *)lynxView;

#if TARGET_OS_IOS
- (void)attachLynxUIOwner:(nullable LynxUIOwner *)uiOwner;
#endif

- (void)setRuntimeId:(NSInteger)runtimeId;

- (void)onMovedToWindow;

- (void)onPageUpdate;

- (void)downloadResource:(NSString *_Nonnull)url callback:(LynxResourceLoadBlock _Nonnull)callback;

- (void)onPerfMetricsEvent:(NSString *_Nonnull)eventName
                  withData:(NSDictionary<NSString *, NSObject *> *_Nonnull)data;

- (NSString *)debugInfoUrl;

- (void)onReceiveMessageEvent:(NSDictionary *)event;

- (void)setDispatchMessageEventBlock:(void (^)(NSDictionary *))block;
@end

NS_ASSUME_NONNULL_END
