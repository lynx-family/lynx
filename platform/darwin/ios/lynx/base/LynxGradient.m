// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "LynxGradient.h"
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "LynxCSSType.h"
#import "LynxColorUtils.h"
#import "LynxConverter+UI.h"
#import "LynxGradientUtils.h"

#pragma mark LynxGradient

@implementation LynxGradient {
}

- (instancetype)initWithColors:(NSArray<NSNumber*>*)colors stops:(NSArray<NSNumber*>*)stops {
  self = [super init];
  if (self) {
    NSUInteger count = [colors count];
    self.positionCount = [stops count];
    self.colors = [NSMutableArray array];
    if (self.positionCount == count) {
      self.positions = malloc(count * sizeof(CGFloat));
    } else {
      self.positions = nil;
    }

    for (NSUInteger i = 0; i < count; i++) {
      [self.colors addObject:[LynxConverter toUIColor:colors[i]]];
      if (self.positions) {
        self.positions[i] = [LynxConverter toCGFloat:stops[i]] / 100.0;
        // Color stops should be listed in ascending order
        if (i >= 1 && self.positions[i] < self.positions[i - 1]) {
          self.positions[i] = self.positions[i - 1];
        }
      }
    }
  }
  return self;
}

- (BOOL)isEqualTo:(LynxGradient*)rhs {
  if (![_colors isEqual:rhs.colors]) {
    return false;
  }

  bool hasPosition = _positions != nil;
  bool rhsHasPositon = rhs.positions != nil;

  if (hasPosition != rhsHasPositon) {
    return false;
  }
  // both position is empty
  if (!hasPosition) {
    return true;
  }

  return memcmp(_positions, rhs.positions, [_colors count] * sizeof(CGFloat)) == 0;
}

- (void)draw:(CGContextRef)context withPath:(CGPathRef)path {
}

- (void)draw:(CGContextRef)context withRect:(CGRect)path {
}

- (void)dealloc {
  if (_positions) {
    free(_positions);
    _positions = NULL;
  }
}

@end

#pragma mark LynxBackgroundLinearGradient
@implementation LynxLinearGradient {
}

- (void)computeStartPoint:(CGPoint* _Nonnull)startPoint
              andEndPoint:(CGPoint* _Nonnull)endPoint
                 withSize:(const CGSize* _Nonnull)size {
  CGPoint m, center;
  float s, c, t;
  const int w = MAX(size->width, 1), h = MAX(size->height, 1);
  // diagonal line is 0.5
  const float mul = 2.0f * w * h / (w * w + h * h);
  switch (_directionType) {
    case LynxLinearGradientDirectionToTop:
      *startPoint = CGPointMake(0, h);
      *endPoint = CGPointMake(0, 0);
      break;
    case LynxLinearGradientDirectionToBottom:
      *startPoint = CGPointMake(0, 0);
      *endPoint = CGPointMake(0, h);
      break;
    case LynxLinearGradientDirectionNone:
      *startPoint = CGPointMake(0, 0);
      *endPoint = CGPointMake(0, h);
      break;
    case LynxLinearGradientDirectionToLeft:
      *startPoint = CGPointMake(w, 0);
      *endPoint = CGPointMake(0, 0);
      break;
    case LynxLinearGradientDirectionToRight:
      *startPoint = CGPointMake(0, 0);
      *endPoint = CGPointMake(w, 0);
      break;
    case LynxLinearGradientDirectionToTopLeft:
      *startPoint = CGPointMake(h * mul, w * mul);
      *endPoint = CGPointMake(0, 0);
      break;
    case LynxLinearGradientDirectionToBottomRight:
      *startPoint = CGPointMake(0, 0);
      *endPoint = CGPointMake(h * mul, w * mul);
      break;
    case LynxLinearGradientDirectionToTopRight:
      *startPoint = CGPointMake(w - h * mul, w * mul);
      *endPoint = CGPointMake(w, 0);
      break;
    case LynxLinearGradientDirectionToBottomLeft:
      *startPoint = CGPointMake(w, 0);
      *endPoint = CGPointMake(w - h * mul, w * mul);
      break;
    case LynxLinearGradientDirectionAngle:
      center = CGPointMake(w / 2, h / 2);
      s = sin(_angle);
      c = cos(_angle);
      t = tan(_angle);
      if (s >= 0 && c >= 0) {
        m = CGPointMake(w, 0);
      } else if (s >= 0 && c < 0) {
        m = CGPointMake(w, h);
      } else if (s < 0 && c < 0) {
        m = CGPointMake(0, h);
      } else {
        m = CGPointMake(0, 0);
      }
      *endPoint =
          CGPointMake(center.x + s * (center.y - m.y - t * center.x + t * m.x) / (s * t + c),
                      center.y - (center.y - m.y - t * center.x + t * m.x) / (t * t + 1));
      *startPoint = CGPointMake(2 * center.x - endPoint->x, 2 * center.y - endPoint->y);
      break;
  }
}

