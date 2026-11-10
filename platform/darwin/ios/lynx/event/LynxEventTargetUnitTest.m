// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxEventSpec.h>
#import <Lynx/LynxEventTargetSpan.h>
#import <Lynx/LynxShadowNode.h>
#import <Lynx/LynxTouchEvent.h>
#import <Lynx/LynxUI.h>
#import <XCTest/XCTest.h>
#import <objc/runtime.h>

@interface LynxLegacyEventTarget : NSObject <LynxEventTargetBase>
@end

@implementation LynxLegacyEventTarget
- (nullable id<LynxEventTargetBase>)parentResponder {
  return nil;
}

- (nullable NSDictionary*)getDataset {
  return nil;
}
@end

// This event target is private to LynxUIText.m.
@interface LynxTextServiceEventTarget : NSObject <LynxEventTarget>
- (instancetype)initWithSign:(NSInteger)sign eventMask:(NSUInteger)eventMask;
- (void)setParentEventTarget:(id<LynxEventTarget>)parent;
@end

@interface LynxEventTargetUnitTest : XCTestCase
@end

@implementation LynxEventTargetUnitTest

- (LynxUI*)createUI {
  return [[LynxUI alloc] initWithView:[[UIView alloc] initWithFrame:CGRectZero]];
}

- (void)testHasEventListenerIsOptional {
  SEL selector = @selector(hasEventListener:);
  Protocol* protocol = @protocol(LynxEventTargetBase);
  XCTAssertEqual(protocol_getMethodDescription(protocol, selector, NO, YES).name, selector);
  XCTAssertEqual(protocol_getMethodDescription(protocol, selector, YES, YES).name, (SEL)NULL);
}

- (void)testLegacyTargetHasEventListenerFallsBackToNo {
  id<LynxEventTargetBase> target = [[LynxLegacyEventTarget alloc] init];
  XCTAssertFalse([target respondsToSelector:@selector(hasEventListener:)]);

  BOOL hasListener = [target respondsToSelector:@selector(hasEventListener:)] &&
                     [target hasEventListener:LynxEventTap];
  XCTAssertFalse(hasListener);
}

- (void)testGuardedHasEventListenerQueriesSupportedTarget {
  LynxUI* ui = [self createUI];
  [ui setRawEvents:[NSSet setWithObject:@"tap(bindEvent)"] andLepusRawEvents:[NSSet set]];
  id<LynxEventTargetBase> target = ui;
  XCTAssertTrue([target respondsToSelector:@selector(hasEventListener:)]);

  BOOL hasTapListener = [target respondsToSelector:@selector(hasEventListener:)] &&
                        [target hasEventListener:LynxEventTap];
  BOOL hasClickListener = [target respondsToSelector:@selector(hasEventListener:)] &&
                          [target hasEventListener:LynxEventClick];
  XCTAssertTrue(hasTapListener);
  XCTAssertFalse(hasClickListener);
}

- (void)testUIHasEventListenerTracksBindingsAndClearing {
  LynxUI* ui = [self createUI];
  id<LynxEventTargetBase> target = ui;
  XCTAssertFalse([target hasEventListener:LynxEventTap]);

  [ui setRawEvents:[NSSet setWithObject:@"tap(bindEvent)"] andLepusRawEvents:[NSSet set]];
  XCTAssertTrue([target hasEventListener:LynxEventTap]);
  XCTAssertFalse([target hasEventListener:LynxEventClick]);
  XCTAssertFalse([target hasEventListener:@"tap(bindEvent)"]);

  [ui setRawEvents:[NSSet setWithObject:@"click(catchEvent)"] andLepusRawEvents:[NSSet set]];
  XCTAssertFalse([target hasEventListener:LynxEventTap]);
  XCTAssertTrue([target hasEventListener:LynxEventClick]);

  [ui setRawEvents:[NSSet set] andLepusRawEvents:[NSSet set]];
  XCTAssertFalse([target hasEventListener:LynxEventTap]);
  XCTAssertFalse([target hasEventListener:LynxEventClick]);
}

- (void)testUIHasEventListenerIncludesLepusBindings {
  LynxUI* ui = [self createUI];
  [ui setRawEvents:[NSSet setWithObject:@"click(bindEvent)"]
      andLepusRawEvents:[NSSet setWithArray:@[
        @"longpress(catchEvent)", @"touchstart(capture-bind)", @"touchend(capture-catch)"
      ]]];

  XCTAssertTrue([ui hasEventListener:LynxEventClick]);
  XCTAssertTrue([ui hasEventListener:LynxEventLongPress]);
  XCTAssertTrue([ui hasEventListener:LynxEventTouchStart]);
  XCTAssertTrue([ui hasEventListener:LynxEventTouchEnd]);
  XCTAssertFalse([ui hasEventListener:LynxEventTap]);
}

