// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface UIScrollView (ScrollCoordinator)

- (void)scrollCoordinator_addObserverBlockForKeyPath:(NSString *)keyPath
                                               block:(void (^)(__weak id obj, id oldVal,
                                                               id newVal))block;
- (void)scrollCoordinator_removeObserverBlocksForKeyPath:(NSString *)keyPath;

@end

NS_ASSUME_NONNULL_END
