// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxRootUI.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import "LynxUITreeHelper.h"

extern NSString* const ScreenshotModeLynxView;
extern NSString* const ScreenshotModeFullScreen;

@interface LynxUITreeHelperUnitTest : XCTestCase

@end

@implementation LynxUITreeHelperUnitTest {
  LynxUITreeHelper* _uiTreeHelper;
  LynxUIOwner* _uiOwner;
}

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
  _uiTreeHelper = [[LynxUITreeHelper alloc] init];
  _uiTreeHelper = OCMPartialMock(_uiTreeHelper);
  _uiOwner = OCMClassMock([LynxUIOwner class]);
  [_uiTreeHelper attachLynxUIOwner:_uiOwner];
}

- (void)tearDown {
  _uiTreeHelper = nil;
  _uiOwner = nil;
}

- (void)testFindNodeIdForLocation {
  LynxRootUI* rootUI = OCMClassMock([LynxRootUI class]);
  OCMStub([_uiOwner rootUI]).andReturn(rootUI);
  UIView* lynxView = OCMClassMock([UIView class]);
  OCMStub([rootUI rootView]).andReturn(lynxView);
  OCMStub([_uiTreeHelper convertPointFromScreen:CGPointMake(1, 1) ToView:lynxView])
      .andReturn(CGPointMake(2, 2));

  OCMExpect([lynxView hitTest:CGPointMake(2, 2) withEvent:nil]);
  [_uiTreeHelper findNodeIdForLocationWithX:1 withY:1 fromUI:0 mode:ScreenshotModeFullScreen];
  OCMVerifyAll(lynxView);
  rootUI = nil;
  lynxView = nil;

  LynxUI* ui = OCMClassMock([LynxUI class]);
  UIView* view = OCMClassMock([UIView class]);
  OCMStub([_uiOwner findUIBySign:1]).andReturn(ui);
  OCMStub([ui view]).andReturn(view);
  OCMStub([_uiTreeHelper convertPointFromScreen:CGPointMake(1, 1) ToView:view])
      .andReturn(CGPointMake(3, 3));
  OCMExpect([view hitTest:CGPointMake(3, 3) withEvent:nil]);
  [_uiTreeHelper findNodeIdForLocationWithX:1 withY:1 fromUI:1 mode:ScreenshotModeFullScreen];
  OCMVerifyAll(view);
  ui = nil;
  view = nil;

  OCMStub([_uiOwner findUIBySign:2]).andReturn(nil);
  XCTAssertEqual([_uiTreeHelper findNodeIdForLocationWithX:1
                                                     withY:1
                                                    fromUI:2
                                                      mode:ScreenshotModeFullScreen],
                 0);
}

@end
