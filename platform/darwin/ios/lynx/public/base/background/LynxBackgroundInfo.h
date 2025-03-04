// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUIUnitUtils.h>
#import <UIKit/UIKit.h>
#import "LynxCSSType.h"

@class LynxPlatformLength;
NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, LynxBorderValueUnit) {
  LynxBorderValueUnitDefault = 0,
  LynxBorderValueUnitPercent = 1,
  LynxBorderValueUnitCalc = 2,
};

typedef NS_ENUM(NSInteger, LynxBorderPosition) {
  LynxBorderTop = 0,
  LynxBorderLeft = 1,
  LynxBorderBottom = 2,
  LynxBorderRight = 3,
};

typedef struct _LynxBorderUnitValue {
  CGFloat val;
  LynxBorderValueUnit unit;
} LynxBorderUnitValue;

typedef struct {
  LynxBorderUnitValue topLeftX, topLeftY, topRightX, topRightY, bottomLeftX, bottomLeftY,
      bottomRightX, bottomRightY;
} LynxBorderRadii;

static inline bool isBorderUnitEqual(LynxBorderUnitValue lhs, LynxBorderUnitValue rhs) {
  return (lhs.val == rhs.val) && (lhs.unit == rhs.unit);
}

static inline bool isBorderUnitEqualA(LynxBorderUnitValue lhs, LynxBorderUnitValue rhs,
                                      LynxPlatformLength* lLength, LynxPlatformLength* rLength) {
  return isBorderUnitEqual(lhs, rhs) &&
         ((lLength && [lLength isEqual:rLength]) || (!lLength && !rLength));
}

@interface LynxBackgroundInfo : NSObject {
 @public
  LynxPlatformLength* borderRadiusCalc[8];
}

#pragma mark outline info
@property(nonatomic, assign) CGFloat outlineWidth;
@property(nonatomic, assign) LynxBorderStyle outlineStyle;
@property(nonatomic, nullable) UIColor* outlineColor;

#pragma mark border info
@property(nonatomic, nullable) UIColor* borderTopColor;
@property(nonatomic, nullable) UIColor* borderBottomColor;
@property(nonatomic, nullable) UIColor* borderLeftColor;
@property(nonatomic, nullable) UIColor* borderRightColor;

@property(nonatomic, nullable) UIColor* backgroundColor;

@property(nonatomic, assign) LynxBorderStyle borderTopStyle;
@property(nonatomic, assign) LynxBorderStyle borderBottomStyle;
@property(nonatomic, assign) LynxBorderStyle borderLeftStyle;
@property(nonatomic, assign) LynxBorderStyle borderRightStyle;

@property(nonatomic, assign) LynxBorderRadii borderRadius;
@property(nonatomic, assign) UIEdgeInsets borderWidth;
@property(nonatomic, assign) UIEdgeInsets paddingWidth;

#pragma mark background info
// TODO(fangzhou):remove these two properties after next merge
@property(nonatomic, assign) BOOL BGChangedNoneImage;
@property(nonatomic, assign) BOOL BGChangedImage;
@property(nonatomic, assign) BOOL borderChanged;

- (CGRect)getPaddingRect:(CGSize)size;
- (CGRect)getContentRect:(CGRect)paddingRect;
- (UIEdgeInsets)getPaddingInsets;
// TODO(fangzhou):move this function to draw module
- (UIImage*)getBorderLayerImageWithSize:(CGSize)viewSize;
- (CGPathRef)getBorderLayerPathWithSize:(CGSize)viewSize;

- (BOOL)updateOutlineWidth:(CGFloat)outlineWidth;
- (BOOL)updateOutlineColor:(UIColor*)outlineColor;
- (BOOL)updateOutlineStyle:(LynxBorderStyle)outlineStyle;

- (void)updateBorderColor:(LynxBorderPosition)position value:(UIColor*)color;
- (BOOL)updateBorderStyle:(LynxBorderPosition)position value:(LynxBorderStyle)style;

- (BOOL)hasBorder;
- (BOOL)isSimpleBorder;
- (BOOL)canUseBorderShapeLayer;
- (BOOL)hasDifferentBorderRadius;

- (void)makeCssDefaultValueToFitW3c;

@end  // LynxBackgroundInfo

NS_ASSUME_NONNULL_END
