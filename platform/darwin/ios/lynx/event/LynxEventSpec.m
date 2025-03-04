// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxEventSpec.h"

@implementation LynxEventSpec

- (instancetype)initWithRawEvent:(NSString*)event withJSEvent:(BOOL)isJSEvent {
  self = [super init];
  if (self) {
    if ([event characterAtIndex:event.length - 1] != ')') {
      _name = event;
    } else {
      // Get event name
      NSUInteger leftBracketLocation = [event rangeOfString:@"("].location;
      _name = [event substringToIndex:leftBracketLocation];
      // Find type
      NSRange typeSearchRange =
          NSMakeRange(leftBracketLocation, event.length - leftBracketLocation);
      void (^block)(BOOL*, BOOL*, BOOL) = ^(BOOL* prop, BOOL* lepusProp, BOOL isJSEvent) {
        if (isJSEvent) {
          *prop = YES;
        } else {
          *lepusProp = YES;
        }
      };
      if ([event rangeOfString:@"bindEvent" options:NSLiteralSearch range:typeSearchRange]
              .location != NSNotFound) {
        block(&_interestedInBubble, &_interestedInLepusBubble, isJSEvent);
      } else if ([event rangeOfString:@"catchEvent" options:NSLiteralSearch range:typeSearchRange]
                     .location != NSNotFound) {
        block(&_interestedInCatch, &_interestedInLepusCatch, isJSEvent);
      } else if ([event rangeOfString:@"capture-bind" options:NSLiteralSearch range:typeSearchRange]
                     .location != NSNotFound) {
        block(&_shouldCaptureBubble, &_shouldLepusCaptureCatch, isJSEvent);
      } else if ([event rangeOfString:@"capture-catch"
                              options:NSLiteralSearch
                                range:typeSearchRange]
                     .location != NSNotFound) {
        block(&_shouldCaptureCatch, &_shouldLepusCaptureBubble, isJSEvent);
      }
    }
  }
  return self;
}

- (void)composeWithOtherSpec:(LynxEventSpec*)spec {
  if ([spec shouldCaptureCatch]) {
    _shouldCaptureCatch = YES;
  }
  if ([spec shouldCaptureBubble]) {
    _shouldCaptureBubble = YES;
  }
  if ([spec interestedInCatch]) {
    _interestedInCatch = YES;
  }
  if ([spec interestedInBubble]) {
    _interestedInBubble = YES;
  }
  _shouldLepusCaptureCatch |= [spec shouldLepusCaptureCatch];
  _shouldLepusCaptureBubble |= [spec shouldLepusCaptureBubble];
  _interestedInLepusCatch |= [spec interestedInLepusCatch];
  _interestedInLepusBubble |= [spec interestedInLepusBubble];
}

+ (nullable NSDictionary<NSString*, LynxEventSpec*>*)
     convertRawEvents:(nullable NSSet<NSString*>*)set
    andRwaLepusEvents:(nullable NSSet<NSString*>*)lepusSet {
  if (set == nil && lepusSet == nil) {
    return nil;
  }
  NSMutableDictionary* eventSet = [NSMutableDictionary new];
  for (NSString* event in set) {
    LynxEventSpec* eventSpec = [[LynxEventSpec alloc] initWithRawEvent:event withJSEvent:YES];
    LynxEventSpec* originEventSpec = [eventSet objectForKey:eventSpec.name];
    if (originEventSpec) {
      [originEventSpec composeWithOtherSpec:eventSpec];
    } else {
      [eventSet setValue:eventSpec forKey:eventSpec.name];
    }
  }
  for (NSString* event in lepusSet) {
    LynxEventSpec* eventSpec = [[LynxEventSpec alloc] initWithRawEvent:event withJSEvent:NO];
    LynxEventSpec* originEventSpec = [eventSet objectForKey:eventSpec.name];
    if (originEventSpec) {
      [originEventSpec composeWithOtherSpec:eventSpec];
    } else {
      [eventSet setValue:eventSpec forKey:eventSpec.name];
    }
  }
  return eventSet;
}

@end
