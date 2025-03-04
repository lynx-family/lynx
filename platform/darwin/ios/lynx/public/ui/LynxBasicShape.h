// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxBasicShape : NSObject
- (UIBezierPath*)pathWithFrameSize:(CGSize)frameSize;
@end

LynxBasicShape* _Nullable LBSCreateBasicShapeFromArray(NSArray<NSNumber*>* array);
CGPathRef LBSCreatePathFromBasicShape(LynxBasicShape* shape, CGSize viewport);
LynxBasicShape* _Nullable LBSCreateBasicShapeFromPathData(NSString* data);

NS_ASSUME_NONNULL_END
