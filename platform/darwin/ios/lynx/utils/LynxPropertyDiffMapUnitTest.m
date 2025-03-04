// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import "LynxPropertyDiffMap.h"

@interface LynxPropertyDiffMapUnitTest : XCTestCase

@end

@implementation LynxPropertyDiffMapUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.l
}

- (LynxPropertyDiffMap *)genDiffMap {
  LynxPropertyDiffMap *diffMap = [[LynxPropertyDiffMap alloc] init];
  [diffMap putValue:@1 forKey:@"1"];
  [diffMap putValue:@"2" forKey:@"2"];
  [diffMap putValue:@{@"value" : @3} forKey:@"3"];
  [diffMap putValue:@YES forKey:@"4"];
  return diffMap;
}

- (void)testPropMapGetValue {
  LynxPropertyDiffMap *diffMap = [self genDiffMap];
  XCTAssertTrue(diffMap != nil);

  XCTAssertTrue(1 == [[diffMap getValueForKey:@"1"] integerValue]);
  XCTAssertTrue([@"2" isEqualToString:[diffMap getValueForKey:@"2"]]);
  XCTAssertTrue(3 == [[[diffMap getValueForKey:@"3"] objectForKey:@"value"] integerValue]);
  XCTAssertTrue([[diffMap getValueForKey:@"4"] boolValue]);
}

- (void)testPropMapGetDefaultValue {
  LynxPropertyDiffMap *diffMap = [self genDiffMap];
  XCTAssertTrue(5 == [[diffMap getValueForKey:@"5" defaultValue:@5] integerValue]);
}

- (void)testPropMapUpdated {
  LynxPropertyDiffMap *diffMap = [self genDiffMap];
  [diffMap clearDirtyRecords];
  XCTAssertFalse([diffMap isValueForKeyUpdated:@"1"]);

  XCTAssertTrue([diffMap getUpdatedValueForKey:@"1"] == nil);

  NSNumber *updatedValue = nil;
  XCTAssertTrue(![diffMap valueChangedForKey:@"1" updateTo:&updatedValue] && !updatedValue);

  [diffMap putValue:@11 forKey:@"1"];
  XCTAssertTrue([diffMap isValueForKeyUpdated:@"1"]);
  XCTAssertTrue([[diffMap getUpdatedValueForKey:@"1"] integerValue] == 11);
  XCTAssertTrue([diffMap valueChangedForKey:@"1" updateTo:&updatedValue] &&
                [updatedValue integerValue] == 11);
  XCTAssertTrue([[diffMap getUpdatedKeys] containsObject:@"1"]);
}

@end
