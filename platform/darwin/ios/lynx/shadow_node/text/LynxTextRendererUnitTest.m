// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseTextShadowNode.h>
#import <Lynx/LynxTextRenderer.h>
#import <XCTest/XCTest.h>

@interface LynxTextRenderer (UnitTestPrivate)
- (NSDictionary *)calculateLastLineGlyphWidth:(NSRange)lastLineGlyphRange;
@end

@interface LynxTextRendererUnitTest : XCTestCase {
  LynxTextRenderer *textRender;
}

@end
@implementation LynxTextRendererUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
  NSString *str = @"This is test text.\nUse render function to get text width and height.";
  NSDictionary *attributesDic = @{
    NSFontAttributeName : [UIFont systemFontOfSize:25],
    NSForegroundColorAttributeName : [UIColor redColor],
  };
  NSMutableAttributedString *mutableAttributeStr =
      [[NSMutableAttributedString alloc] initWithString:str attributes:attributesDic];
  [mutableAttributeStr addAttribute:LynxInlineTextShadowNodeSignKey
                              value:[[LynxBaseTextShadowNode alloc] initWithSign:1 tagName:@"text"]
                              range:NSMakeRange(0, mutableAttributeStr.length)];
  LynxTextStyle *textStyle = [LynxTextStyle new];
  textStyle.fontSize = 25;
  LynxLayoutSpec *spec = [[LynxLayoutSpec alloc] initWithWidth:100.f
                                                        height:100.f
                                                     widthMode:LynxMeasureModeDefinite
                                                    heightMode:LynxMeasureModeIndefinite
                                                  textOverflow:LynxTextOverflowEllipsis
                                                      overflow:LynxNoOverflow
                                                    whiteSpace:LynxWhiteSpaceNormal
                                                    maxLineNum:NAN
                                                 maxTextLength:NAN
                                                     textStyle:textStyle
                                        enableTailColorConvert:FALSE];
  textRender = [[LynxTextRenderer alloc] initWithAttributedString:mutableAttributeStr
                                                       layoutSpec:spec];
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
  textRender = NULL;
}

- (void)disable_testGenSubSpan {
  // This is an example of a functional test case.
  // Use XCTAssert and related functions to verify your tests produce the correct results.
  [textRender genSubSpan];
  // generate four rect, the width of the second and the fourth rect is smaller than width
  // constraint
  XCTAssertTrue(textRender.subSpan.count == 4);
  // subSpan[0] is the first line, contains point (10, 10)
  XCTAssertTrue([textRender.subSpan[0] containsPoint:CGPointMake(10, 10)]);
  // subSpan[1] is the second line, the end of line is newline, doesn't contain point (99, 50)
  XCTAssertFalse([textRender.subSpan[1] containsPoint:CGPointMake(99, 50)]);
  // subSpan[3] is the last line, the end of line is blank, doesn't contain point (99,
  // textRender.size.height-10)
  XCTAssertFalse(
      [textRender.subSpan[3] containsPoint:CGPointMake(99, textRender.size.height - 10)]);
}

- (LynxTextRenderer *)createRendererWithString:(NSString *)string {
  NSDictionary *attributesDic = @{
    NSFontAttributeName : [UIFont systemFontOfSize:25],
    NSForegroundColorAttributeName : [UIColor redColor],
  };
  NSMutableAttributedString *mutableAttributeStr =
      [[NSMutableAttributedString alloc] initWithString:string attributes:attributesDic];
  [mutableAttributeStr addAttribute:LynxInlineTextShadowNodeSignKey
                              value:[[LynxBaseTextShadowNode alloc] initWithSign:1 tagName:@"text"]
                              range:NSMakeRange(0, mutableAttributeStr.length)];
  LynxTextStyle *textStyle = [LynxTextStyle new];
  textStyle.fontSize = 25;
  LynxLayoutSpec *spec = [[LynxLayoutSpec alloc] initWithWidth:200.f
                                                        height:100.f
                                                     widthMode:LynxMeasureModeDefinite
                                                    heightMode:LynxMeasureModeIndefinite
                                                  textOverflow:LynxTextOverflowEllipsis
                                                      overflow:LynxNoOverflow
                                                    whiteSpace:LynxWhiteSpaceNormal
                                                    maxLineNum:NAN
                                                 maxTextLength:NAN
                                                     textStyle:textStyle
                                        enableTailColorConvert:FALSE];
  LynxTextRenderer *renderer =
      [[LynxTextRenderer alloc] initWithAttributedString:mutableAttributeStr layoutSpec:spec];
  [renderer ensureTextRenderLayout];
  return renderer;
}

