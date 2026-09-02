// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxCollectionViewLayoutModel.h>
#import <Lynx/LynxUIKitAPIAdapter.h>

@implementation LynxCollectionViewLayoutModel

- (instancetype)init {
  self = [super self];
  if (self) {
    _frame.size.height = [LynxCollectionViewLayoutModel defaultHeight];
    _frame.size.width = [LynxCollectionViewLayoutModel defaultWidth];
  }
  return self;
}

+ (instancetype)modelWithBounds:(CGRect)bound {
  LynxCollectionViewLayoutModel* model = [[LynxCollectionViewLayoutModel alloc] init];
  model.frame = bound;
  return model;
}

+ (instancetype)modelWithHeight:(CGFloat)height {
  return [LynxCollectionViewLayoutModel modelWithBounds:CGRectMake(0, 0, 0, height)];
}

+ (instancetype)modelWithWidth:(CGFloat)width {
  return [LynxCollectionViewLayoutModel modelWithBounds:CGRectMake(0, 0, width, 0)];
}

+ (instancetype)modelWithDefaultSize {
  return [[LynxCollectionViewLayoutModel alloc] init];
}

+ (CGFloat)defaultHeight {
  return [LynxUIKitAPIAdapter getKeyWindow] ? [LynxUIKitAPIAdapter getKeyWindow].bounds.size.height
                                            : [UIScreen mainScreen].bounds.size.height;
}

+ (CGFloat)defaultWidth {
  return [LynxUIKitAPIAdapter getKeyWindow] ? [LynxUIKitAPIAdapter getKeyWindow].bounds.size.width
                                            : [UIScreen mainScreen].bounds.size.width;
}

- (id)copyWithZone:(NSZone*)zone {
  LynxCollectionViewLayoutModel* copyModel = [[LynxCollectionViewLayoutModel alloc] init];
  copyModel.frame = _frame;
  return copyModel;
}

@end