- (void)testUIHasEventListenerKeepsLepusBindingAfterJSRemoval {
  LynxUI* ui = [self createUI];
  NSSet<NSString*>* lepusEvents = [NSSet setWithObject:@"tap(catchEvent)"];
  [ui setRawEvents:[NSSet setWithObject:@"tap(bindEvent)"] andLepusRawEvents:lepusEvents];
  XCTAssertTrue([ui hasEventListener:LynxEventTap]);

  [ui setRawEvents:[NSSet set] andLepusRawEvents:lepusEvents];
  XCTAssertTrue([ui hasEventListener:LynxEventTap]);

  [ui setRawEvents:[NSSet set] andLepusRawEvents:[NSSet set]];
  XCTAssertFalse([ui hasEventListener:LynxEventTap]);
}

- (void)testUIHasEventListenerIncludesGlobalBindings {
  LynxUI* ui = [self createUI];
  [ui setRawEvents:[NSSet setWithObject:@"tap(global-bindEvent)"]
      andLepusRawEvents:[NSSet setWithObject:@"longpress(global-bindEvent)"]];

  XCTAssertTrue([ui hasEventListener:LynxEventTap]);
  XCTAssertTrue([ui hasEventListener:LynxEventLongPress]);
  XCTAssertFalse([ui hasEventListener:LynxEventClick]);
}

- (void)testUIHasEventListenerDoesNotSearchParentChain {
  LynxUI* root = [self createUI];
  LynxUI* parent = [self createUI];
  LynxUI* child = [self createUI];
  [root setRawEvents:[NSSet setWithObject:@"tap(bindEvent)"] andLepusRawEvents:[NSSet set]];
  [parent setRawEvents:[NSSet setWithObject:@"longpress(bindEvent)"] andLepusRawEvents:[NSSet set]];
  parent.parent = root;
  child.parent = parent;

  XCTAssertEqual([child parentResponder], parent);
  XCTAssertEqual([parent parentResponder], root);
  XCTAssertTrue([root hasEventListener:LynxEventTap]);
  XCTAssertTrue([parent hasEventListener:LynxEventLongPress]);
  XCTAssertFalse([parent hasEventListener:LynxEventTap]);
  XCTAssertFalse([child hasEventListener:LynxEventTap]);
  XCTAssertFalse([child hasEventListener:LynxEventLongPress]);

  [child setRawEvents:[NSSet setWithObject:@"click(bindEvent)"] andLepusRawEvents:[NSSet set]];
  XCTAssertTrue([child hasEventListener:LynxEventClick]);
  XCTAssertFalse([child hasEventListener:LynxEventTap]);
  XCTAssertFalse([child hasEventListener:LynxEventLongPress]);
  XCTAssertFalse([parent hasEventListener:LynxEventClick]);
}

- (void)testSpanHasEventListenerUsesEventSetSnapshot {
  LynxShadowNode* node = [[LynxShadowNode alloc] initWithSign:1 tagName:@"inline-text"];
  NSMutableDictionary<NSString*, LynxEventSpec*>* events = [[LynxEventSpec
       convertRawEvents:[NSSet setWithObject:@"tap(bindEvent)"]
      andRwaLepusEvents:[NSSet setWithObject:@"longpress(global-bindEvent)"]] mutableCopy];
  node.eventSet = events;
  LynxEventTargetSpan* original = [[LynxEventTargetSpan alloc] initWithShadowNode:node
                                                                            frame:CGRectZero];

  [events removeObjectForKey:LynxEventTap];
  events[LynxEventClick] = [[LynxEventSpec alloc] initWithRawEvent:@"click(catchEvent)"
                                                       withJSEvent:YES];
  LynxEventTargetSpan* updated = [[LynxEventTargetSpan alloc] initWithShadowNode:node
                                                                           frame:CGRectZero];

  XCTAssertTrue([original hasEventListener:LynxEventTap]);
  XCTAssertTrue([original hasEventListener:LynxEventLongPress]);
  XCTAssertFalse([original hasEventListener:LynxEventClick]);
  XCTAssertFalse([updated hasEventListener:LynxEventTap]);
  XCTAssertTrue([updated hasEventListener:LynxEventLongPress]);
  XCTAssertTrue([updated hasEventListener:LynxEventClick]);

  node.eventSet = nil;
  LynxEventTargetSpan* empty = [[LynxEventTargetSpan alloc] initWithShadowNode:node
                                                                         frame:CGRectZero];
  XCTAssertFalse([empty hasEventListener:LynxEventTap]);
  XCTAssertFalse([empty hasEventListener:LynxEventClick]);
  XCTAssertFalse([empty hasEventListener:LynxEventLongPress]);
  XCTAssertTrue([original hasEventListener:LynxEventTap]);
  XCTAssertTrue([updated hasEventListener:LynxEventClick]);
}

