// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTextLayoutManager.h"
#import <CoreText/CoreText.h>
#import "LynxTextStyle.h"
#import "LynxUI+Internal.h"
#import "LynxUIUnitUtils.h"

@implementation LynxTextLayoutManager

- (void)drawTextClipMaskWithFont:(UIFont *_Nonnull)font
                      glyphCount:(NSUInteger)glyphCount
                          glyphs:(const CGGlyph *_Nonnull)glyphs
                     maskContext:(CGContextRef)maskContext
                     mutableAttr:(NSMutableDictionary *)mutableAttr
                       positions:(const CGPoint *_Nonnull)positions
                            size:(const CGSize *)size
                      textMatrix:(const CGAffineTransform *)textMatrix
                    adjustOffset:(const CGPoint *)adjustOffset {
  CGFontRef fontRef = CGFontCreateWithFontName((CFStringRef)font.fontName);
  CGContextSetFont(maskContext, fontRef);
  CGContextSetTextMatrix(maskContext, *textMatrix);
  CGContextTranslateCTM(maskContext, 0, size->height);
  CGContextScaleCTM(maskContext, 1, -1);
  CGContextSetFontSize(maskContext, font.pointSize);
  CGContextSetFillColorWithColor(maskContext, [[UIColor blackColor] CGColor]);
  ///  _________________
  ///  |    | offset y
  ///  | -------------------
  ///  | x  | text content
  ///
  ///  After rendering, the layer position will translate -offset.x and -offset.y.
  ///  The text's origin has changed outside with offset set.
  ///  Move it back to ensure all glyphs rendered correctly on newly created context.
  ///
  ///  Then we got a text mask without offset
  ///
  ///   ________________
  ///  | text content
  ///  |________________
  ///
  CGContextTranslateCTM(maskContext, -_overflowOffset.x + adjustOffset->x,
                        -_overflowOffset.y + adjustOffset->y);
  [super showCGGlyphs:glyphs
            positions:positions
                count:glyphCount
                 font:font
               matrix:*textMatrix
           attributes:mutableAttr
            inContext:maskContext];
  CGFontRelease(fontRef);
}

- (void)showCGGlyphs:(const CGGlyph *)glyphs
           positions:(const CGPoint *)positions
               count:(NSUInteger)glyphCount
                font:(UIFont *)font
              matrix:(CGAffineTransform)textMatrix
          attributes:(NSDictionary<NSAttributedStringKey, id> *)attributes
           inContext:(CGContextRef)graphicsContext {
  if ([attributes objectForKey:LynxTextColorGradientKey] != nil &&
      // iOS unicode emoji use System AppleColorEmojiUI font
      // So here can use font name to check if sub glyphs is emoji
      ![font.fontName isEqualToString:@".AppleColorEmojiUI"]) {
    NSMutableDictionary *mutableAttr = [attributes mutableCopy];

    if ([mutableAttr objectForKey:NSShadowAttributeName] != nil) {
      // If text has shadow, need to draw shadow first
      // We use transparent text color to make sure only shadow and background is rendered
      UIColor *color = [attributes objectForKey:NSForegroundColorAttributeName];

      [mutableAttr setObject:[UIColor clearColor] forKey:NSForegroundColorAttributeName];

      [super showCGGlyphs:glyphs
                positions:positions
                    count:glyphCount
                     font:font
                   matrix:textMatrix
               attributes:mutableAttr
                inContext:graphicsContext];

      [mutableAttr removeObjectForKey:NSShadowAttributeName];
      if (color) {
        [mutableAttr setObject:color forKey:NSForegroundColorAttributeName];
      } else {
        [mutableAttr removeObjectForKey:NSForegroundColorAttributeName];
      }
    }
    // part of this text contains gradient
    LynxGradient *gradient = [mutableAttr objectForKey:LynxTextColorGradientKey];
    CGRect rect = [self usedRectForTextContainer:self.textContainers[0]];
    // if positions[0] != rect.origin this means there are offsetX outside NSLayoutManager
    CGSize size = CGSizeMake(MAX(rect.origin.x, positions[0].x) + self.textBoundingRectSize.width,
                             rect.origin.y + self.textBoundingRectSize.height);

    // It's necessary to add some extra size to `maskImage`; otherwise, there could be scenarios
    // where `maskImage` cannot completely clip the source content. Use `_overflowOffset` as the
    // extra size directly.
    CGPoint adjustOffset = _overflowOffset;
    CGSize maskImageSize =
        CGSizeMake(size.width + adjustOffset.x * 2, size.height + adjustOffset.y * 2);

    UIImage *image = [LynxUI
        imageWithActionBlock:^(CGContextRef _Nonnull context) {
          [self drawTextClipMaskWithFont:font
                              glyphCount:glyphCount
                                  glyphs:glyphs
                             maskContext:context
                             mutableAttr:mutableAttr
                               positions:positions
                                    size:&maskImageSize
                              textMatrix:&textMatrix
                            adjustOffset:&adjustOffset];
        }
                      opaque:NO
                       scale:[LynxUIUnitUtils screenScale]
                        size:maskImageSize];

    // When drawing encountered an error, image will be nil. Protect here.
    if (!image) {
      return;
    }

    // clip a mask for this
    CGContextSaveGState(graphicsContext);
    // restore the position offset.
    CGContextTranslateCTM(graphicsContext, _overflowOffset.x, _overflowOffset.y);
    CGContextClipToMask(
        graphicsContext,
        CGRectMake(-adjustOffset.x, -adjustOffset.y, maskImageSize.width, maskImageSize.height),
        [image CGImage]);
    [gradient draw:graphicsContext withRect:CGRectMake(0, 0, size.width, size.height)];
    CGContextRestoreGState(graphicsContext);
  } else {
    [super showCGGlyphs:glyphs
              positions:positions
                  count:glyphCount
                   font:font
                 matrix:textMatrix
             attributes:attributes
              inContext:graphicsContext];
  }
}

