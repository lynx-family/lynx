// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "LynxBackgroundCapInsets.h"
#import "LynxBackgroundDrawable.h"
#import "LynxBackgroundInfo.h"
#import "LynxCSSType.h"
#import "LynxConverter.h"
#import "LynxError.h"
#import "LynxGradient.h"

NS_ASSUME_NONNULL_BEGIN

NSString* NSStringFromLynxBorderRadii(LynxBorderRadii* radii);

typedef NS_ENUM(NSInteger, LynxAnimOpts) {
  LynxAnimOptHasBorderLayer = (1 << 0),
  LynxAnimOptHasBorderComplex = (1 << 1),
  LynxAnimOptHasBGLayer = (1 << 2),
  LynxAnimOptHasBGComplex = (1 << 3),
};

typedef NS_ENUM(uint8_t, LynxBgTypes) {
  LynxBgTypeSimple,
  LynxBgTypeShape,
  LynxBgTypeComplex,
};

typedef NS_ENUM(uint8_t, LynxBgShapeLayerProp) {
  LynxBgShapeLayerPropUndefine,
  LynxBgShapeLayerPropEnabled,
  LynxBgShapeLayerPropDisabled
};

@class LynxUI;
@class LynxBoxShadow;
@class LynxBackgroundImageLayerInfo;
@class LynxBackgroundInfo;

@interface LynxBackgroundSubLayer : CALayer
@property(nonatomic, assign) LynxBgTypes type;
@property(nonatomic, nullable) NSMutableArray<LynxBackgroundImageLayerInfo*>* imageArray;
@end

@interface LynxBorderLayer : CAShapeLayer
@property(nonatomic, assign) LynxBgTypes type;
@end

@interface LynxBackgroundSubBackgroundLayer : LynxBackgroundSubLayer
@property(nonatomic) BOOL isAnimated;
@property(nonatomic) NSUInteger frameCount;
@property(nonatomic) NSTimeInterval animatedImageDuration;
@property(nonatomic) BOOL enableAsyncDisplay;
@property(nonatomic) LynxBackgroundClipType backgroundColorClip;
@property(nonatomic, assign) UIEdgeInsets paddingWidth;
@property(nonatomic, assign) CGRect shadowsBounds;

- (void)markDirtyWithSize:(CGSize)viewSize
                    radii:(LynxBorderRadii)cornerRadii
             borderInsets:(UIEdgeInsets)borderInsets
          backgroundColor:(UIColor*)backgroundColor
               drawToEdge:(BOOL)drawToEdge
                capInsets:(UIEdgeInsets)insets
           isGradientOnly:(BOOL)isGradientOnly
              isPixelated:(BOOL)isPixelated;

- (void)setAnimatedPropsWithImage:(UIImage*)image;

// remove all gradientLayers in the imageArray if exists.
- (void)detachAllGradientLayers;
@end

extern const LynxBorderRadii LynxBorderRadiiZero;

static inline bool _noneZeroBUV(LynxBorderUnitValue unit) { return unit.val > 1e-3; }
static inline bool LynxHasBorderRadii(LynxBorderRadii radii) {
  return _noneZeroBUV(radii.topLeftX) || _noneZeroBUV(radii.topLeftY) ||
         _noneZeroBUV(radii.topRightX) || _noneZeroBUV(radii.topRightY) ||
         _noneZeroBUV(radii.bottomLeftX) || _noneZeroBUV(radii.bottomLeftY) ||
         _noneZeroBUV(radii.bottomRightX) || _noneZeroBUV(radii.bottomRightY);
}
//----------------------------------------------------------
//  Layers:
//    maskLayer (at the top)
//    borderLayer (for border and outline)
//    ui.view.layer
//    backgroundLayer (at back, for background and shadows)
//  Transform and Animations:
//    All the layers should use the same transform matrix, opacity etc.
//    For animations, we should check if the attached layers exits.
//  Border:
//    Simple mode: none / solid border + same border-radius, set border and cornerRadius on view's
//    layer Complex mode: create self.borderLayer, use complex mode to generate a image as the layer
//    content Any outlines will be attached to self.borderLayer
//  Background:
//    Simple mode: pure color + same border-radius + no background-image, set background color on
//      the bottom layer (self.layer if not nil, else the view's layer) in order that the
//      background-color will not cover anything;
//    Complex mode: create self.layer, use complex mode to generate a image as the layer content,
//      all the images and gradients will be drawn on it.
//    Any shadows will be attached to self.layer.
//  Radius and Clip:
//    if border radius is set, all layers should clip radius
//    if radius values of all corners are same, just use layer cornerRadius,
//      else use complex mode to draw a image as layer content
//----------------------------------------------------------
@interface LynxBackgroundManager : NSObject <CALayerDelegate>
@property(nonatomic, weak) LynxUI* ui;
@property(nonatomic) LynxBackgroundInfo* backgroundInfo;

@property(nonatomic, assign) CGFloat opacity;
@property(nonatomic, assign) BOOL hidden;

// TODO(fangzhou):implement [backgroundManager getInfo] and remove these properties
@property(nonatomic, nullable) UIColor* backgroundColor;
@property(nonatomic, assign) LynxBorderRadii borderRadius;
@property(nonatomic, assign) LynxBorderRadii borderRadiusRaw;
@property(nonatomic, assign) UIEdgeInsets borderWidth;

