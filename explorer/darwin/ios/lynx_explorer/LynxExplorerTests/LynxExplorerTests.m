// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import "LynxViewShellViewController.h"

@interface LynxViewShellViewController (Testing)
- (id)getGlobalPropsForScreenSize:(CGSize)screenSize;
@end

@interface LynxExplorerTests : XCTestCase

@end

@implementation LynxExplorerTests

- (void)testShellViewControllerAllowsAutorotation {
  LynxViewShellViewController *shellVC = [[LynxViewShellViewController alloc] init];
  XCTAssertTrue([shellVC shouldAutorotate]);
}

- (void)testShellViewControllerSupportsLandscapeByDefault {
  LynxViewShellViewController *shellVC = [[LynxViewShellViewController alloc] init];
  shellVC.params = [NSMutableDictionary dictionary];
  XCTAssertEqual([shellVC supportedInterfaceOrientations], UIInterfaceOrientationMaskAll);
}

- (void)testShellViewControllerKeepsExplicitOrientationRestrictions {
  LynxViewShellViewController *shellVC = [[LynxViewShellViewController alloc] init];

  shellVC.params = [@{@"orientation" : @"portrait"} mutableCopy];
  XCTAssertEqual([shellVC supportedInterfaceOrientations], UIInterfaceOrientationMaskPortrait);

  shellVC.params = [@{@"orientation" : @"landscape"} mutableCopy];
  XCTAssertEqual([shellVC supportedInterfaceOrientations], UIInterfaceOrientationMaskLandscape);
}

- (void)testShellViewControllerExposesRouteParamsAsQueryItems {
  LynxViewShellViewController *shellVC = [[LynxViewShellViewController alloc] init];
  shellVC.params = [@{ @"userId" : @"42", @"action" : @"view" } mutableCopy];

  NSDictionary *globalProps = [[shellVC getGlobalPropsForScreenSize:CGSizeMake(402, 874)] dictionary];

  XCTAssertEqualObjects(globalProps[@"queryItems"], shellVC.params);
}

@end
