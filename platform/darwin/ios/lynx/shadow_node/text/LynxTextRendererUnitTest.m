// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseTextShadowNode.h>
#import <Lynx/LynxTextRenderer.h>
#import <XCTest/XCTest.h>

@interface LynxTextRenderer (LynxInlineEventTarget)
- (nullable NSNumber *)inlineTextEventTargetAtPoint:(CGPoint)point;
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

- (void)testInlineEventTargetsWithoutShadowNodes {
  NSAttributedStringKey key = @"LynxInlineTextEventTargetSignKey";
  NSTextStorage *storage = textRender.textStorage;
  [storage removeAttribute:LynxInlineTextShadowNodeSignKey range:NSMakeRange(0, storage.length)];
  [storage addAttribute:key value:@101 range:NSMakeRange(0, 17)];
  [storage addAttribute:key value:@102 range:NSMakeRange(5, 2)];
  [textRender ensureTextRenderLayout];

  NSLayoutManager *layout = textRender.layoutManager;
  NSTextContainer *container = layout.textContainers.firstObject;
  for (NSNumber *index in @[ @0, @5 ]) {
    NSRange glyphs = [layout glyphRangeForCharacterRange:NSMakeRange(index.unsignedIntegerValue, 1)
                                    actualCharacterRange:NULL];
    CGRect rect = [layout boundingRectForGlyphRange:glyphs inTextContainer:container];
    CGPoint point =
        CGPointMake(CGRectGetMidX(rect) + textRender.textContentOffsetX, CGRectGetMidY(rect));
    XCTAssertEqualObjects([textRender inlineTextEventTargetAtPoint:point],
                          index.integerValue == 0 ? @101 : @102);
    [storage removeAttribute:key range:NSMakeRange(index.unsignedIntegerValue, 1)];
    XCTAssertNil([textRender inlineTextEventTargetAtPoint:point]);
  }
  XCTAssertNil([textRender inlineTextEventTargetAtPoint:CGPointMake(-100, -100)]);
}

- (void)testInlineAttachmentDoesNotHitInheritedTextTarget {
  NSTextAttachment *attachment = [NSTextAttachment new];
  attachment.bounds = CGRectMake(0, 0, 24, 24);
  NSMutableAttributedString *text = [[NSMutableAttributedString alloc] initWithString:@"A"];
  [text appendAttributedString:[NSAttributedString attributedStringWithAttachment:attachment]];
  [text appendAttributedString:[[NSAttributedString alloc] initWithString:@"B"]];
  [text addAttributes:@{
    NSFontAttributeName : [UIFont systemFontOfSize:25],
    @"LynxInlineTextEventTargetSignKey" : @101
  }
                range:NSMakeRange(0, text.length)];
  [textRender.textStorage setAttributedString:text];
  [textRender ensureTextRenderLayout];

  NSLayoutManager *layout = textRender.layoutManager;
  for (NSUInteger index = 0; index < 3; ++index) {
    NSRange glyphs = [layout glyphRangeForCharacterRange:NSMakeRange(index, 1)
                                    actualCharacterRange:NULL];
    CGRect rect = [layout boundingRectForGlyphRange:glyphs
                                    inTextContainer:layout.textContainers.firstObject];
    CGPoint point =
        CGPointMake(CGRectGetMidX(rect) + textRender.textContentOffsetX, CGRectGetMidY(rect));
    NSNumber *target = [textRender inlineTextEventTargetAtPoint:point];
    if (index == 1) {
      XCTAssertNil(target);
    } else {
      XCTAssertEqualObjects(target, @101);
    }
  }
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

- (void)testInheritedTextStrokePreservesAbsoluteWidthWhenFontSizeChanges {
  CGFloat parentFontSize = 14;
  CGFloat childFontSize = 70;
  CGFloat strokeWidth = 2;
  NSDictionary<NSAttributedStringKey, id> *parentAttributes = @{
    NSFontAttributeName : [UIFont systemFontOfSize:parentFontSize],
    NSStrokeColorAttributeName : UIColor.whiteColor,
    NSStrokeWidthAttributeName : @(-strokeWidth / parentFontSize * 100),
  };

  LynxBaseTextShadowNode *child = [[LynxBaseTextShadowNode alloc] initWithSign:2 tagName:@"text"];
  child.text = @"3";
  child.textStyle.fontSize = childFontSize;
  [child layoutDidStart];

  NSAttributedString *result = [child generateAttributedString:parentAttributes
                                             withTextMaxLength:LynxNumberNotSet
                                                 withDirection:NSWritingDirectionNatural];
  NSNumber *resolvedStrokeWidth = [result attribute:NSStrokeWidthAttributeName
                                            atIndex:0
                                     effectiveRange:NULL];
  XCTAssertEqualWithAccuracy(resolvedStrokeWidth.doubleValue, -strokeWidth / childFontSize * 100,
                             0.0001);
}
@end
