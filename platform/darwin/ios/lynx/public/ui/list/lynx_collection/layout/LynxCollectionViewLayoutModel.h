// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN
typedef NS_ENUM(NSUInteger, LynxCollectionViewLayoutType) {
  LynxCollectionViewLayoutNone,
  LynxCollectionViewLayoutWaterfall,
  LynxCollectionViewLayoutFlow
};

typedef NS_ENUM(NSUInteger, LynxCollectionViewLayoutDirection) {
  LynxCollectionViewLayoutVertical,
  LynxCollectionViewLayoutHorizontal
};

@interface LynxCollectionViewLayoutModel : NSObject <NSCopying>
@property(nonatomic) CGRect frame;

+ (instancetype)modelWithBounds:(CGRect)bound;
+ (instancetype)modelWithHeight:(CGFloat)height;
+ (instancetype)modelWithWidth:(CGFloat)width;
+ (instancetype)modelWithDefaultSize;
+ (CGFloat)defaultHeight;
+ (CGFloat)defaultWidth;
@end
NS_ASSUME_NONNULL_END
