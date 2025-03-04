// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import <Lynx/LynxCSSType.h>
#import <objc/NSObject.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxUIUnitUtils : NSObject

+ (CGFloat)screenScale;
+ (CGFloat)roundPtToPhysicalPixel:(CGFloat)number;
+ (void)roundRectToPhysicalPixelGrid:(CGRect*)rect;
+ (void)roundInsetsToPhysicalPixelGrid:(UIEdgeInsets*)insets;

@end

/**
 * @brief Class used for calculate the length value used by drawing the UI. 'calc' and
 * '<percentage>' value. The correct value should get via `getValueWithParentValue:(CGFloat)`.
 * `parentValue` is the reference length for percentage calculation.
 *
 */
@interface LynxPlatformLength : NSObject <NSCopying>
@property(nonatomic, assign) LynxPlatformLengthUnit type;
@property(nonatomic, assign) CGFloat value;
@property(nonatomic, strong, nullable) NSArray* calcArray;

- (instancetype)initWithValue:(id)value type:(LynxPlatformLengthUnit)type;
- (CGFloat)valueWithParentValue:(CGFloat)parentValue;
- (CGFloat)numberValue;

@end

NS_ASSUME_NONNULL_END
