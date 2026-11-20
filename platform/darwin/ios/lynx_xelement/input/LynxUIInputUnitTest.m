// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxPropsProcessor.h>
#import <XCTest/XCTest.h>
#import <XElement/LynxUIInput.h>

@interface LynxUIInputUnitTest : XCTestCase
@end

@implementation LynxUIInputUnitTest

- (void)assertColor:(UIColor *)color
               red:(CGFloat)expectedRed
             green:(CGFloat)expectedGreen
              blue:(CGFloat)expectedBlue
             alpha:(CGFloat)expectedAlpha {
  CGFloat red = 0, green = 0, blue = 0, alpha = 0;
  XCTAssertTrue([color getRed:&red green:&green blue:&blue alpha:&alpha]);
  XCTAssertEqualWithAccuracy(red, expectedRed, 0.001);
  XCTAssertEqualWithAccuracy(green, expectedGreen, 0.001);
  XCTAssertEqualWithAccuracy(blue, expectedBlue, 0.001);
  XCTAssertEqualWithAccuracy(alpha, expectedAlpha, 0.001);
}

- (LynxUIInput *)inputWithPlaceholder {
  LynxUIInput *input = [LynxUIInput new];
  [LynxPropsProcessor updateProp:@"Name" withKey:@"placeholder" forUI:input];
  return input;
}

- (UIColor *)placeholderColor:(LynxUIInput *)input {
  [input propsDidUpdate];
  return [input.view.attributedPlaceholder attribute:NSForegroundColorAttributeName
                                           atIndex:0
                                    effectiveRange:NULL];
}

- (void)testPlaceholderColorAcceptsLiteralString {
  LynxUIInput *input = [self inputWithPlaceholder];
  [LynxPropsProcessor updateProp:@"#999999" withKey:@"placeholder-color" forUI:input];
  [self assertColor:[self placeholderColor:input] red:0.6 green:0.6 blue:0.6 alpha:1.0];
}

- (void)testTextColorAcceptsRGBAString {
  LynxUIInput *input = [LynxUIInput new];
  [LynxPropsProcessor updateProp:@"#33669980" withKey:@"color" forUI:input];
  [input propsDidUpdate];
  [self assertColor:input.view.defaultTextAttributes[NSForegroundColorAttributeName]
                red:0.2
              green:0.4
               blue:0.6
              alpha:128.0 / 255.0];
}

- (void)testPlaceholderAliasPreservesStringAndResolvedARGBValues {
  LynxUIInput *input = [self inputWithPlaceholder];
  [LynxPropsProcessor updateProp:@"#336699" withKey:@"-x-placeholder-color" forUI:input];
  [self assertColor:[self placeholderColor:input] red:0.2 green:0.4 blue:0.6 alpha:1.0];

  // CSS colors, including resolved variables, reach the native prop setter as numeric ARGB.
  [LynxPropsProcessor updateProp:@(0x80336699) withKey:@"-x-placeholder-color" forUI:input];
  [self assertColor:[self placeholderColor:input]
                red:0.2
              green:0.4
               blue:0.6
              alpha:128.0 / 255.0];
}

- (void)testExistingUIColorIsPreservedForTextAndPlaceholder {
  LynxUIInput *input = [self inputWithPlaceholder];
  UIColor *color = [UIColor colorWithRed:0.2 green:0.4 blue:0.6 alpha:0.5];
  [LynxPropsProcessor updateProp:color withKey:@"color" forUI:input];
  XCTAssertTrue(input.inputAttrs[NSForegroundColorAttributeName] == color);

  [LynxPropsProcessor updateProp:color withKey:@"placeholder-color" forUI:input];
  XCTAssertTrue(input.placeholderAttrs[NSForegroundColorAttributeName] == color);

  [LynxPropsProcessor updateProp:color withKey:@"-x-placeholder-color" forUI:input];
  XCTAssertTrue(input.placeholderAttrs[NSForegroundColorAttributeName] == color);
}

- (void)testPlaceholderColorResetRestoresDefault {
  LynxUIInput *input = [self inputWithPlaceholder];
  [LynxPropsProcessor updateProp:@"#999999" withKey:@"placeholder-color" forUI:input];
  [LynxPropsProcessor updateProp:[NSNull null] withKey:@"placeholder-color" forUI:input];
  [self assertColor:[self placeholderColor:input] red:0.235 green:0.263 blue:0.235 alpha:0.3];
}

@end
