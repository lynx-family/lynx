// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>

#import "LynxConverter+UI.h"
#import "LynxConverter.h"

@interface LynxConverterUIUnitTest : XCTestCase
@end

@implementation LynxConverterUIUnitTest

- (void)setUp {
}

- (void)tearDown {
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
