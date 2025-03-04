// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxShadowNode.h"
#import "LynxConverter+LynxCSSType.h"
#import "LynxDefines.h"
#import "LynxPropsProcessor.h"
#import "LynxTemplateRender+Internal.h"

@implementation LynxShadowNodeStyle
@end

@implementation LynxShadowNode {
  BOOL _needsDisplay;
  __weak id<LynxShadowNodeDelegate> _delegate;
  NSRunLoop *_currentLayoutLooper;
}

LYNX_NOT_IMPLEMENTED(-(instancetype)init)

LYNX_PROPS_GROUP_DECLARE(LYNX_PROP_DECLARE("event-through", setEventThrough, BOOL))

- (instancetype)initWithSign:(NSInteger)sign tagName:(NSString *)tagName {
  self = [super initWithSign:sign tagName:tagName];
  if (self) {
    _isDestroy = NO;
    _eventSet = nil;
    _ignoreFocus = kLynxEventPropUndefined;
    _enableTouchPseudoPropagation = YES;
    _eventThrough = kLynxEventPropUndefined;
    _currentLayoutLooper = [NSRunLoop currentRunLoop];
  }
  return self;
}

- (void)setUIOperation:(LynxUIOwner *)owner {
  _uiOwner = owner;
}

- (void)setDelegate:(id<LynxShadowNodeDelegate>)delegate {
  _delegate = delegate;
};

- (void)postExtraDataToUI:(id)value {
  // this method can be removed
}

- (void)postFrameToUI:(CGRect)frame {
  // this method can be removed
}

- (void)setNeedsLayout {
  if (_isDestroy) {
    return;
  }
  // Executing Layout on the wrong thread can cause render exceptions.
  // Add thread checking to fix related issues.
  // TODO(huangweiwu): The judgment method may be modified due to thread model switching
  if ([[_uiOwner templateRender] getThreadStrategyForRender] !=
          LynxThreadStrategyForRenderAllOnUI &&
      [NSThread isMainThread]) {
    if (@available(iOS 10.0, *)) {
      __weak __typeof(self) weakSelf = self;
      [_currentLayoutLooper performBlock:^{
        __strong typeof(weakSelf) strongSelf = weakSelf;
        if (strongSelf) {
          [strongSelf setNeedsLayoutWithThreadCheckFinished];
        }
      }];
      return;
    }
  }
  [self setNeedsLayoutWithThreadCheckFinished];
}

- (void)setNeedsLayoutWithThreadCheckFinished {
  if (_isDestroy) {
    return;
  }
  if (![self isVirtual]) {
    [super setNeedsLayout];
  } else {
    [[self findNonVirtualNode] setNeedsLayout];
  }
}

- (void)internalSetNeedsLayoutForce {
  if (_isDestroy) {
    return;
  }
  if (![self isVirtual]) {
    [super internalSetNeedsLayoutForce];
  } else {
    [[self findNonVirtualNode] internalSetNeedsLayoutForce];
  }
}

- (LynxShadowNode *)findNonVirtualNode {
  if ([self isVirtual]) {
    return [self.parent findNonVirtualNode];
  }
  return self;
}

- (BOOL)isVirtual {
  return NO;
}

LYNX_PROP_SETTER("ignore-focus", setIgnoreFocus, BOOL) {
  // If requestReset, the _ignoreFocus will be Undefined.
  enum LynxEventPropStatus res = kLynxEventPropUndefined;
  if (requestReset) {
    _ignoreFocus = res;
    return;
  }
  _ignoreFocus = value ? kLynxEventPropEnable : kLynxEventPropDisable;
}

LYNX_PROP_SETTER("enable-touch-pseudo-propagation", setEnableTouchPseudoPropagation, BOOL) {
  if (requestReset) {
    value = YES;
  }
  _enableTouchPseudoPropagation = value;
}

LYNX_PROP_DEFINE("event-through", setEventThrough, BOOL) {
  // If requestReset, the _eventThrough will be Undefined.
  enum LynxEventPropStatus res = kLynxEventPropUndefined;
  if (requestReset) {
    _eventThrough = res;
    return;
  }
  _eventThrough = value ? kLynxEventPropEnable : kLynxEventPropDisable;
}

LYNX_PROP_SETTER("vertical-align", setVerticalAlign, NSArray *) {
  // be compatible with old pages
  if (!_uiOwner.uiContext.enableTextRefactor) {
    [self setVerticalAlignOnShadowNode:requestReset value:value];
  }
}

LYNX_PROP_SETTER("dataset", setDataset, NSDictionary *) {
  if (requestReset) {
    value = [NSDictionary dictionary];
  }

  _dataset = value;
}

- (void)setVerticalAlignOnShadowNode:(BOOL)requestReset value:(NSArray *)value {
  if (requestReset) {
    value = nil;
  }

  if (_shadowNodeStyle == nil) {
    _shadowNodeStyle = [LynxShadowNodeStyle new];
  }
  if (value == nil || [value count] < 2) {
    _shadowNodeStyle.valign = LynxVerticalAlignDefault;
  } else {
    _shadowNodeStyle.valign = [LynxConverter toLynxVerticalAlign:[value objectAtIndex:0]];
    _shadowNodeStyle.valignLength = [LynxConverter toCGFloat:[value objectAtIndex:1]];
  }
  [self setNeedsLayout];
}

- (void)destroy {
  _isDestroy = YES;
}

- (id)getExtraBundle {
  return nil;
}

- (BOOL)needsEventSet {
  return NO;
}

- (BOOL)supportInlineView {
  return NO;
}

@end
