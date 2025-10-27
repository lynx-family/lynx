// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxRootUI.h>
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

@end