- (instancetype)initWithArray:(NSArray*)arr {
  self = [super initWithColors:arr[1] stops:arr[2]];
  if (self) {
    // [angle, color, stop, side-or-corner]
    // The parsed value from old css style from binary code (e.g CSSParser) don't have the last
    // field. All values should be treated as <angle>.
    self.directionType = arr.count == 4 ? [arr[3] intValue] : LynxLinearGradientDirectionAngle;
    self.angle = [arr[0] doubleValue] * M_PI / 180.0;
  }
  return self;
}

- (void)draw:(CGContextRef)context withRect:(CGRect)pathRect {
  NSMutableArray* ar = [NSMutableArray array];
  for (UIColor* c in self.colors) {
    [ar addObject:(id)c.CGColor];
  }
  CGColorSpaceRef colorSpace = CGColorGetColorSpace([[self.colors lastObject] CGColor]);
  CGGradientRef gradient = CGGradientCreateWithColors(colorSpace, (CFArrayRef)ar, self.positions);
  CGPoint start, end;
  [self computeStartPoint:&start andEndPoint:&end withSize:&pathRect.size];
  CGContextDrawLinearGradient(
      context, gradient, start, end,
      kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation);
  CGGradientRelease(gradient);
}

- (void)draw:(CGContextRef)context withPath:(CGPathRef)path {
  CGContextAddPath(context, path);
  CGContextClip(context);
  CGRect pathRect = CGPathGetBoundingBox(path);
  [self draw:context withRect:pathRect];
}

- (BOOL)isEqualTo:(LynxGradient*)object {
  if (![object isKindOfClass:[LynxLinearGradient class]]) {
    return false;
  }
  LynxLinearGradient* rhs = (LynxLinearGradient*)object;
  return [super isEqualTo:rhs] && self.angle == rhs.angle;
}

@end

#pragma mark LynxBackgroundRadialGradient
@implementation LynxRadialGradient {
}

- (instancetype)initWithArray:(NSArray*)arr {
  self = [super initWithColors:arr[1] stops:arr[2]];
  if (self) {
    NSArray* shapeSize = arr[0];
    _shape = [shapeSize[0] unsignedIntValue];
    _shapeSize = [shapeSize[1] unsignedIntValue];

    _at = CGPointMake(0.5, 0.5);
    // [x-position-type, x-position, y-position-type y-position]
    self.centerX = [shapeSize[2] integerValue];
    self.centerXValue = [shapeSize[3] floatValue];
    self.centerY = [shapeSize[4] integerValue];
    self.centerYValue = [shapeSize[5] floatValue];

    if (_shapeSize == LynxRadialGradientSizeLength) {
      self.shapeSizeXValue = [shapeSize[10] floatValue];
      self.shapeSizeXUnit = [shapeSize[11] integerValue];
      self.shapeSizeYValue = [shapeSize[12] floatValue];
      self.shapeSizeYUnit = [shapeSize[13] integerValue];
    }
  }
  return self;
}

