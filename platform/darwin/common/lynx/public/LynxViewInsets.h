// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef LYNX_PUBLIC_LYNXVIEWINSETS_H_
#define LYNX_PUBLIC_LYNXVIEWINSETS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NSString* LynxInsetType NS_TYPED_ENUM;
FOUNDATION_EXPORT LynxInsetType const LynxInsetStatusBars;
FOUNDATION_EXPORT LynxInsetType const LynxInsetNavigationBars;
FOUNDATION_EXPORT LynxInsetType const LynxInsetCaptionBar;
FOUNDATION_EXPORT LynxInsetType const LynxInsetSystemBars;
FOUNDATION_EXPORT LynxInsetType const LynxInsetSystemOverlays;
FOUNDATION_EXPORT LynxInsetType const LynxInsetIme;
FOUNDATION_EXPORT LynxInsetType const LynxInsetDisplayCutout;
FOUNDATION_EXPORT LynxInsetType const LynxInsetWaterfall;
FOUNDATION_EXPORT LynxInsetType const LynxInsetRoundedCorners;
FOUNDATION_EXPORT LynxInsetType const LynxInsetSystemGestures;
FOUNDATION_EXPORT LynxInsetType const LynxInsetMandatorySystemGestures;
FOUNDATION_EXPORT LynxInsetType const LynxInsetTappableElement;

/** Geometry in screen coordinates, in points. */
@interface LynxInsetGeometry : NSObject
- (instancetype)init NS_UNAVAILABLE;
@end

@interface LynxInsetEdges : LynxInsetGeometry
@property(nonatomic, readonly) CGFloat left;
@property(nonatomic, readonly) CGFloat top;
@property(nonatomic, readonly) CGFloat right;
@property(nonatomic, readonly) CGFloat bottom;
- (instancetype)initWithLeft:(CGFloat)left
                         top:(CGFloat)top
                       right:(CGFloat)right
                      bottom:(CGFloat)bottom;
@end

@interface LynxInsetRect : NSObject
@property(nonatomic, readonly) CGFloat x;
@property(nonatomic, readonly) CGFloat y;
@property(nonatomic, readonly) CGFloat width;
@property(nonatomic, readonly) CGFloat height;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithX:(CGFloat)x y:(CGFloat)y width:(CGFloat)width height:(CGFloat)height;
@end

@interface LynxInsetRects : LynxInsetGeometry
@property(nonatomic, readonly, copy) NSArray<LynxInsetRect*>* rects;
- (instancetype)initWithRects:(NSArray<LynxInsetRect*>*)rects;
@end

@interface LynxInsetCorner : NSObject
@property(nonatomic, readonly) CGFloat centerX;
@property(nonatomic, readonly) CGFloat centerY;
@property(nonatomic, readonly) CGFloat radius;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithCenterX:(CGFloat)x centerY:(CGFloat)y radius:(CGFloat)radius;
@end

@interface LynxInsetCorners : LynxInsetGeometry
@property(nonatomic, readonly, nullable) LynxInsetCorner* topLeft;
@property(nonatomic, readonly, nullable) LynxInsetCorner* topRight;
@property(nonatomic, readonly, nullable) LynxInsetCorner* bottomRight;
@property(nonatomic, readonly, nullable) LynxInsetCorner* bottomLeft;
- (instancetype)initWithTopLeft:(nullable LynxInsetCorner*)topLeft
                       topRight:(nullable LynxInsetCorner*)topRight
                    bottomRight:(nullable LynxInsetCorner*)bottomRight
                     bottomLeft:(nullable LynxInsetCorner*)bottomLeft;
@end

@interface LynxInsetState : NSObject
@property(nonatomic, readonly) LynxInsetGeometry* geometry;
@property(nonatomic, readonly, nullable, copy) NSNumber* visible;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithGeometry:(LynxInsetGeometry*)geometry visible:(nullable NSNumber*)visible;
@end

/** Immutable category update. Missing categories retain their previous state. */
@interface LynxViewInsets : NSObject
@property(nonatomic, readonly, copy) NSDictionary<LynxInsetType, LynxInsetState*>* categories;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithCategories:(NSDictionary<LynxInsetType, LynxInsetState*>*)categories;
+ (instancetype)empty;
@end

@interface LynxViewInsetsBuilder : NSObject
/** nil omits a category from this update. Submit explicit zero geometry to clear it. */
- (instancetype)setInset:(LynxInsetType)type state:(nullable LynxInsetState*)state;
- (LynxViewInsets*)build;
@end

NS_ASSUME_NONNULL_END
#endif  // LYNX_PUBLIC_LYNXVIEWINSETS_H_
