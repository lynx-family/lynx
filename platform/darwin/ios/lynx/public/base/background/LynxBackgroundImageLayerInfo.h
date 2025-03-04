// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxBackgroundDrawable.h"
#import "LynxBackgroundUtils.h"
#pragma mark LynxBackgroundImageLayerInfo

NS_ASSUME_NONNULL_BEGIN
@interface LynxBackgroundImageLayerInfo : NSObject
// property item must be type of NSURL, LynxGradient or LynxBackgroundDrawable.
// the maintainability of this piece of code is low and need to be refactored as quickly as
// possible.
@property(atomic, nullable) LynxBackgroundDrawable* item;
@property(nonatomic, assign) CGRect paintingRect, clipRect, contentRect, borderRect, paddingRect;
@property(nonatomic, assign) LynxBackgroundOriginType backgroundOrigin;
@property(nonatomic, assign) LynxBackgroundRepeatType repeatXType;
@property(nonatomic, assign) LynxBackgroundRepeatType repeatYType;
@property(atomic, nullable) LynxBackgroundSize* backgroundSizeX;
@property(atomic, nullable) LynxBackgroundSize* backgroundSizeY;
@property(atomic, nullable) LynxBackgroundPosition* backgroundPosX;
@property(atomic, nullable) LynxBackgroundPosition* backgroundPosY;
@property(nonatomic, assign) LynxBackgroundClipType backgroundClip;
@property(nonatomic, assign) LynxCornerInsets cornerInsets;
- (void)drawInContext:(CGContextRef _Nullable)ctx;

/**
 This method prepares and initializes gradient layers for a gradient background effect.
 It uses both CAReplicateLayer and CAGradientLayer to create a dynamic, repeating gradient pattern.
 The layers stored in LynxGradientDrawable, structured as [ verticalRepeatLayer ->
 horizontalRepeatLayer -> gradientLayer ]. Add the `verticalRepeatLayer` to view tree to display the
 gradient.

 - Returns: BOOL indicating whether the gradient layers were prepared successfully or not.
 */
- (BOOL)prepareGradientLayers;

/**
  Creates and returns a CGMutablePath. Note: Callers must release the returned path to prevent
  memory leaks.
*/
- (CGPathRef)createClipPath;
@end

void LynxPathAddRoundedRect(CGMutablePathRef path, CGRect bounds, LynxCornerInsets ci);
NS_ASSUME_NONNULL_END
