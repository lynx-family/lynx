// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxExtensionModule.h>
#if TARGET_OS_IPHONE
#import <Lynx/LynxFrameView.h>
#import <Lynx/LynxFrameViewProvider.h>
#import <Lynx/LynxKeyboardEventDispatcher.h>
#endif
#import <Lynx/LynxLog.h>
#import <Lynx/LynxSubErrorCode.h>
#import <Lynx/LynxView+Internal.h>
#import "JSModule+Internal.h"
#import "LynxContext+Internal.h"

NSString *const kDefaultComponentID = @"-1";

@interface LynxContext ()

@property(nonatomic, strong)
    NSMutableDictionary<NSString *, id<LynxExtensionModule>> *extentionModules;
@property(nonatomic, assign) LynxEmbeddedMode lynxEmbeddedMode;

@end

@implementation LynxContext

- (instancetype)initWithContainerView:(id<LUIBodyView>)containerView {
  if (self = [super init]) {
    _containerView = containerView;
    _instanceId = -1;
    _extentionModules = [[NSMutableDictionary alloc] init];
  }
  return self;
}

- (void)setJSProxy:(const std::shared_ptr<lynx::shell::JSProxyDarwin> &)proxy {
  proxy_ = proxy;
}

- (void)setLayoutProxy:(const std::shared_ptr<lynx::shell::LynxLayoutProxyDarwin> &)layout_proxy {
  layout_proxy_ = layout_proxy;
}

- (void)sendGlobalEvent:(nonnull NSString *)name withParams:(nullable NSArray *)params {
  auto eventEmitter = [self getJSModule:@"GlobalEventEmitter"];
  NSMutableArray *args = [[NSMutableArray alloc] init];
  // if name is nil, it will crash. To avoid crash, let name be @"";
  if (name == nil) {
    _LogW(@"Lynx sendGlobalEvent warning: name is nil");
    [args addObject:@""];
  } else {
    [args addObject:name];
  }
  // if params is nil, it will crash. To avoid crash, let params be [];
  if (params == nil) {
    _LogW(@"Lynx sendGlobalEvent warning: params is nil");
    [args addObject:[[NSArray alloc] init]];
  } else {
    [args addObject:params];
  }
  [eventEmitter fire:@"emit" withParams:args];
}

- (nullable JSModule *)getJSModule:(nonnull NSString *)name {
  auto module = [[JSModule alloc] initWithModuleName:name];
  [module setJSProxy:proxy_];
  return module;
}

- (NSNumber *)getLynxRuntimeId {
  if (proxy_) {
    return [NSNumber numberWithLongLong:proxy_->GetId()];
  }
  return @(-1);
}

// issue: #1510
- (void)reportModuleCustomError:(NSString *)message {
  [self reportLynxError:[LynxError lynxErrorWithCode:ECLynxNativeModulesCustomError
                                         description:message]];
}

- (void)reportLynxError:(LynxError *)error {
  [_containerView reportLynxError:error];
}

- (nullable LynxView *)getLynxView {
  if ([_containerView isKindOfClass:[LynxView class]]) {
    return (LynxView *)_containerView;
#if TARGET_OS_IPHONE
  } else if ([_containerView isKindOfClass:[LynxFrameView class]]) {
    return (LynxView *)[(LynxFrameView *)_containerView getRootView];
#endif
  }
  return nil;
}

- (void)dealloc {
  _LogI(@"LynxContext destroy: %p", self);
}

- (void)runOnTasmThread:(dispatch_block_t)task {
  [_containerView runOnTasmThread:task];
}

- (void)runOnJSThread:(dispatch_block_t)task {
  if (task && proxy_) {
    proxy_->RunOnJSThread(task);
  }
}

- (void)runOnLayoutThread:(dispatch_block_t)task {
  if (task) {
    if (layout_proxy_) {
      layout_proxy_->RunOnLayoutThread(task);
    } else {
      // TODO(chennengshi): We shuld remove this and set custom layoutProxy to LynxContext when
      // layout_proxy.h become a public header.
      task();
    }
  }
}

- (void)setExtensionModule:(nonnull id<LynxExtensionModule>)extensionModule
                    forKey:(nonnull NSString *)key {
  [_extentionModules setValue:extensionModule forKey:key];
}

- (nonnull id<LynxExtensionModule>)getExtensionModuleByKey:(nonnull NSString *)key {
  return [_extentionModules valueForKey:key];
}

- (void)setHasCustomGenericFetcher:(BOOL)hasCustomGenericFetcher {
  _LogI(@"setHasCustomGenericFetcher: %d", hasCustomGenericFetcher);
  _hasCustomGenericFetcher = hasCustomGenericFetcher;
}

- (void)setHasCustomMediaFetcher:(BOOL)hasCustomMediaFetcher {
  _LogI(@"setHasCustomMediaFetcher: %d", hasCustomMediaFetcher);
  _hasCustomMediaFetcher = hasCustomMediaFetcher;
}

- (void)setHasCustomTemplateFetcher:(BOOL)hasCustomTemplateFetcher {
  _LogI(@"setHasCustomImageFetcher: %d", hasCustomTemplateFetcher);
  _hasCustomTemplateFetcher = hasCustomTemplateFetcher;
}

- (void)addKeyboardEventObserver:(id)observer {
#if TARGET_OS_IPHONE
  [_keyboardEventDispatcher addKeyboardEventObserver:observer];
#endif
}

- (void)setEmbeddedMode:(LynxEmbeddedMode)mode {
  self.lynxEmbeddedMode = mode;
}

- (bool)isEmbeddedModeOn {
  return (self.lynxEmbeddedMode & LynxEmbeddedModeBase) > 0;
}

- (bool)isFragmentLayerRenderOn {
  return (self.lynxEmbeddedMode & LynxEmbeddedModeFragmentLayerRender) > 0;
}

- (bool)isLayoutInElementModeOn {
  return (self.lynxEmbeddedMode & LynxEmbeddedModeLayoutInElement) > 0;
}

- (bool)isTextServiceModeOn {
  return (self.lynxEmbeddedMode & LynxEmbeddedModeUseTextService) > 0;
}

@end
