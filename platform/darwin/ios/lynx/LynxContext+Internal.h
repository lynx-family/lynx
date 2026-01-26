// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxContext.h>
#import <Lynx/LynxError.h>
#import <Lynx/LynxPerformanceController.h>
#import <Lynx/LynxProviderRegistry.h>
#import <Lynx/LynxView.h>
#import <Lynx/LynxViewEnum.h>
#include "core/shell/ios/js_proxy_darwin.h"
#include "core/shell/ios/lynx_layout_proxy_darwin.h"

@class LynxUIIntersectionObserverManager;
@class LynxUIOwner;
@class LynxKeyboardEventDispatcher;

@interface LynxContext () {
 @public
  std::shared_ptr<lynx::shell::JSProxyDarwin> proxy_;
  std::shared_ptr<lynx::shell::LynxLayoutProxyDarwin> layout_proxy_;
}

@property(nonatomic, weak) LynxUIOwner* _Nullable uiOwner;
@property(nonatomic, weak) LynxUIIntersectionObserverManager* _Nullable intersectionManager;
@property(nonatomic, weak) id<LUIBodyView> _Nullable containerView;
@property(nonatomic, weak) LynxKeyboardEventDispatcher* _Nullable keyboardEventDispatcher;
@property(nonatomic, weak) LynxPerformanceController* _Nullable perfController;
@property(nonatomic, readonly) bool isFragmentLayerRenderOn;
@property(nonatomic, readonly) bool isLayoutInElementModeOn;
@property(nonatomic, readonly) bool isTextServiceModeOn;

// Generated in the LynxShell, id of template instance.
// instanceId is a value greater than or equal to 0, the initial value is -1.
@property(nonatomic, assign) int32_t instanceId;

- (nonnull instancetype)initWithContainerView:(id<LUIBodyView> _Nullable)containerView;
- (void)setJSProxy:(const std::shared_ptr<lynx::shell::JSProxyDarwin>&)proxy;
- (void)setLayoutProxy:(const std::shared_ptr<lynx::shell::LynxLayoutProxyDarwin>&)layout_proxy;

- (nullable NSDictionary*)extentionModules;

@end
