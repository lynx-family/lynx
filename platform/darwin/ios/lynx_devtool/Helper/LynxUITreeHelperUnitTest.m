// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxUIContext.h>
#import <LynxDevtool/LynxUITreeHelper.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>

extern NSString* const ScreenshotModeLynxView;
extern NSString* const ScreenshotModeFullScreen;

@interface LynxUITreeHelperUnitTest : XCTestCase

@end

@implementation LynxUITreeHelperUnitTest {
  LynxUITreeHelper* _uiTreeHelper;
  LynxUIOwner* _uiOwner;
  LynxUIContext* _uiContext;
}

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
  _uiTreeHelper = [[LynxUITreeHelper alloc] init];
  _uiTreeHelper = OCMPartialMock(_uiTreeHelper);
  _uiOwner = OCMClassMock([LynxUIOwner class]);
  _uiContext = OCMClassMock([LynxUIContext class]);
  [_uiTreeHelper attachLynxUIOwner:_uiOwner];
}

- (void)tearDown {
  _uiTreeHelper = nil;
  _uiOwner = nil;
  _uiContext = nil;
}

- (void)testGetRectToWindowFallbackToRootViewWhenRootUIRectIsEmpty {
  UIView* rootView = [[UIView alloc] initWithFrame:CGRectMake(10, 20, 100, 200)];
  OCMStub([_uiOwner getRootSign]).andReturn(1);
  OCMStub([_uiOwner findUIBySign:1]).andReturn(nil);
  OCMStub([_uiOwner uiContext]).andReturn(_uiContext);
  OCMStub([_uiContext rootView]).andReturn(rootView);

  CGRect rect = [_uiTreeHelper getRectToWindow];
  CGFloat scale = UIScreen.mainScreen.scale;

  XCTAssertEqualWithAccuracy(rect.size.width, 100 * scale, 0.001);
  XCTAssertEqualWithAccuracy(rect.size.height, 200 * scale, 0.001);
}

@end
