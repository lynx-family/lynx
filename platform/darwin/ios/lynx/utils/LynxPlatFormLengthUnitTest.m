// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import "LynxUIUnitUtils.h"

@interface LynxPlatFormLengthUnitTest : XCTestCase
@property NSArray* lValue;
@property NSArray* rValue;
@end

@implementation LynxPlatFormLengthUnitTest
- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
}

- (void)tearDown {
  _lValue = nil;
  _rValue = nil;
}

- (void)testEqual {
  _lValue = @[ @10, @0, @0.5, @1 ];
  LynxPlatformLength* lLength =
      [[LynxPlatformLength alloc] initWithValue:_lValue type:LynxPlatformLengthUnitCalc];

  _rValue = @[ @10, @0, @0.5, @0 ];
  LynxPlatformLength* rLength =
      [[LynxPlatformLength alloc] initWithValue:_rValue type:LynxPlatformLengthUnitCalc];

  XCTAssertFalse([lLength isEqual:rLength]);

  _rValue = @[ @10, @0, @0.5, @1 ];
  rLength = [[LynxPlatformLength alloc] initWithValue:_rValue type:LynxPlatformLengthUnitCalc];
  XCTAssertTrue([lLength isEqual:rLength]);
}

- (void)testValue {
  _lValue = @[ @10, @0, @0.5, @1 ];
  LynxPlatformLength* lLength =
      [[LynxPlatformLength alloc] initWithValue:_lValue type:LynxPlatformLengthUnitCalc];

  CGFloat l = [lLength valueWithParentValue:100.f];
  XCTAssertEqual(l, 60.f);
  _lValue = @[ @10, @0, @[ @0.5, @1, @10, @0 ], @2 ];
  lLength = [[LynxPlatformLength alloc] initWithValue:_lValue type:LynxPlatformLengthUnitCalc];

  l = [lLength valueWithParentValue:100.f];
  XCTAssertEqual(l, 70);

  _lValue = @[ @100, @0 ];
  lLength = [[LynxPlatformLength alloc] initWithValue:_lValue type:LynxPlatformLengthUnitCalc];
  l = [lLength valueWithParentValue:100.f];
  XCTAssertEqual(100, l);

  _lValue = @[ @0.5, @1 ];
  lLength = [[LynxPlatformLength alloc] initWithValue:_lValue type:LynxPlatformLengthUnitCalc];
  l = [lLength valueWithParentValue:100.f];
  XCTAssertEqual(50, l);
}

@end
