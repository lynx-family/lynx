// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUIFrame.h>

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxEventHandler+Internal.h>
#import <Lynx/LynxFrameView.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUI+Private.h>
#import <Lynx/LynxUIContext.h>

@interface LynxUIFrame ()
@property(nonatomic, strong, nullable) LynxTemplateData* data;
@property(nonatomic, strong, nullable) LynxTemplateData* globalProps;
@end

@implementation LynxUIFrame

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_UI("frame")
#else
LYNX_REGISTER_UI("frame")
#endif

- (UIView*)createView {
  return [[LynxFrameView alloc] init];
}

- (void)setContext:(LynxUIContext*)context {
  [super setContext:context];
  [[self view] initWithRootView:context.rootView];
}

- (void)onReceiveAppBundle:(LynxTemplateBundle*)bundle {
  // need to establish the parent-child UI relationship before loadBundle currently
  // TODO(hexionghui): fix it later
  [self attachPageUICallback];
  [[self view] setAppBundle:bundle];
}

- (void)updateFrame:(CGRect)frame
            withPadding:(UIEdgeInsets)padding
                 border:(UIEdgeInsets)border
                 margin:(UIEdgeInsets)margin
    withLayoutAnimation:(BOOL)with {
  [super updateFrame:frame
              withPadding:padding
                   border:border
                   margin:margin
      withLayoutAnimation:with];
  [[self view] setFrame:frame];
}

- (void)attachPageUICallback {
  __weak typeof(self) weakSelf = self;
  [self.view setAttachLynxPageUICallback:^(NSObject* _Nonnull __weak ui) {
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (strongSelf.childrenLynxPageUI == nil) {
      strongSelf.childrenLynxPageUI = [NSMutableDictionary new];
    }
    if ([ui isKindOfClass:[LynxRootUI class]]) {
      LynxRootUI* rootUI = (LynxRootUI*)ui;
      strongSelf.childrenLynxPageUI[[NSString stringWithFormat:@"%p", strongSelf]] = rootUI;
      rootUI.parentLynxPageUI = strongSelf.context.rootUI;
      rootUI.view.isChildLynxPage = YES;
      [rootUI.context.eventHandler removeEventGestures];
    }
  }];
}

- (void)propsDidUpdate {
  [super propsDidUpdate];
  [[self view] updateMetaData:self.data globalProps:self.globalProps];
  self.data = nil;
  self.globalProps = nil;
}

// TODO(zhoupeng.z): pass data on native directly
LYNX_PROP_SETTER("data", updateData, NSDictionary*) {
  if (value != nil) {
    [self setData:[[LynxTemplateData alloc] initWithDictionary:value useBoolLiterals:YES]];
  }
}

LYNX_PROP_SETTER("src", setUrl, NSString*) { [[self view] setUrl:value]; }

LYNX_PROP_SETTER("global-props", updateGlobalProps, NSDictionary*) {
  if (value != nil) {
    [self setGlobalProps:[[LynxTemplateData alloc] initWithDictionary:value useBoolLiterals:YES]];
  }
}
@end
