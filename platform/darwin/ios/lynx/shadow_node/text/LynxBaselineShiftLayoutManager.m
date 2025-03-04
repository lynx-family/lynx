// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxBaselineShiftLayoutManager.h"

@implementation LynxBaselineShiftLayoutManager

- (instancetype)initWithVerticalAlign:(LynxVerticalAlign)verticalAlign {
  self = [super init];
  if (self) {
    self.verticalAlign = verticalAlign;
  }
  return self;
}

- (void)makeCenterOffset:(CGContextRef)ctx withFont:(UIFont *)font {
  CGFloat leading = font.leading;
  CGFloat ascent = font.ascender;
  CGFloat descent = font.descender;
  CGFloat capHeight = font.capHeight;
  CGFloat offset = 0.f;
  if (self.verticalAlign == LynxVerticalAlignMiddle && leading == 0.f) {
    offset = (-descent + ascent - capHeight) / 2.f - (ascent - capHeight);
  } else if (self.verticalAlign == LynxVerticalAlignTop) {
    offset = (capHeight - ascent);
  } else if (self.verticalAlign == LynxVerticalAlignBottom) {
    offset = -font.leading;
  }
  CGContextTranslateCTM(ctx, 0, offset);
}

- (void)showCGGlyphs:(const CGGlyph *)glyphs
           positions:(const CGPoint *)positions
               count:(NSInteger)glyphCount
                font:(UIFont *)font
          textMatrix:(CGAffineTransform)textMatrix
          attributes:(NSDictionary<NSAttributedStringKey, id> *)attributes
           inContext:(CGContextRef)CGContext {
  BOOL needOffset = self.verticalAlign != LynxVerticalAlignDefault;
  if (needOffset) {
    CGContextSaveGState(CGContext);
    [self makeCenterOffset:CGContext withFont:font];
  }
  [super showCGGlyphs:glyphs
            positions:positions
                count:glyphCount
                 font:font
           textMatrix:textMatrix
           attributes:attributes
            inContext:CGContext];
  if (needOffset) {
    CGContextRestoreGState(CGContext);
  }
}

@end
