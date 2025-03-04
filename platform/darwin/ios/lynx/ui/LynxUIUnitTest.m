// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import "LynxPropsProcessor.h"
#import "LynxUI+Gesture.h"
#import "LynxUI+Internal.h"
#import "LynxUI+Private.h"
#import "LynxUIView.h"

@implementation LynxUI (Test)
- (UIView *)createView {
  return nil;
}
@end

@interface LynxUIUnitTest : XCTestCase
@end
@implementation LynxUIUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testIsVisible {
  LynxUI *ui = OCMPartialMock([[LynxUI alloc] initWithView:nil]);
  XCTAssertFalse([ui isVisible]);

  UIView *view = OCMClassMock([UIView class]);
  CGRect rect = CGRectZero;
  ui = OCMPartialMock([[LynxUI alloc] initWithView:view]);
  OCMExpect([view isHidden]).andReturn(NO);
  OCMExpect([view alpha]).andReturn(1);
  OCMExpect([view frame]).andReturn(rect);
  OCMExpect([view clipsToBounds]).andReturn(NO);
  OCMExpect([view window]).andReturn(OCMClassMock([UIWindow class]));

  XCTAssertTrue([ui isVisible]);
  OCMVerifyAll(ui);
}

- (void)testCopy {
  LynxUIView *parent = [[LynxUIView alloc] initWithView:nil];
  [parent updateFrame:CGRectMake(0, 0, 100, 100)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  [LynxPropsProcessor updateProp:UIColor.redColor withKey:@"background-color" forUI:parent];

  LynxUIView *child = [[LynxUIView alloc] initWithView:nil];
  [child updateFrame:CGRectMake(0, 0, 50, 50)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  [LynxPropsProcessor updateProp:UIColor.greenColor withKey:@"background-color" forUI:child];

  [parent insertChild:child atIndex:0];

  [parent propsDidUpdate];
  [child propsDidUpdate];
  [parent onNodeReadyForUIOwner];
  [child onNodeReadyForUIOwner];

  LynxUIView *copy = [parent copy];

  XCTAssert([copy isKindOfClass:LynxUIView.class]);
  XCTAssert(copy.frame.size.width == 100);
  XCTAssert(copy.frame.size.height == 100);
  XCTAssert(copy.backgroundManager.backgroundColor == parent.backgroundManager.backgroundColor);

  LynxUIView *copyChild = parent.children.firstObject;

  XCTAssert([copyChild isKindOfClass:LynxUIView.class]);
  XCTAssert(copyChild.frame.size.width == 50);
  XCTAssert(copyChild.frame.size.height == 50);
  XCTAssert(copyChild.backgroundManager.backgroundColor == child.backgroundManager.backgroundColor);
}

- (void)testGestureInterfaces {
  LynxUIView *ui = [[LynxUIView alloc] initWithView:nil];

  [ui setGestureDetectorState:1 state:LynxGestureStateActive];
  [ui setGestureDetectorState:1 state:LynxGestureStateFail];
  [ui setGestureDetectorState:1 state:LynxGestureStateEnd];

  [ui onGestureScrollBy:CGPointMake(10, 10)];
  XCTAssertTrue([ui canConsumeGesture:CGPointMake(10, 10)]);
  XCTAssertTrue([ui getGestureArenaMemberId] == 0);
  XCTAssertTrue([ui getMemberScrollX] == 0.0f);
  XCTAssertTrue([ui getMemberScrollY] == 0.0f);
  XCTAssertTrue([ui getGestureDetectorMap].count == 0);
  XCTAssertTrue([ui getGestureHandlers].count == 0);
}

@end