@property(nonatomic, nullable) UIColor* borderTopColor;
@property(nonatomic, nullable) UIColor* borderBottomColor;
@property(nonatomic, nullable) UIColor* borderLeftColor;
@property(nonatomic, nullable) UIColor* borderRightColor;

@property(nonatomic, nullable) NSMutableArray* backgroundDrawable;
@property(nonatomic, nullable) NSMutableArray* backgroundOrigin;
@property(nonatomic, nullable) NSMutableArray* backgroundPosition;
@property(nonatomic, nullable) NSMutableArray* backgroundRepeat;
@property(nonatomic, nullable) NSMutableArray* backgroundClip;
@property(nonatomic, nullable) NSMutableArray* backgroundImageSize;
@property(nonatomic, nullable) LynxBackgroundCapInsets* backgroundCapInsets;
@property(nonatomic, nullable) NSMutableArray* maskDrawable;
@property(nonatomic, nullable) NSMutableArray* maskOrigin;
@property(nonatomic, nullable) NSMutableArray* maskPosition;
@property(nonatomic, nullable) NSMutableArray* maskRepeat;
@property(nonatomic, nullable) NSMutableArray* maskClip;
@property(nonatomic, nullable) NSMutableArray* maskSize;

@property(nonatomic, nullable) LynxLinearGradient* linearGradient;
@property(nonatomic, assign) BOOL implicitAnimation;
@property(nonatomic, assign) CATransform3D transform;
@property(nonatomic, assign) CGPoint transformOrigin;

@property(nonatomic, nullable, readonly) LynxBackgroundSubBackgroundLayer* backgroundLayer;
@property(nonatomic, nullable, readonly) LynxBorderLayer* borderLayer;
@property(nonatomic, nullable, readonly) CALayer* outlineLayer;
@property(nonatomic) CGPoint postTranslate;
@property(nonatomic, nullable, readonly) LynxBackgroundSubBackgroundLayer* maskLayer;
@property(nonatomic, strong) UIView* opacityView;

// You can check if there are any style changed before calling applyEffect
@property(nonatomic, strong) NSArray<LynxBoxShadow*>* shadowArray;
@property(nonatomic, readonly) int animationOptions;
@property(nonatomic, readonly) int animationLayerCount;
@property(nonatomic, assign) BOOL allowsEdgeAntialiasing;
@property(nonatomic, assign) BOOL overlapRendering;
@property(nonatomic, assign) LynxBgShapeLayerProp uiBackgroundShapeLayerEnabled;
@property(nonatomic, assign) BOOL shouldRasterizeShadow;
@property(nonatomic, assign) BOOL isPixelated;

- (instancetype)initWithUI:(LynxUI*)ui;
- (void)markBackgroundDirty;
- (void)markMaskDirty;

- (void)applyEffect;
- (void)updateShadow;
- (void)removeAllAnimations;
- (void)addAnimationToViewAndLayers:(CAAnimation*)anim forKey:(nullable NSString*)key;
- (void)addAnimation:(CAAnimation*)anim forKey:(nullable NSString*)key;
- (void)removeAnimationForKey:(NSString*)key;
- (void)setPostTranslate:(CGPoint)postTranslate;
- (CATransform3D)getTransformWithPostTranslate;
- (UIImage*)getBackgroundImageForContentsAnimation;
- (UIImage*)getBackgroundImageForContentsAnimationWithSize:(CGSize)size;
- (UIImage*)getBorderImageForContentsAnimationWithSize:(CGSize)size;
- (CGPathRef)getBorderPathForAnimationWithSize:(CGSize)size;
- (void)getBackgroundImageForContentsAnimationAsync:(void (^)(UIImage*))completionBlock
                                           withSize:(CGSize)size;
- (void)removeAssociateLayers;
- (void)setFilters:(nullable NSArray*)array;

#pragma mark duplicate utils functions
+ (CGPathRef)createBezierPathWithRoundedRect:(CGRect)bounds
                                 borderRadii:(LynxBorderRadii)borderRadii
                                  edgeInsets:(UIEdgeInsets)edgeInsets;
+ (CGPathRef)createBezierPathWithRoundedRect:(CGRect)bounds
                                 borderRadii:(LynxBorderRadii)borderRadii;

#pragma mark access info functions
// TODO(fangzhou):implement [backgroundManager getInfo] and remove these functions
- (BOOL)hasDifferentBorderRadius;
- (void)makeCssDefaultValueToFitW3c;
- (BOOL)hasDifferentBackgroundColor:(UIColor*)color;

- (BOOL)updateOutlineWidth:(CGFloat)outlineWidth;
- (BOOL)updateOutlineColor:(UIColor*)outlineColor;
- (BOOL)updateOutlineStyle:(LynxBorderStyle)outlineStyle;

- (void)updateBorderColor:(LynxBorderPosition)position value:(UIColor*)color;
- (BOOL)updateBorderStyle:(LynxBorderPosition)position value:(LynxBorderStyle)style;
@end

@interface LynxConverter (LynxBorderStyle)
+ (LynxBorderStyle)toLynxBorderStyle:(id)value;
@end

NS_ASSUME_NONNULL_END
