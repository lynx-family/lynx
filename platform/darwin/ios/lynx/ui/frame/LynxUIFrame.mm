// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUIFrame.h>

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxEventHandler+Internal.h>
#import <Lynx/LynxFrameView.h>
#import <Lynx/LynxFrameViewProvider.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxTemplateData+Converter.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUI+Private.h>
#import <Lynx/LynxUIContext.h>

@class LynxTemplateBundle;

@interface LynxUIFrame () {
  LynxTemplateBundle* _pendingBundle;
  BOOL _isPropsUpdated;
  BOOL _isUrlChanged;
}
@end

static LynxTemplateData* ConsumeFrameTemplateDataHolder(NSInteger value) {
  if (value == 0) {
    return nil;
  }
  auto* native_template_data = reinterpret_cast<std::shared_ptr<lynx::tasm::TemplateData>*>(value);
  LynxTemplateData* template_data =
      native_template_data
          ? [[LynxTemplateData alloc] initWithNativeTemplateData:*native_template_data]
          : nil;
  delete native_template_data;
  return template_data;
}

static CGFloat ConvertPresetLengthToPt(LynxUIFrame* ui, NSString* value) {
  if (value == nil) {
    return 0;
  }
  return [ui toPtWithUnitValue:value fontSize:0];
}

@implementation LynxUIFrame

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_UI("frame")
#else
LYNX_REGISTER_UI("frame")
#endif

- (UIView*)createView {
  id<LynxFrameViewProvider> provider = self.context.lynxFrameViewProvider;
  if (provider) {
    LynxFrameView* view = [provider getLynxFrameView:self.context];
    if (view) {
      return view;
    }
  }
  return [[LynxFrameView alloc] init];
}

- (void)setSign:(NSInteger)sign {
  [super setSign:sign];
  [[self view] setSign:sign];
}

- (void)setContext:(LynxUIContext*)context {
  [super setContext:context];
  [[self view] initWithRootView:context.rootView];
  [[self view] setContext:self.context];
}

- (void)onReceiveAppBundle:(LynxTemplateBundle*)bundle {
  if (_isUrlChanged && _isPropsUpdated && [self loadBundle:bundle]) {
    _pendingBundle = nil;
    _isUrlChanged = NO;
    _isPropsUpdated = NO;
  } else {
    _pendingBundle = bundle;
  }
}

- (BOOL)loadBundle:(LynxTemplateBundle*)bundle {
  LynxFrameView* view = [self view];
  if (view == nil) {
    return NO;
  }
  // need to establish the parent-child UI relationship before loadBundle currently
  // TODO(hexionghui): fix it later
  [self attachPageUICallback];
  return [view setAppBundle:bundle];
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

  UIEdgeInsets inset =
      UIEdgeInsetsMake(padding.top + border.top, padding.left + border.left,
                       padding.bottom + border.bottom, padding.right + border.right);
  CGRect contentFrame = UIEdgeInsetsInsetRect(frame, inset);
  [[self view] updateFrame:frame contentFrame:contentFrame];
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
  if (_isUrlChanged) {
    if (_pendingBundle && [self loadBundle:_pendingBundle]) {
      _pendingBundle = nil;
      _isUrlChanged = NO;
    }
  }
  _isPropsUpdated = YES;

  [[self view] propsDidUpdate];
}

LYNX_PROP_SETTER("data", updateData, NSInteger) {
  [[self view] setInitData:requestReset ? nil : ConsumeFrameTemplateDataHolder(value)];
}

LYNX_PROP_SETTER("src", setUrl, NSString*) {
  _isUrlChanged = YES;
  _isPropsUpdated = NO;
  [[self view] setUrl:value];
}

LYNX_PROP_SETTER("global-props", updateGlobalProps, NSInteger) {
  [[self view] setGlobalProps:requestReset ? nil : ConsumeFrameTemplateDataHolder(value)];
}

LYNX_PROP_SETTER("embedded-mode", setEmbeddedMode, NSNumber*) {
  [[self view] setEmbeddedMode:value.intValue];
}

LYNX_PROP_SETTER("auto-width", setAutoWidth, BOOL) {
  [[self view] setAutoWidth:requestReset ? NO : value];
}

LYNX_PROP_SETTER("auto-height", setAutoHeight, BOOL) {
  [[self view] setAutoHeight:requestReset ? NO : value];
}

LYNX_PROP_SETTER("preset-width", setPresetWidth, NSString*) {
  [[self view] setPresetWidth:requestReset ? -1 : ConvertPresetLengthToPt(self, value)];
}

LYNX_PROP_SETTER("preset-height", setPresetHeight, NSString*) {
  [[self view] setPresetHeight:requestReset ? -1 : ConvertPresetLengthToPt(self, value)];
}

LYNX_PROP_SETTER("enable-multi-async-thread", setEnableMultiAsyncThread, NSNumber*) {
  [[self view] setEnableMultiAsyncThread:requestReset ? nil : value];
}
@end
