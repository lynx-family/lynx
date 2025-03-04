// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxBackgroundUtils.h"
#import "LynxUI.h"

@implementation LynxBackgroundInfo {
  LynxBorderStyle _borderStyles[4];
}

@dynamic borderTopStyle;
@dynamic borderRightStyle;
@dynamic borderBottomStyle;
@dynamic borderLeftStyle;

- (instancetype)init {
  self = [super init];
  if (self) {
    _borderChanged = _BGChangedImage = _BGChangedNoneImage = NO;
    memset(_borderStyles, 0, sizeof(LynxBorderStyle) * 4);
    _outlineStyle = LynxBorderStyleNone;
    memset(borderRadiusCalc, 0, sizeof(LynxPlatformLength*) * 8);
  }
  return self;
}

#pragma mark outline info
- (void)setOutlineStyle:(LynxBorderStyle)outlineStyle {
  if (_outlineStyle != outlineStyle) {
    _outlineStyle = outlineStyle;
    _borderChanged = YES;
  }
}

- (void)setOutlineWidth:(CGFloat)outlineWidth {
  if (fabs(_outlineWidth - outlineWidth) > 1e-6) {
    _outlineWidth = outlineWidth;
    _borderChanged = YES;
  }
}

- (void)setOutlineColor:(UIColor*)color {
  if (![color isEqual:_outlineColor]) {
    _outlineColor = color;
    _borderChanged = YES;
  }
}

#pragma mark border info
- (UIImage*)getBorderLayerImageWithSize:(CGSize)viewSize {
  LynxBorderColors borderColors = {_borderTopColor.CGColor, _borderLeftColor.CGColor,
                                   _borderBottomColor.CGColor, _borderRightColor.CGColor};
  LynxBorderStyles borderStyles = {_borderStyles[LynxBorderTop], _borderStyles[LynxBorderLeft],
                                   _borderStyles[LynxBorderBottom], _borderStyles[LynxBorderRight]};
  return LynxGetBorderLayerImage(borderStyles, viewSize, _borderRadius, _borderWidth, borderColors,
                                 NO);
}

- (CGPathRef)getBorderLayerPathWithSize:(CGSize)viewSize {
  return LynxCreateBorderCenterPath(viewSize, _borderRadius, _borderWidth);
}

#pragma mark border colors
- (void)setBorderTopColor:(UIColor*)borderTopColor {
  if ((_borderTopColor && ![_borderTopColor isEqual:borderTopColor]) ||
      (!_borderTopColor && borderTopColor)) {
    _borderTopColor = borderTopColor;
    _borderChanged = YES;
  }
}

- (void)setBorderLeftColor:(UIColor*)borderLeftColor {
  if ((_borderLeftColor && ![_borderLeftColor isEqual:borderLeftColor]) ||
      (!_borderLeftColor && borderLeftColor)) {
    _borderLeftColor = borderLeftColor;
    _borderChanged = YES;
  }
}

- (void)setBorderBottomColor:(UIColor*)borderBottomColor {
  if ((_borderBottomColor && ![_borderBottomColor isEqual:borderBottomColor]) ||
      (!_borderBottomColor && borderBottomColor)) {
    _borderBottomColor = borderBottomColor;
    _borderChanged = YES;
  }
}

- (void)setBorderRightColor:(UIColor*)borderRightColor {
  if ((_borderRightColor && ![_borderRightColor isEqual:borderRightColor]) ||
      (!_borderRightColor && borderRightColor)) {
    _borderRightColor = borderRightColor;
    _borderChanged = YES;
  }
}

- (LynxBorderColors)borderColors {
  LynxBorderColors colors = {_borderTopColor ? _borderTopColor.CGColor : nil,
                             _borderLeftColor ? _borderLeftColor.CGColor : nil,
                             _borderBottomColor ? _borderBottomColor.CGColor : nil,
                             _borderRightColor ? _borderRightColor.CGColor : nil};
  return colors;
}