- (void)testSpanHasEventListenerDoesNotSearchParentChain {
  LynxUI* root = [self createUI];
  [root setRawEvents:[NSSet setWithObject:@"tap(bindEvent)"] andLepusRawEvents:[NSSet set]];
  LynxShadowNode* parentNode = [[LynxShadowNode alloc] initWithSign:1 tagName:@"inline-text"];
  parentNode.eventSet =
      [LynxEventSpec convertRawEvents:[NSSet setWithObject:@"longpress(bindEvent)"]
                    andRwaLepusEvents:[NSSet set]];
  LynxEventTargetSpan* parent = [[LynxEventTargetSpan alloc] initWithShadowNode:parentNode
                                                                          frame:CGRectZero];
  [parent setParentEventTarget:root];
  LynxShadowNode* childNode = [[LynxShadowNode alloc] initWithSign:2 tagName:@"inline-text"];
  LynxEventTargetSpan* empty = [[LynxEventTargetSpan alloc] initWithShadowNode:childNode
                                                                         frame:CGRectZero];
  [empty setParentEventTarget:parent];

  XCTAssertEqual([empty parentResponder], parent);
  XCTAssertEqual([parent parentResponder], root);
  XCTAssertTrue([parent hasEventListener:LynxEventLongPress]);
  XCTAssertFalse([parent hasEventListener:LynxEventTap]);
  XCTAssertFalse([empty hasEventListener:LynxEventLongPress]);
  XCTAssertFalse([empty hasEventListener:LynxEventTap]);

  childNode.eventSet = [LynxEventSpec convertRawEvents:[NSSet setWithObject:@"click(bindEvent)"]
                                     andRwaLepusEvents:[NSSet set]];
  LynxEventTargetSpan* child = [[LynxEventTargetSpan alloc] initWithShadowNode:childNode
                                                                         frame:CGRectZero];
  [child setParentEventTarget:parent];
  XCTAssertTrue([child hasEventListener:LynxEventClick]);
  XCTAssertFalse([child hasEventListener:LynxEventLongPress]);
  XCTAssertFalse([child hasEventListener:LynxEventTap]);
}

- (NSArray<NSString*>*)textServiceEventNames {
  // Order matches the private event-mask bits in LynxUIText.m.
  return @[
    LynxEventTap, LynxEventClick, LynxEventLongPress, LynxEventTouchStart, LynxEventTouchMove,
    LynxEventTouchEnd, LynxEventTouchCancel
  ];
}

- (void)testTextServiceHasEventListenerMatchesEachEventMaskBit {
  NSArray<NSString*>* names = [self textServiceEventNames];
  for (NSUInteger maskIndex = 0; maskIndex < names.count; ++maskIndex) {
    NSUInteger mask = (NSUInteger)1 << maskIndex;
    LynxTextServiceEventTarget* target = [[LynxTextServiceEventTarget alloc] initWithSign:1
                                                                                eventMask:mask];
    for (NSUInteger nameIndex = 0; nameIndex < names.count; ++nameIndex) {
      XCTAssertEqual([target hasEventListener:names[nameIndex]], (BOOL)(maskIndex == nameIndex),
                     @"mask %@, event %@", @(mask), names[nameIndex]);
    }
  }
}

- (void)testTextServiceHasEventListenerHandlesCombinedAndEmptyMasks {
  NSUInteger mask = (1u << 0) | (1u << 4) | (1u << 6);
  LynxTextServiceEventTarget* combined = [[LynxTextServiceEventTarget alloc] initWithSign:1
                                                                                eventMask:mask];
  LynxTextServiceEventTarget* empty = [[LynxTextServiceEventTarget alloc] initWithSign:2
                                                                             eventMask:0];
  NSArray<NSString*>* expected = @[ LynxEventTap, LynxEventTouchMove, LynxEventTouchCancel ];
  for (NSString* name in [self textServiceEventNames]) {
    XCTAssertEqual([combined hasEventListener:name], [expected containsObject:name], @"%@", name);
    XCTAssertFalse([empty hasEventListener:name], @"%@", name);
  }
  XCTAssertFalse([combined hasEventListener:@"custom"]);
}

- (void)testTextServiceHasEventListenerDoesNotSearchParentChain {
  LynxUI* root = [self createUI];
  [root setRawEvents:[NSSet setWithObject:@"tap(bindEvent)"] andLepusRawEvents:[NSSet set]];
  LynxTextServiceEventTarget* parent = [[LynxTextServiceEventTarget alloc] initWithSign:1
                                                                              eventMask:1u << 2];
  [parent setParentEventTarget:root];
  LynxTextServiceEventTarget* child = [[LynxTextServiceEventTarget alloc] initWithSign:2
                                                                             eventMask:1u << 1];
  [child setParentEventTarget:parent];
  LynxTextServiceEventTarget* empty = [[LynxTextServiceEventTarget alloc] initWithSign:3
                                                                             eventMask:0];
  [empty setParentEventTarget:parent];

  XCTAssertEqual([child parentResponder], parent);
  XCTAssertEqual([parent parentResponder], root);
  XCTAssertTrue([parent hasEventListener:LynxEventLongPress]);
  XCTAssertFalse([parent hasEventListener:LynxEventTap]);
  XCTAssertTrue([child hasEventListener:LynxEventClick]);
  XCTAssertFalse([child hasEventListener:LynxEventLongPress]);
  XCTAssertFalse([child hasEventListener:LynxEventTap]);
  XCTAssertFalse([empty hasEventListener:LynxEventLongPress]);
  XCTAssertFalse([empty hasEventListener:LynxEventTap]);
}

@end
