// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxRecorderPageManager.h>
#import <XCTest/XCTest.h>

@interface LynxRecorderPageManager (RouteTesting)
- (BOOL)commitRouteForLabel:(NSString *)label popLast:(BOOL *)popLast;
@end

@interface LynxRecorderPageManagerUnitTest : XCTestCase
@end

@implementation LynxRecorderPageManagerUnitTest

- (LynxRecorderPageManager *)managerWithPages:(NSDictionary *)pages
                                      routers:(NSDictionary *)routers {
  LynxRecorderPageManager *manager = [[LynxRecorderPageManager alloc] init];
  [manager setValue:pages forKey:@"pages"];
  [manager setValue:routers forKey:@"routers"];
  [manager setValue:[NSMutableArray arrayWithObject:@"detail#0"] forKey:@"pageStack"];
  return manager;
}

- (void)testDetailToSquareCommitsValidRoute {
  LynxRecorderPageManager *manager =
      [self managerWithPages:@{@"detail" : @{@"url" : @"detail"}, @"square" : @{@"url" : @"square"}}
                     routers:@{@"detail" : @{@"to-square" : @{@"next" : @"square"}}}];
  BOOL popLast = YES;

  XCTAssertTrue([manager commitRouteForLabel:@"to-square" popLast:&popLast]);
  XCTAssertFalse(popLast);
  XCTAssertEqualObjects([[manager valueForKey:@"pageStack"] lastObject], @"square#0");
}

- (void)testInvalidRouteKeepsCurrentPageStack {
  LynxRecorderPageManager *manager =
      [self managerWithPages:@{@"detail" : @{@"url" : @"detail"}}
                     routers:@{@"detail" : @{@"to-square" : @{@"next" : @"square"}}}];
  NSArray *originalStack = [[manager valueForKey:@"pageStack"] copy];
  BOOL popLast = YES;

  XCTAssertFalse([manager commitRouteForLabel:@"to-square" popLast:&popLast]);
  XCTAssertEqualObjects([manager valueForKey:@"pageStack"], originalStack);
  XCTAssertTrue(popLast);
}

- (void)testDetailAndSquareRoutesRemainTransactionalAcrossRepeatedTransitions {
  LynxRecorderPageManager *manager =
      [self managerWithPages:@{@"detail" : @{@"url" : @"detail"}, @"square" : @{@"url" : @"square"}}
                     routers:@{
                       @"detail" : @{@"to-square" : @{@"next" : @"square", @"popLast" : @YES}},
                       @"square" : @{@"to-detail" : @{@"next" : @"detail", @"popLast" : @YES}}
                     }];

  for (NSInteger index = 0; index < 100; ++index) {
    BOOL fromDetail = index % 2 == 0;
    BOOL popLast = NO;
    NSString *label = fromDetail ? @"to-square" : @"to-detail";
    NSString *expectedPage = fromDetail ? @"square#0" : @"detail#0";

    XCTAssertTrue([manager commitRouteForLabel:label popLast:&popLast]);
    XCTAssertTrue(popLast);
    XCTAssertEqualObjects([manager valueForKey:@"pageStack"], (@[ expectedPage ]));
  }
}

@end