- (NSRange)lastLineGlyphRangeForRenderer:(LynxTextRenderer *)renderer {
  NSLayoutManager *layoutManager = renderer.layoutManager;
  NSRange glyphRange =
      [layoutManager glyphRangeForTextContainer:layoutManager.textContainers.firstObject];
  __block NSRange lastLineGlyphRange = NSMakeRange(0, 0);
  [layoutManager
      enumerateLineFragmentsForGlyphRange:glyphRange
                               usingBlock:^(CGRect rect, CGRect usedRect,
                                            NSTextContainer *_Nonnull textContainer,
                                            NSRange lineGlyphRange, BOOL *_Nonnull stop) {
                                 lastLineGlyphRange = lineGlyphRange;
                               }];
  return lastLineGlyphRange;
}

- (void)testLastLineGlyphWidthIncludesZeroWidthSpaceInMiddle {
  NSString *text = @"A\u200BB";
  LynxTextRenderer *renderer = [self createRendererWithString:text];
  NSRange lastLineGlyphRange = [self lastLineGlyphRangeForRenderer:renderer];
  NSDictionary *glyphWidthDic = [renderer calculateLastLineGlyphWidth:lastLineGlyphRange];

  NSUInteger zeroWidthCharIndex = [text rangeOfString:@"\u200B"].location;
  XCTAssertNotEqual(zeroWidthCharIndex, NSNotFound);

  BOOL foundZeroWidthGlyph = NO;
  NSLayoutManager *layoutManager = renderer.layoutManager;
  for (NSNumber *key in glyphWidthDic) {
    NSUInteger glyphIndex = [key unsignedIntegerValue];
    NSUInteger characterIndex = [layoutManager characterIndexForGlyphAtIndex:glyphIndex];
    if (characterIndex == zeroWidthCharIndex) {
      CGFloat width = [glyphWidthDic[key] floatValue];
      XCTAssertEqualWithAccuracy(width, 0.f, 0.0001f);
      foundZeroWidthGlyph = YES;
    }
  }
  XCTAssertTrue(foundZeroWidthGlyph);
}

- (void)testLastLineGlyphWidthIncludesZeroWidthJoinerAtLineStart {
  NSString *text = @"\u200DABC";
  LynxTextRenderer *renderer = [self createRendererWithString:text];
  NSRange lastLineGlyphRange = [self lastLineGlyphRangeForRenderer:renderer];
  NSDictionary *glyphWidthDic = [renderer calculateLastLineGlyphWidth:lastLineGlyphRange];

  NSUInteger zeroWidthCharIndex = [text rangeOfString:@"\u200D"].location;
  XCTAssertNotEqual(zeroWidthCharIndex, NSNotFound);

  BOOL foundZeroWidthGlyph = NO;
  NSLayoutManager *layoutManager = renderer.layoutManager;
  for (NSNumber *key in glyphWidthDic) {
    NSUInteger glyphIndex = [key unsignedIntegerValue];
    NSUInteger characterIndex = [layoutManager characterIndexForGlyphAtIndex:glyphIndex];
    if (characterIndex == zeroWidthCharIndex) {
      CGFloat width = [glyphWidthDic[key] floatValue];
      XCTAssertEqualWithAccuracy(width, 0.f, 0.0001f);
      foundZeroWidthGlyph = YES;
    }
  }
  XCTAssertTrue(foundZeroWidthGlyph);

  CGFloat sumOfGlyphWidth = 0.f;
  for (NSNumber *key in glyphWidthDic) {
    sumOfGlyphWidth += [glyphWidthDic[key] floatValue];
  }
  CGRect lastLineRect =
      [renderer.layoutManager lineFragmentUsedRectForGlyphAtIndex:lastLineGlyphRange.location
                                                   effectiveRange:NULL];
  XCTAssertLessThanOrEqual(fabs(sumOfGlyphWidth - lastLineRect.size.width), 1.f);
}

@end
