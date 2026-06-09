// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxOffsetCalculator : NSObject

/**
 * Get a point on the CGPath based on the given progress
 * @param path The CGPath to calculate on
 * @param progress Progress value (0.0 ~ 1.0)
 * @return The point on the path at the specified progress
 */

+ (CGPoint)pointAtProgress:(CGFloat)progress
                    onPath:(CGPathRef)path
               withTangent:(nullable CGFloat *)tangent;

+ (CGPoint)pointAtDistance:(CGFloat)distance
                    onPath:(CGPathRef)path
               withTangent:(nullable CGFloat *)tangent;

+ (CGFloat)pathLength:(CGPathRef)path;

@end

@interface PathLengthCache : NSObject
@property(nonatomic, assign) CGFloat totalLength;
@property(nonatomic, strong) NSMutableArray<NSNumber *> *segmentLengths;
@end

NS_ASSUME_NONNULL_END
