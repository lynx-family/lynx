// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "LynxComponent.h"
#import "LynxCustomMeasureDelegate.h"
#import "LynxLayoutStyle.h"
#import "LynxMeasureDelegate.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxLayoutNode<__covariant V> : LynxComponent <V>

@property(nonatomic, readonly) NSInteger sign;
@property(nonatomic, readonly) NSString* tagName;
@property(nonatomic, readonly, assign) CGRect frame;
@property(nonatomic, readonly, assign) UIEdgeInsets padding;
@property(nonatomic, readonly, assign) UIEdgeInsets margin;
@property(nonatomic, readonly, assign) UIEdgeInsets border;
@property(nonatomic, readonly, nullable) LynxLayoutStyle* style;
@property(nonatomic, weak) id<LynxMeasureDelegate> measureDelegate;
@property(nonatomic, weak) id<LynxCustomMeasureDelegate> customMeasureDelegate;
@property(nonatomic, assign) void* layoutNodeManagerPtr;

- (instancetype)initWithSign:(NSInteger)sign tagName:(NSString*)tagName;
- (void)adoptNativeLayoutNode:(int64_t)ptr;
- (void)updateLayoutWithFrame:(CGRect)frame;
- (MeasureResult)measureWithWidth:(float)width
                        widthMode:(LynxMeasureMode)widthMode
                           height:(float)height
                       heightMode:(LynxMeasureMode)heightMode
                     finalMeasure:(bool)finalMeasure;
- (void)align;

/**
 * Request layout and it will happens on next vsync.
 * Call this method when something relevant to the frame of view has change.
 */
- (void)setNeedsLayout;
/**
 * Temporary method, will be deleted in the future
 */
- (void)internalSetNeedsLayoutForce;
- (BOOL)needsLayout;

- (void)layoutDidStart;
- (void)layoutDidUpdate;

/**
 * When hasCustomLayout return true, this node will handle layouting
 * it's children, otherwise the layout position of children still
 * respect the result given by native layout system.
 *
 * @return If node can handle child layout, return true otherwise false.
 */
- (BOOL)hasCustomLayout;
@end

NS_ASSUME_NONNULL_END
