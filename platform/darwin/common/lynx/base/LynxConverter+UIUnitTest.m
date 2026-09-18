// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>

#import <Lynx/LynxConverter+UI.h>
#import <Lynx/LynxConverter.h>

@interface LynxConverterUIUnitTest : XCTestCase
@end

@implementation LynxConverterUIUnitTest

- (void)setUp {
}

- (void)tearDown {
}

- (void)testToNSUIntegerWithUnsupportedValues {
  NSArray *values = @[ @"1", [NSMutableString stringWithString:@"1"], @"", @[], @{} ];
  for (id value in values) {
    XCTAssertEqual([LynxConverter toNSUInteger:value], (NSUInteger)0);
  }
}

- (void)testToNSUIntegerPreservesNumbersAndDefaults {
  XCTAssertEqual([LynxConverter toNSUInteger:nil], (NSUInteger)0);
  XCTAssertEqual([LynxConverter toNSUInteger:NSNull.null], (NSUInteger)0);
  XCTAssertEqual([LynxConverter toNSUInteger:@0], (NSUInteger)0);
  XCTAssertEqual([LynxConverter toNSUInteger:@1], (NSUInteger)1);
  XCTAssertEqual([LynxConverter toNSUInteger:@(NSUIntegerMax)], NSUIntegerMax);
}

#ifdef OS_IOS
- (void)testToAccessibilityTraits {
  UIAccessibilityTraits traits =
      [LynxConverter toAccessibilityTraits:@"text,image,button,adjustable"];
  UIAccessibilityTraits expectedTraits =
      (UIAccessibilityTraitStaticText | UIAccessibilityTraitImage | UIAccessibilityTraitButton |
       UIAccessibilityTraitAdjustable);
  XCTAssertEqual(traits, expectedTraits);
}
#endif

@end
