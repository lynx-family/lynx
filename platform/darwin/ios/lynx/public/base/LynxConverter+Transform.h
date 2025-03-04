// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "LynxConverter.h"
#import "LynxTransformOriginRaw.h"
#import "LynxTransformRaw.h"
#import "LynxUI.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, LynxTransformRotationType) {
  LynxTransformRotationNone = 0,
  LynxTransformRotationX = 1,
  LynxTransformRotationY = 1 << 1,
  LynxTransformRotationZ = 1 << 2
};

@interface LynxConverter (Transform)

+ (CATransform3D)toCATransform3D:(NSArray<LynxTransformRaw*>*)value
                    rotationType:(char*)rotationType
                       rotationX:(CGFloat*)currentRotationX
                       rotationY:(CGFloat*)currentRotationY
                       rotationZ:(CGFloat*)currentRotationZ
                              ui:(LynxUI*)ui;

+ (CATransform3D)toCATransform3D:(NSArray<LynxTransformRaw*>*)value
                              ui:(LynxUI*)ui
                        newFrame:(CGRect)frame
          transformWithoutRotate:(CATransform3D*)transformWithoutRotate
        transformWithoutRotateXY:(CATransform3D*)transformWithoutRotateXY
                    rotationType:(char*)rotationType
                       rotationX:(CGFloat*)rotationX
                       rotationY:(CGFloat*)rotationY
                       rotationZ:(CGFloat*)rotationZ;

+ (CATransform3D)toCATransform3D:(NSArray<LynxTransformRaw*>*)value
                              ui:(LynxUI*)ui
                        newFrame:(CGRect)frame
                    rotationType:(char*)rotationType
                       rotationX:(CGFloat*)currentRotationX
                       rotationY:(CGFloat*)currentRotationY
                       rotationZ:(CGFloat*)currentRotationZ;

+ (CATransform3D)toCATransform3D:(NSArray<LynxTransformRaw*>*)value ui:(LynxUI*)ui;

+ (BOOL)isDefaultTransformOrigin:(LynxTransformOriginRaw*)transformOrigin;
@end
NS_ASSUME_NONNULL_END
