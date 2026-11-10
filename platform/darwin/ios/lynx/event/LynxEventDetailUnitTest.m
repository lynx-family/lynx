// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxEventDetail.h>
#import <Lynx/LynxTouchEvent.h>
#import <XCTest/XCTest.h>

@interface LynxEventDetailUnitTest : XCTestCase
@end

@implementation LynxEventDetailUnitTest

- (void)testCustomEventReturnsPayloadWithoutEnvelope {
  NSDictionary* payload = @{@"value" : @42};
  LynxCustomEvent* event = [[LynxCustomEvent alloc] initWithName:@"custom"
                                                      targetSign:1
                                                          params:[payload mutableCopy]];
  LynxEventDetail* detail = [[LynxEventDetail alloc] initWithEvent:event target:nil lynxView:nil];

  XCTAssertEqualObjects([detail params], payload);
  XCTAssertEqual([detail params], event.params);
}

- (void)testDetailEventReturnsPayloadWithoutEnvelope {
  NSDictionary* payload = @{@"value" : @42};
  LynxDetailEvent* event = [[LynxDetailEvent alloc] initWithName:@"custom"
                                                      targetSign:1
                                                          detail:[payload mutableCopy]];
  LynxEventDetail* detail = [[LynxEventDetail alloc] initWithEvent:event target:nil lynxView:nil];

  XCTAssertEqualObjects([detail params], payload);
  XCTAssertEqual([detail params], event.params);
}

- (void)testCustomEventWithoutPayloadReturnsEmptyParameters {
  LynxCustomEvent* event = [[LynxCustomEvent alloc] initWithName:@"custom" targetSign:1];
  LynxEventDetail* detail = [[LynxEventDetail alloc] initWithEvent:event target:nil lynxView:nil];

  XCTAssertEqualObjects([detail params], @{});
  XCTAssertNil(event.params);
  XCTAssertNil([event generateEventBody][@"params"]);
}

- (void)testTouchEventHasNoCustomParameters {
  LynxTouchEvent* event = [[LynxTouchEvent alloc] initWithName:LynxEventTap targetTag:1];
  LynxEventDetail* detail = [[LynxEventDetail alloc] initWithEvent:event target:nil lynxView:nil];

  XCTAssertNil([detail params]);
}

- (void)testCustomEventFrontendBodyRetainsEnvelope {
  NSDictionary* payload = @{@"value" : @42};
  LynxCustomEvent* event = [[LynxCustomEvent alloc] initWithName:@"custom"
                                                      targetSign:1
                                               currentTargetSign:2
                                                          params:[payload mutableCopy]];
  LynxEventDetail* detail = [[LynxEventDetail alloc] initWithEvent:event target:nil lynxView:nil];
  [detail params];

  NSDictionary* body = [event generateEventBody];
  XCTAssertEqualObjects(body[@"type"], @"custom");
  XCTAssertEqualObjects(body[@"target"], @1);
  XCTAssertEqualObjects(body[@"currentTarget"], @2);
  XCTAssertEqualObjects(body[@"params"], payload);
}

- (void)testDetailEventFrontendBodyRetainsEnvelope {
  NSDictionary* payload = @{@"value" : @42};
  LynxDetailEvent* event = [[LynxDetailEvent alloc] initWithName:@"custom"
                                                      targetSign:1
                                               currentTargetSign:2
                                                          detail:[payload mutableCopy]];
  LynxEventDetail* detail = [[LynxEventDetail alloc] initWithEvent:event target:nil lynxView:nil];
  [detail params];

  NSDictionary* body = [event generateEventBody];
  XCTAssertEqualObjects(body[@"type"], @"custom");
  XCTAssertEqualObjects(body[@"target"], @1);
  XCTAssertEqualObjects(body[@"currentTarget"], @2);
  XCTAssertEqualObjects(body[@"detail"], payload);
}

@end