#pragma mark border style
- (void)setBorderLeftStyle:(LynxBorderStyle)borderStyle {
  if (_borderStyles[LynxBorderLeft] != borderStyle) {
    _borderStyles[LynxBorderLeft] = borderStyle;
    _borderChanged = YES;
  }
}

- (LynxBorderStyle)borderLeftStyle {
  return _borderStyles[LynxBorderLeft];
}

- (void)setBorderRightStyle:(LynxBorderStyle)borderStyle {
  if (_borderStyles[LynxBorderRight] != borderStyle) {
    _borderStyles[LynxBorderRight] = borderStyle;
    _borderChanged = YES;
  }
}

- (LynxBorderStyle)borderRightStyle {
  return _borderStyles[LynxBorderRight];
}

- (void)setBorderTopStyle:(LynxBorderStyle)borderStyle {
  if (_borderStyles[LynxBorderTop] != borderStyle) {
    _borderStyles[LynxBorderTop] = borderStyle;
    _borderChanged = YES;
  }
}

- (LynxBorderStyle)borderTopStyle {
  return _borderStyles[LynxBorderTop];
}

- (void)setBorderBottomStyle:(LynxBorderStyle)borderStyle {
  if (_borderStyles[LynxBorderBottom] != borderStyle) {
    _borderStyles[LynxBorderBottom] = borderStyle;
    _borderChanged = YES;
  }
}

- (LynxBorderStyle)borderBottomStyle {
  return _borderStyles[LynxBorderBottom];
}

- (void)makeCssDefaultValueToFitW3c {
  for (int i = 0; i < 4; i++) {
    _borderStyles[i] = LynxBorderStyleNone;
  }
  _borderWidth.bottom = _borderWidth.left = _borderWidth.right = _borderWidth.top =
      LynxBorderWidthMedium;
}

- (void)setBorderWidth:(UIEdgeInsets)borderWidth {
  if (!UIEdgeInsetsEqualToEdgeInsets(_borderWidth, borderWidth)) {
    _borderWidth = borderWidth;
    _borderChanged = YES;
  }
}

- (void)setBorderRadius:(LynxBorderRadii)borderRadius {
  _borderRadius = borderRadius;
  _borderChanged = _BGChangedNoneImage = _BGChangedImage = YES;
}

- (BOOL)updateOutlineWidth:(CGFloat)outlineWidth {
  if (outlineWidth != _outlineWidth) {
    _outlineWidth = outlineWidth;
    return YES;
  }
  return NO;
}

- (BOOL)updateOutlineColor:(UIColor*)outlineColor {
  if (outlineColor != _outlineColor) {
    _outlineColor = outlineColor;
    return YES;
  }
  return NO;
}

- (BOOL)updateOutlineStyle:(LynxBorderStyle)outlineStyle {
  if (outlineStyle != _outlineStyle) {
    _outlineStyle = outlineStyle;
    return YES;
  }
  return NO;
}

- (void)updateBorderColor:(LynxBorderPosition)position value:(UIColor*)color {
  switch (position) {
    case LynxBorderTop:
      [self setBorderTopColor:color];
      break;
    case LynxBorderLeft:
      [self setBorderLeftColor:color];
      break;
    case LynxBorderBottom:
      [self setBorderBottomColor:color];
      break;
    case LynxBorderRight:
      [self setBorderRightColor:color];
      break;
  }
}

- (BOOL)updateBorderStyle:(LynxBorderPosition)position value:(LynxBorderStyle)style {
  if (_borderStyles[position] != style) {
    _borderStyles[position] = style;
    _borderChanged = YES;
    return YES;
  }
  return NO;
}

#pragma mark get functions
- (CGRect)getPaddingRect:(CGSize)size {
  CGRect paddingRect = {.size = size};
  paddingRect.origin.x += _borderWidth.left;
  paddingRect.origin.y += _borderWidth.top;
  paddingRect.size.width -= (_borderWidth.left + _borderWidth.right);
  paddingRect.size.height -= (_borderWidth.top + _borderWidth.bottom);
  return paddingRect;
}

