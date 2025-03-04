// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxCollectionViewCell;
@class LynxUIListLoader;
@class LynxUICollection;

@interface LynxCollectionDataSource : NSObject <UICollectionViewDataSource>
- (instancetype)initWithLynxUICollection:(LynxUICollection*)collection;
- (void)apply;
- (BOOL)applyFirstTime;
@property(nonatomic) BOOL ignoreLoadCell;
@end

NS_ASSUME_NONNULL_END
