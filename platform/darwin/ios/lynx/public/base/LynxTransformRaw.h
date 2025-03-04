// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxUIUnitUtils.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxTransformRaw : NSObject

- (instancetype)initWithArray:(NSArray*)arr;

@property(nonatomic, assign) float p0;
@property(nonatomic, assign) float p1;
@property(nonatomic, assign) float p2;

@property(strong, nonatomic) LynxPlatformLength* platformLengthP0;
@property(strong, nonatomic) LynxPlatformLength* platformLengthP1;
@property(strong, nonatomic) LynxPlatformLength* platformLengthP2;
@property(nonatomic, assign) int type;
@property(nonatomic, assign) CATransform3D transformMatrix;

- (bool)isP0Percent;
- (bool)isP1Percent;
- (bool)isP2Percent;
- (bool)isRotate;
- (bool)isRotateXY;
- (bool)isMatrix;
- (void)initTransformMatrixWithArray:(NSArray*)array;

+ (NSArray<LynxTransformRaw*>*)toTransformRaw:(NSArray*)arr;
+ (bool)hasPercent:(NSArray<LynxTransformRaw*>*)arr;
+ (bool)hasScaleOrRotate:(NSArray<LynxTransformRaw*>*)arr;
+ (CGFloat)getRotateZRad:(NSArray<LynxTransformRaw*>*)arr;
+ (CGFloat)getTranslateX:(NSArray<LynxTransformRaw*>*)arr;
+ (CGFloat)getTranslateY:(NSArray<LynxTransformRaw*>*)arr;
@end

NS_ASSUME_NONNULL_END