- (void)drawBackgroundForGlyphRange:(NSRange)glyphsToShow atPoint:(CGPoint)origin {
  [self
      enumerateLineFragmentsForGlyphRange:glyphsToShow
                               usingBlock:^(CGRect rect, CGRect usedRect,
                                            NSTextContainer *_Nonnull textContainer,
                                            NSRange glyphRange, BOOL *_Nonnull stop) {
                                 NSRange lineRange = [self characterRangeForGlyphRange:glyphRange
                                                                      actualGlyphRange:nil];
                                 NSMutableDictionary *inlineBackgroundDic =
                                     [[NSMutableDictionary alloc] init];
                                 [self.textStorage
                                     enumerateAttribute:LynxInlineBackgroundKey
                                                inRange:lineRange
                                                options:
                                                    NSAttributedStringEnumerationLongestEffectiveRangeNotRequired
                                             usingBlock:^(id _Nullable value, NSRange range,
                                                          BOOL *_Nonnull stop) {
                                               if (value != nil) {
                                                 NSValue *backgroundRange =
                                                     [inlineBackgroundDic objectForKey:value];
                                                 if (backgroundRange == nil) {
                                                   [inlineBackgroundDic
                                                       setObject:[NSValue valueWithRange:range]
                                                          forKey:value];
                                                 } else {
                                                   NSRange originRange =
                                                       [backgroundRange rangeValue];
                                                   NSRange newRange = NSMakeRange(
                                                       MIN(originRange.location, range.location),
                                                       originRange.length + range.length);
                                                   [inlineBackgroundDic
                                                       setObject:[NSValue valueWithRange:newRange]
                                                          forKey:value];
                                                 }
                                               }
                                             }];

                                 for (NSArray *inlineBackground in inlineBackgroundDic) {
                                   NSRange inlineTextRange = [[inlineBackgroundDic
                                       objectForKey:inlineBackground] rangeValue];
                                   LynxBackgroundDrawable *backgroundDrawable =
                                       inlineBackground.firstObject;
                                   NSUInteger inlineTextStartIndex = inlineTextRange.location;
                                   NSUInteger inlineTextEndIndex =
                                       inlineTextRange.location + inlineTextRange.length;
                                   CGFloat startPosition = usedRect.origin.x;
                                   CGFloat endPosition = usedRect.origin.x + usedRect.size.width;
                                   if (inlineTextStartIndex > lineRange.location) {
                                     startPosition = [self locationForGlyphAtIndex:
                                                               [self glyphIndexForCharacterAtIndex:
                                                                         inlineTextStartIndex]]
                                                         .x;
                                   }
                                   if (inlineTextEndIndex < lineRange.location + lineRange.length) {
                                     endPosition = [self locationForGlyphAtIndex:
                                                             [self glyphIndexForCharacterAtIndex:
                                                                       inlineTextEndIndex]]
                                                       .x;
                                   }
                                   if ([backgroundDrawable respondsToSelector:@selector
                                                           (drawTextBackgroundInContext:
                                                                            contentRect:)]) {
                                     CGContextRef currentContext = UIGraphicsGetCurrentContext();

                                     CGContextSaveGState(currentContext);
                                     CGRect resultRect = CGRectMake(
                                         origin.x + MIN(startPosition, endPosition),
                                         origin.y + rect.origin.y, ABS(endPosition - startPosition),
                                         rect.size.height);
                                     CGPathRef path = [LynxBackgroundUtils
                                         createBezierPathWithRoundedRect:resultRect
                                                             borderRadii:backgroundDrawable
                                                                             .borderRadius
                                                              edgeInsets:UIEdgeInsetsZero];

                                     CGContextAddPath(currentContext, path);
                                     CGContextClip(currentContext);
                                     CGPathRelease(path);
                                     [backgroundDrawable drawTextBackgroundInContext:currentContext
                                                                         contentRect:resultRect];
                                     CGContextRestoreGState(currentContext);
                                   }
                                 }
                               }];
  [super drawBackgroundForGlyphRange:glyphsToShow atPoint:origin];
}

@end
