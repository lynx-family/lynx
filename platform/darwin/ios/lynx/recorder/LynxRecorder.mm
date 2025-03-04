// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxRecorder.h"

#import <objc/runtime.h>
#include <unordered_map>
#include "core/services/recorder/ios/template_assembler_recorder_darwin.h"

#pragma mark - UIApplication (LynxRecorder)
@interface UIApplication (LynxRecorder)

@end

@implementation UIApplication (LynxRecorder)

// Swizzle UIApplication sendEvent: method to record system events.
- (void)lynx_recorder_sendEvent:(UIEvent *)event {
  [[LynxRecorder sharedInstance] recordUIEvent:event withLynxView:nil];
  [self lynx_recorder_sendEvent:event];
}

@end

#pragma mark - LynxView (LynxRecorder)
@interface LynxView (LynxRecorder)

@end

@implementation LynxView (LynxRecorder)

// Swizzle LynxView hittest:withEvent: method to record system events.
- (nullable UIView *)lynx_recorder_hitTest:(CGPoint)point withEvent:(nullable UIEvent *)event {
  UIView *res = [self lynx_recorder_hitTest:point withEvent:event];
  if (res != nil) {
    [[LynxRecorder sharedInstance] recordUIEvent:event withLynxView:self];
  }
  return res;
}

@end

#pragma mark - LynxRecorder
@implementation LynxRecorder {
  std::unordered_map<NSUInteger, __weak LynxView *> event_lynxview_map_;
  std::unordered_map<NSUInteger, __weak LynxView *> touch_lynxview_map_;
}

+ (void)swizzle:(Class)klass instanceMethod:(SEL)originalSelector with:(SEL)swizzledSelector {
  Method originalMethod = class_getInstanceMethod(klass, originalSelector);
  Method swizzledMethod = class_getInstanceMethod(klass, swizzledSelector);

  BOOL didAddMethod =
      class_addMethod(klass, originalSelector, method_getImplementation(swizzledMethod),
                      method_getTypeEncoding(swizzledMethod));

  if (didAddMethod) {
    class_replaceMethod(klass, swizzledSelector, method_getImplementation(originalMethod),
                        method_getTypeEncoding(originalMethod));
  } else {
    method_exchangeImplementations(originalMethod, swizzledMethod);
  }
}

LYNX_LOAD_LAZY(static dispatch_once_t onceToken;
               void (^swizzleMethod)(NSString *, NSString *, Class) =
                   ^(NSString *origin, NSString *swizzle, Class className) {
                     SEL originalSelector = NSSelectorFromString(origin);
                     SEL swizzledSelector = NSSelectorFromString(swizzle);
                     [self swizzle:className instanceMethod:originalSelector with:swizzledSelector];
                   };
               dispatch_once(&onceToken, ^{
                 swizzleMethod(@"sendEvent:", @"lynx_recorder_sendEvent:", [UIApplication class]);
                 swizzleMethod(
                     @"hitTest:withEvent:", @"lynx_recorder_hitTest:withEvent:", [LynxView class]);
               });)

+ (instancetype)sharedInstance {
  static LynxRecorder *_instance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _instance = [[LynxRecorder alloc] init];
  });
  return _instance;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    event_lynxview_map_ = {};
    touch_lynxview_map_ = {};
  }
  return self;
}

/*
 LynxView hittest:withEvent: (Record UIEvents that LynxView is interested in.)
    |
    |
    |
 UIApplication sendEvent: (Check if UIEvents are of interest to LynxView, and if so, record all
 event.allTouches.)
 */
- (void)recordUIEvent:(nullable UIEvent *)event withLynxView:(nullable LynxView *)lynxView {
  if (lynxView != nil) {
    event_lynxview_map_[event.hash] = lynxView;
    return;
  }
  auto iter = event_lynxview_map_.find(event.hash);
  if (iter != event_lynxview_map_.end()) {
    [event.allTouches enumerateObjectsUsingBlock:^(UITouch *_Nonnull obj, BOOL *_Nonnull stop) {
      if (obj == nil) {
        return;
      }
      touch_lynxview_map_[obj.hash] = iter->second;
    }];
    event_lynxview_map_.erase(iter);
  }

  __block BOOL needRecord = NO;
  __block __weak LynxView *view = nil;
  [event.allTouches enumerateObjectsUsingBlock:^(UITouch *_Nonnull obj, BOOL *_Nonnull stop) {
    if (obj == nil) {
      return;
    }
    auto iiter = touch_lynxview_map_.find(obj.hash);
    if (iiter == touch_lynxview_map_.end()) {
      return;
    }
    view = iiter->second;
    needRecord = YES;
    if (obj.phase == UITouchPhaseEnded || obj.phase == UITouchPhaseCancelled) {
      touch_lynxview_map_.erase(iiter);
    }
  }];

  if (needRecord) {
    lynx::tasm::recorder::TemplateAssemblerRecorderDarwin::RecordPlatformEventDarwin(event, view);
  }
}

@end