- (CGRect)getContentRect:(CGRect)paddingRect {
  CGRect contentRect = paddingRect;
  contentRect.origin.x += _paddingWidth.left;
  contentRect.origin.y += _paddingWidth.top;
  contentRect.size.width -= (_paddingWidth.left + _paddingWidth.right);
  contentRect.size.height -= (_paddingWidth.top + _paddingWidth.bottom);
  return contentRect;
}

- (UIEdgeInsets)getPaddingInsets {
  const UIEdgeInsets paddingInsets = {
      .top = _borderWidth.top + _paddingWidth.top,
      .right = _borderWidth.right + _paddingWidth.right,
      .bottom = _borderWidth.bottom + _paddingWidth.bottom,
      .left = _borderWidth.left + _paddingWidth.left,
  };
  return paddingInsets;
}

- (LynxBorderRadii)getBorderRadius {
  return _borderRadius;
}

- (UIEdgeInsets)getBorderWidth {
  return _borderWidth;
}

- (BOOL)hasCoincidentBorderColors {
  if (!_borderTopColor && !_borderBottomColor && !_borderLeftColor && !_borderRightColor) {
    return YES;
  }
  if (_borderTopColor && _borderBottomColor && _borderLeftColor && _borderRightColor) {
    return [_borderTopColor isEqual:_borderBottomColor] &&
           [_borderRightColor isEqual:_borderLeftColor] &&
           [_borderTopColor isEqual:_borderRightColor];
  }
  return NO;
}

- (BOOL)hasIdenticalBorderStyles {
  BOOL isIdentical = YES;
  for (int i = 0; i < 3; i++) {
    if (_borderStyles[i] != _borderStyles[i + 1]) {
      isIdentical = NO;
      break;
    }
  }
  return isIdentical;
}

- (BOOL)hasIdenticalBorderWidths {
  return _borderWidth.left == _borderWidth.top && _borderWidth.left == _borderWidth.right &&
         _borderWidth.left == _borderWidth.bottom;
}

- (BOOL)hasBorder {
  return (_borderWidth.left > 0) || (_borderWidth.top > 0) || (_borderWidth.right > 0) ||
         (_borderWidth.bottom > 0);
}

- (BOOL)hasDifferentBorderRadius {
  return !internalHasSameBorderRadius(_borderRadius);
}

- (BOOL)isSimpleBorder {
  if (![self hasIdenticalBorderWidths]) {
    return NO;
  }

  if (_borderWidth.bottom <= 1e-3) {
    return YES;
  }

  if (![self hasIdenticalBorderStyles]) {
    return NO;
  }

  if (_borderStyles[LynxBorderLeft] != LynxBorderStyleNone &&
      _borderStyles[LynxBorderLeft] != LynxBorderStyleHidden) {
    if (_borderWidth.bottom < 1) {
      return NO;
    }
    if (_borderStyles[LynxBorderLeft] != LynxBorderStyleSolid) {
      return NO;
    }
    if (![self hasCoincidentBorderColors]) {
      // normal layer do not support border with different color
      return NO;
    }

    if ([self hasDifferentBorderRadius]) {
      // normal layer do not support different radius
      return NO;
    }
  }

  return YES;
}

- (BOOL)canUseBorderShapeLayer {
  // 1. identical border styles && identical border widths && identical border colors && border
  // styles are all solid.
  // TODO(renzhongyue): support more styles.
  return _borderStyles[LynxBorderLeft] == LynxBorderStyleSolid && [self hasIdenticalBorderStyles] &&
         [self hasCoincidentBorderColors] && [self hasIdenticalBorderWidths];
}

#pragma mark background info
- (void)setBackgroundColor:(UIColor*)backgroundColor {
  if (_backgroundColor != backgroundColor) {
    _backgroundColor = backgroundColor;
    _BGChangedNoneImage = YES;
  }
}

@end  // LynxBackgroundInfo
