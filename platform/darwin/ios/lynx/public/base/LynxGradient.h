// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

#import "LynxCSSType.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, LynxLinearGradientDirection) {
  LynxLinearGradientDirectionNone = 0,
  LynxLinearGradientDirectionToTop,
  LynxLinearGradientDirectionToBottom,
  LynxLinearGradientDirectionToLeft,
  LynxLinearGradientDirectionToRight,
  LynxLinearGradientDirectionToTopRight,
  LynxLinearGradientDirectionToTopLeft,
  LynxLinearGradientDirectionToBottomRight,
  LynxLinearGradientDirectionToBottomLeft,
  LynxLinearGradientDirectionAngle
};

typedef NS_ENUM(NSInteger, LynxRadialCenterType) {
  LynxRadialCenterTypePercentage = 11,
  LynxRadialCenterTypeRPX = 6,
  LynxRadialCenterTypePX = 5,
};

@interface LynxGradient : NSObject
@property(nonatomic, nullable) NSMutableArray* colors;
@property(nonatomic, nullable) CGFloat* positions;
@property(nonatomic, assign) NSUInteger positionCount;
- (instancetype)initWithColors:(NSArray<NSNumber*>*)colors stops:(NSArray<NSNumber*>*)stops;
- (void)draw:(CGContextRef)context withPath:(CGPathRef)path;
- (void)draw:(CGContextRef)context withRect:(CGRect)pathRect;
- (BOOL)isEqualTo:(LynxGradient*)rhs;
@end

@interface LynxLinearGradient : LynxGradient
@property(nonatomic, assign) double angle;
@property(nonatomic, assign) LynxLinearGradientDirection directionType;
- (instancetype)initWithArray:(NSArray*)arr;
/**
 * Computes the start and end points based on the provided size.
 *
 * @param[out] startPoint The calculated start point. Cannot be NULL.
 * @param[out] endPoint The calculated end point. Cannot be NULL.
 * @param[in] size The dimensions used for the calculation. Cannot be NULL.
 */
- (void)computeStartPoint:(CGPoint* _Nonnull)startPoint
              andEndPoint:(CGPoint* _Nonnull)endPoint
                 withSize:(const CGSize* _Nonnull)size;
@end

@interface LynxRadialGradient : LynxGradient
@property(nonatomic, assign) LynxRadialCenterType centerX;
@property(nonatomic, assign) LynxRadialCenterType centerY;
@property(nonatomic, assign) CGFloat centerXValue;
@property(nonatomic, assign) CGFloat centerYValue;
@property(nonatomic, assign) CGPoint at;
@property(nonatomic, assign) LynxRadialGradientShapeType shape;
@property(nonatomic, assign) LynxRadialGradientSizeType shapeSize;
@property(nonatomic, assign) CGFloat shapeSizeXValue;
@property(nonatomic, assign) LynxPlatformLengthUnit shapeSizeXUnit;
@property(nonatomic, assign) CGFloat shapeSizeYValue;
@property(nonatomic, assign) LynxPlatformLengthUnit shapeSizeYUnit;
- (instancetype)initWithArray:(NSArray*)arr;
- (CGPoint)calculateCenterWithWidth:(CGFloat)width andHeight:(CGFloat)height;
- (CGPoint)calculateRadiusWithCenter:(const CGPoint* _Nonnull)center
                               sizeX:(CGFloat)width
                               sizeY:(CGFloat)height;
@end

BOOL LynxSameLynxGradient(LynxGradient* _Nullable left, LynxGradient* _Nullable right);

NS_ASSUME_NONNULL_END
