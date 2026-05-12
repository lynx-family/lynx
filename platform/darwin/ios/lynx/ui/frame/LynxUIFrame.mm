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

@interface LynxFrameView (FrameUICallback)
- (void)setFrameIntrinsicContentSizeChangeCallback:(void (^)(CGSize size))callback;
@end

@interface LynxUIFrame () {
  LynxTemplateBundle* _pendingBundle;
  BOOL _isPropsUpdated;
  BOOL _isUrlChanged;
  BOOL _hasIntrinsicContentSize;
  CGSize _intrinsicContentSize;
}
- (void)frameIntrinsicContentSizeDidChange:(CGSize)size;
@end

static NSString* const kLynxFrameLayoutChangeDetailKeyFrameUrl = @"frameUrl";
static NSString* const kLynxFrameLayoutChangeDetailKeyIntrinsicWidth = @"intrinsicWidth";
static NSString* const kLynxFrameLayoutChangeDetailKeyIntrinsicHeight = @"intrinsicHeight";
static NSString* const kLynxFramePropNameData = @"data";
static NSString* const kLynxFramePropNameGlobalProps = @"global-props";

static LynxTemplateData* ConsumeFrameLepusValuePtr(NSInteger value) {
  if (value == 0) {
    return nil;
  }
  auto* lepus_value = reinterpret_cast<lynx::lepus::Value*>(value);
  LynxTemplateData* template_data =
      lepus_value ? [[LynxTemplateData alloc] initWithLepusValue:*lepus_value] : nil;
  delete lepus_value;
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
  LynxFrameView* view = nil;
  if (provider) {
    view = [provider getLynxFrameView:self.context];
  }
  if (view == nil) {
    view = [[LynxFrameView alloc] init];
  }

  __weak typeof(self) weakSelf = self;
  [view setFrameIntrinsicContentSizeChangeCallback:^(CGSize size) {
    __strong typeof(weakSelf) strongSelf = weakSelf;
    [strongSelf frameIntrinsicContentSizeDidChange:size];
  }];

  return view;
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

- (NSDictionary*)buildLayoutChangeEventDetail {
  NSMutableDictionary* data = [[super buildLayoutChangeEventDetail] mutableCopy];
  data[kLynxFrameLayoutChangeDetailKeyFrameUrl] = self.view.url ?: @"";
  if (_hasIntrinsicContentSize) {
    data[kLynxFrameLayoutChangeDetailKeyIntrinsicWidth] = @(_intrinsicContentSize.width);
    data[kLynxFrameLayoutChangeDetailKeyIntrinsicHeight] = @(_intrinsicContentSize.height);
  }
  return data;
}

- (void)frameIntrinsicContentSizeDidChange:(CGSize)size {
  _hasIntrinsicContentSize = YES;
  _intrinsicContentSize = size;
  [self sendLayoutChangeEvent];
}

- (void)resetIntrinsicContentSize {
  _hasIntrinsicContentSize = NO;
  _intrinsicContentSize = CGSizeZero;
}

- (void)propsDidUpdate {
  [super propsDidUpdate];
  [self.lynxProps removeObjectForKey:kLynxFramePropNameData];
  [self.lynxProps removeObjectForKey:kLynxFramePropNameGlobalProps];
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
  [[self view] setInitData:requestReset ? nil : ConsumeFrameLepusValuePtr(value)];
}

LYNX_PROP_SETTER("src", setUrl, NSString*) {
  _isUrlChanged = YES;
  _isPropsUpdated = NO;
  [self resetIntrinsicContentSize];
  [[self view] setUrl:value];
}

LYNX_PROP_SETTER("global-props", updateGlobalProps, NSInteger) {
  [[self view] setGlobalProps:requestReset ? nil : ConsumeFrameLepusValuePtr(value)];
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