- (CGPoint)calculateRadiusWithCenter:(const CGPoint* _Nonnull)center
                               sizeX:(CGFloat)width
                               sizeY:(CGFloat)height {
  CGPoint radius;

  if (_shapeSize == LynxRadialGradientSizeLength) {
    float x = self.shapeSizeXUnit == LynxPlatformLengthUnitPercentage ? width * self.shapeSizeXValue
                                                                      : self.shapeSizeXValue;
    float y = self.shapeSizeYUnit == LynxPlatformLengthUnitPercentage
                  ? height * self.shapeSizeYValue
                  : self.shapeSizeYValue;
    radius = CGPointMake(x, y);
  } else {
    radius = [LynxGradientUtils getRadialRadiusWithShape:_shape
                                               shapeSize:_shapeSize
                                                 centerX:center->x
                                                 centerY:center->y
                                                   sizeX:width
                                                   sizeY:height];
  }
  return radius;
}

- (void)draw:(CGContextRef)context withRect:(CGRect)pathRect {
  NSMutableArray* ar = [NSMutableArray array];
  for (UIColor* c in self.colors) {
    [ar addObject:(id)c.CGColor];
  }
  CGColorSpaceRef colorSpace = CGColorGetColorSpace([[self.colors lastObject] CGColor]);
  CGGradientRef gradient = CGGradientCreateWithColors(colorSpace, (CFArrayRef)ar, self.positions);
  int w = pathRect.size.width, h = pathRect.size.height;

  CGPoint center = [self calculateCenterWithWidth:w andHeight:h];
  CGPoint radius = [self calculateRadiusWithCenter:&center sizeX:w sizeY:h];

  bool hasZero = !radius.x || !radius.y;
  float aspectRatio = hasZero ? 1 : radius.x / radius.y;

  if (aspectRatio != 1) {
    CGContextSaveGState(context);
    CGContextTranslateCTM(context, center.x, center.y);
    CGContextScaleCTM(context, 1, 1 / aspectRatio);
    CGContextTranslateCTM(context, -center.x, -center.y);
  }

  CGContextDrawRadialGradient(
      context, gradient, center, 0, center, hasZero ? 0 : radius.x,
      kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation);
  if (aspectRatio != 1) {
    CGContextRestoreGState(context);
  }
  CGGradientRelease(gradient);
}

- (void)draw:(CGContextRef)context withPath:(CGPathRef)path {
  CGContextAddPath(context, path);
  CGContextClip(context);
  CGRect pathRect = CGPathGetBoundingBox(path);
  [self draw:context withRect:pathRect];
}

- (BOOL)isEqualTo:(LynxGradient*)object {
  if (![object isKindOfClass:[LynxRadialGradient class]]) {
    return false;
  }
  LynxRadialGradient* rhs = (LynxRadialGradient*)object;
  return [super isEqualTo:rhs] && CGPointEqualToPoint(self.at, rhs.at);
}

- (CGPoint)calculateCenterWithWidth:(CGFloat)width andHeight:(CGFloat)height {
  _at.x = [self calculateValue:self.centerX value:self.centerXValue base:width];
  _at.y = [self calculateValue:self.centerY value:self.centerYValue base:height];
  return _at;
}

- (CGFloat)calculateValue:(NSInteger)type value:(CGFloat)value base:(CGFloat)base {
  switch (type) {
    case -LynxBackgroundPositionCenter:
      return base * 0.5;
    case -LynxBackgroundPositionTop:
    case -LynxBackgroundPositionLeft:
      return 0.0;
    case -LynxBackgroundPositionRight:
    case -LynxBackgroundPositionBottom:
      return base;
    case LynxRadialCenterTypePercentage:
      return base * value / 100.0;
    default:
      // TODO handle REM RPX or other length type
      return value;
  }
}

@end

BOOL LynxSameLynxGradient(LynxGradient* _Nullable left, LynxGradient* _Nullable right) {
  if (left == nil && right == nil) {
    return YES;
  }
  if (left == nil || right == nil) {
    return NO;
  }
  return [left isEqualTo:right];
}
