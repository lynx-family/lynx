// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <CoreFoundation/CoreFoundation.h>
#import "LynxBackgroundInfo.h"
#import "LynxCSSType.h"
#import "LynxGradient.h"

#import "LynxUIUnitUtils.h"

@interface LynxBackgroundSize : NSObject

@property(nonatomic, strong) LynxPlatformLength* _Nullable value;

- (instancetype _Nullable)initWithLength:(LynxPlatformLength* _Nullable)value;

- (BOOL)isCover;

- (BOOL)isContain;

- (BOOL)isAuto;

- (CGFloat)apply:(CGFloat)parentValue currentValue:(CGFloat)currentValue;

@end

@interface LynxBackgroundPosition : NSObject

@property(nonatomic, strong) LynxPlatformLength* _Nullable value;

- (instancetype _Nullable)initWithValue:(LynxPlatformLength* _Nullable)value;

- (CGFloat)apply:(CGFloat)avaiableValue;

@end

@interface LynxBackgroundDrawable : NSObject
@property(nonatomic, assign, readonly) LynxBackgroundImageType type;
@property(nonatomic, assign) LynxBackgroundRepeatType repeatX;
@property(nonatomic, assign) LynxBackgroundRepeatType repeatY;
@property(nonatomic, assign) LynxBackgroundClipType clip;
@property(nonatomic, assign) LynxBackgroundOriginType origin;

@property(nonatomic, strong, nullable) LynxBackgroundPosition* posX;
@property(nonatomic, strong, nullable) LynxBackgroundPosition* posY;
@property(nonatomic, strong, nullable) LynxBackgroundSize* sizeX;
@property(nonatomic, strong, nullable) LynxBackgroundSize* sizeY;

@property(nonatomic, assign) CGRect bounds;

// inline text background support radius
@property(nonatomic, assign) LynxBorderRadii borderRadius;

- (CGFloat)getImageWidth;
- (CGFloat)getImageHeight;

- (void)drawInContext:(CGContextRef _Nonnull)ctx
           borderRect:(CGRect)borderRect
          paddingRect:(CGRect)paddingRect
          contentRect:(CGRect)contentRect;

- (void)drawTextBackgroundInContext:(CGContextRef _Nonnull)ctx contentRect:(CGRect)contentRect;

/**
 Computes the background size based on the dimensions of the background image and the paint box.

 @param imageSize The size of the background image.
 @param paintBoxSize The size of the paint box.
 @return The computed background size.
 */
- (CGSize)computeBackgroundSizeWithImageSize:(const CGSize* _Nonnull)imageSize
                             andPaintBoxSize:(const CGSize* _Nonnull)paintBoxSize;

- (void)computeBackgroundPosition:(CGFloat* _Nonnull)offsetX
                          offsetY:(CGFloat* _Nonnull)offsetY
                         paintBox:(const CGRect)paintBox
                             size:(const CGSize)size;
@end

@interface LynxBackgroundImageDrawable : LynxBackgroundDrawable
@property(nonatomic, strong, nullable) NSURL* url;
@property(atomic, strong, nullable) UIImage* image;

- (instancetype _Nullable)initWithString:(NSString* _Nullable)string;
- (instancetype _Nullable)initWithURL:(NSURL* _Nullable)url;
@end

@interface LynxBackgroundGradientDrawable : LynxBackgroundDrawable
@property(nonatomic, strong, nullable) LynxGradient* gradient;

// The following setup employs three layers to achieve a gradient with a background-repeat effect.
// The 'gradientLayer' applies the initial gradient which is then replicated along the x-axis by
// 'horizontalRepeatLayer'. Subsequently, 'verticalRepeatLayer' repeats this pattern along the
// y-axis. The 'verticalRepeatLayer' eventually serves as a sublayer to the 'backgroundLayer'.
@property(nonatomic, strong, nullable) CAReplicatorLayer* horizontalRepeatLayer;
@property(nonatomic, strong, nullable) CAReplicatorLayer* verticalRepeatLayer;
@property(nonatomic, strong, nullable) CAGradientLayer* gradientLayer;

- (void)prepareGradientWithBorderBox:(CGRect)borderBox
                         andPaintBox:(CGRect)paintBox
                         andClipRect:(CGRect)clipRect;
- (void)onPrepareGradientWithSize:(CGSize)gradientSize;
@end

@interface LynxBackgroundLinearGradientDrawable : LynxBackgroundGradientDrawable
- (instancetype _Nullable)initWithArray:(NSArray* _Nonnull)array;
@end

@interface LynxBackgroundRadialGradientDrawable : LynxBackgroundGradientDrawable
- (instancetype _Nullable)initWithArray:(NSArray* _Nonnull)array;
@end

@interface LynxBackgroundNoneDrawable : LynxBackgroundDrawable
@end
