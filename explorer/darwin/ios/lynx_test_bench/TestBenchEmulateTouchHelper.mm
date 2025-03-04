// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "TestBenchEmulateTouchHelper.h"

#import <Lynx/LynxTouchEvent.h>
#import <LynxDevtool/LynxUIEvent+EmulateEvent.h>
#import <LynxDevtool/LynxUITouch+EmulateTouch.h>
#include <string>

#pragma mark - TestBenchEmulateTouchHelper
@implementation TestBenchEmulateTouchHelper {
}

+ (void)emulateTouch:(NSDictionary*)dict {
  static NSString* kUIEvent = @"UIEvent";
  static NSString* kTimeStamp = @"timestamp";
  static NSString* kAllTouches = @"allTouches";
  static NSString* kUniqueID = @"uniqueID";
  static NSString* kTouchPhase = @"phase";
  static NSString* kTapCount = @"tapCount";
  static NSString* kLocationInLynxViewX = @"locationInLynxViewX";
  static NSString* kLocationInLynxViewY = @"locationInLynxViewY";
  static NSString* kLynxView = @"LynxView";
  static NSMutableDictionary<NSString*, UITouch*>* uidDict = [NSMutableDictionary new];

  NSDictionary* event = [dict objectForKey:kUIEvent];
  if (event == nil) {
    return;
  }

  LynxView* lynxView = [dict objectForKey:kLynxView];
  if (lynxView == nil) {
    return;
  }

  // remove old touch
  UIEvent* event_obj = [[UIApplication sharedApplication] _touchesEvent];
  [event_obj _clearTouches];
  NSTimeInterval timeStamp = [[event objectForKey:kTimeStamp] doubleValue];
  [event_obj _setTimestamp:timeStamp];

  // add new touch or keep old touch
  NSArray* ary = [event objectForKey:kAllTouches];
  NSMutableDictionary<NSString*, UITouch*>* newUidDict = [NSMutableDictionary new];
  [ary enumerateObjectsUsingBlock:^(id _Nonnull obj, NSUInteger idx, BOOL* _Nonnull stop) {
    NSString* uniqueID = [NSString
        stringWithFormat:@"%@,%@", [obj objectForKey:kUniqueID], [@(lynxView.hash) stringValue]];
    NSTimeInterval t = [[obj objectForKey:kTimeStamp] doubleValue];
    NSInteger phase = [[obj objectForKey:kTouchPhase] integerValue];
    NSUInteger tapCount = [[obj objectForKey:kTapCount] unsignedIntegerValue];
    double x = [[obj objectForKey:kLocationInLynxViewX] doubleValue];
    double y = [[obj objectForKey:kLocationInLynxViewY] doubleValue];
    CGPoint viewPoint = CGPointMake(x, y);
    CGPoint windowPoint = [lynxView convertPoint:viewPoint toView:lynxView.window];
    UITouch* touch = [uidDict objectForKey:uniqueID];
    if (touch == nil) {
      touch = [[UITouch alloc] initInView:lynxView
                              coordinateX:windowPoint.x
                              coordinateY:windowPoint.y];
    } else {
      [touch setWindow:lynxView.window];
      [touch _setLocationInWindow:windowPoint resetPrevious:NO];
      UIView* target = [lynxView.window hitTest:windowPoint withEvent:nil];
      [touch setView:target];
    }
    [touch setTimestamp:t];
    [touch setPhase:UITouchPhase(phase)];
    [touch setTapCount:tapCount];
    [event_obj _addTouch:touch forDelayedDelivery:NO];
    [newUidDict setObject:touch forKey:uniqueID];
  }];

  // record touch in event
  [uidDict removeAllObjects];
  uidDict = newUidDict;

  [[UIApplication sharedApplication] sendEvent:event_obj];
}

@end
