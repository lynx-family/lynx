// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxListViewCellLight.h"

NS_ASSUME_NONNULL_BEGIN
@interface LynxListReusePool : NSObject
@property(nonatomic, strong)
    NSMutableDictionary<NSString *, NSMutableArray<id<LynxListCell>> *> *pool;
@property(nonatomic, strong) NSMutableDictionary<NSString *, Class> *reuseIdentifierMap;
- (void)enqueueReusableCell:(id<LynxListCell>)cell;
- (id<LynxListCell>)dequeueReusableCellInIndex:(NSInteger)index
                           withReuseIdentifier:(NSString *)reuseIdentifier;

- (void)registerClass:(Class)cellClass forCellReuseIdentifier:(nonnull NSString *)identifier;
@end
NS_ASSUME_NONNULL_END
