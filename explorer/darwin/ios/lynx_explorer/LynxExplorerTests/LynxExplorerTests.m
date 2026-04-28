// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import "AppDelegate+DebugRouter.h"
#import "AppDelegate.h"

@interface LynxExplorerTests : XCTestCase

@property(nonatomic, strong) AppDelegate *appDelegate;

@end

@implementation LynxExplorerTests

- (void)setUp {
  [super setUp];
  self.appDelegate = [[AppDelegate alloc] init];
  self.appDelegate.navigationController = [[UINavigationController alloc] init];
}

- (void)tearDown {
  self.appDelegate = nil;
  [super tearDown];
}

- (void)testOpenPageFailsWhenURLIsMissing {
  NSDictionary<NSString *, id> *result = [self.appDelegate debugRouterOpenPageResponseForURL:nil];

  XCTAssertEqualObjects(result[@"code"], @(-1));
  XCTAssertEqualObjects(result[@"message"], @"url is required");
}

- (void)testOpenPageSucceedsWhenURLIsSupported {
  NSDictionary<NSString *, id> *result =
      [self.appDelegate debugRouterOpenPageResponseForURL:@"https://example.com/page"];

  XCTAssertEqualObjects(result[@"code"], @(0));
  XCTAssertNil(result[@"message"]);
}

- (void)testClosePageFailsWhenNoPageCanBeClosed {
  NSDictionary<NSString *, id> *result = [self.appDelegate debugRouterClosePageResponse];

  XCTAssertEqualObjects(result[@"code"], @(-1));
  XCTAssertEqualObjects(result[@"message"], @"no page to close");
}

- (void)testClosePageSucceedsWhenNavigationStackHasMoreThanOnePage {
  UIViewController *rootViewController = [[UIViewController alloc] init];
  UIViewController *detailViewController = [[UIViewController alloc] init];
  [self.appDelegate.navigationController
      setViewControllers:@[ rootViewController, detailViewController ]
                animated:NO];

  NSDictionary<NSString *, id> *result = [self.appDelegate debugRouterClosePageResponse];

  XCTAssertEqualObjects(result[@"code"], @(0));
  XCTAssertEqual(self.appDelegate.navigationController.viewControllers.count, 1);
}

@end
