// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxBaseInspectorOwnerNG.h>
#import <Lynx/LynxPageReloadHelper.h>
#import <Lynx/LynxView+Internal.h>

NS_ASSUME_NONNULL_BEGIN

@protocol GlobalPropsUpdatedObserver <NSObject>

- (void)onGlobalPropsUpdated:(NSDictionary *)props;

@end

@interface LynxInspectorOwner : NSObject <LynxBaseInspectorOwnerNG>

- (instancetype)init;
- (nonnull instancetype)initWithLynxView:(nullable LynxView *)view;
- (void)setReloadHelper:(nullable LynxPageReloadHelper *)reloadHelper;
- (void)call:(NSString *_Nonnull)function withParam:(NSString *_Nullable)params;

- (void)onTemplateAssemblerCreated:(intptr_t)ptr;

- (void)onLoadFinished;

- (void)reloadLynxView:(BOOL)ignoreCache;
- (void)reloadLynxView:(BOOL)ignoreCache
          withTemplate:(nullable NSString *)templateBin
         fromFragments:(BOOL)fromFragments
              withSize:(int32_t)size;
- (void)onReceiveTemplateFragment:(nullable NSString *)data withEof:(BOOL)eof;

- (void)navigateLynxView:(nonnull NSString *)url;

- (void)startCasting:(int)quality
               width:(int)maxWidth
              height:(int)maxGeight
                mode:(int)screenshot_mode;
- (void)stopCasting;
- (void)continueCasting;
- (void)pauseCasting;
- (LynxView *)getLynxView;

- (void)handleLongPress;

- (NSInteger)getSessionId;

- (void)setConnectionID:(int)connectionID;

- (NSString *)getTemplateUrl;

- (UIView *)getTemplateView;

- (LynxTemplateData *)getTemplateData;

- (NSString *)getTemplateJsInfo:(uint32_t)offset withSize:(uint32_t)size;

- (BOOL)isDebugging;

- (void)sendConsoleMessage:(NSString *)message
                 withLevel:(int32_t)level
             withTimeStamp:(int64_t)timeStamp;

- (void)attachDebugBridge:(NSString *)url;

- (void)initRecord;

- (void)sendMessage:(CustomizedMessage *)message;

- (void)subscribeMessage:(NSString *)type withHandler:(id<MessageHandler>)handler;

- (void)unsubscribeMessage:(NSString *)type;

- (void)invokeCDPFromSDK:(NSString *)msg withCallback:(CDPResultCallback)callback;

- (int64_t)getRecordID;

- (void)enableRecording:(bool)enable;

- (void)enableTraceMode:(bool)enable;

- (void)onPageUpdate;

- (void)attachLynxUIOwnerToAgent:(nullable LynxUIOwner *)uiOwner;

- (CGPoint)getViewLocationOnScreen;

- (void)dispatchMessageEvent:(NSDictionary *)event;

- (void)setGlobalPropsUpdatedObserver:(id<GlobalPropsUpdatedObserver>)observer;

@end

NS_ASSUME_NONNULL_END
