// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxListReusePool.h"

@implementation LynxListReusePool

- (instancetype)init {
  self = [super init];
  if (self) {
    self.pool = [NSMutableDictionary dictionary];
    self.reuseIdentifierMap = [NSMutableDictionary dictionary];
  }
  return self;
}

- (void)registerClass:(Class)cellClass forCellReuseIdentifier:(nonnull NSString *)identifier {
  [self.reuseIdentifierMap setObject:cellClass forKey:identifier];
}

- (void)enqueueReusableCell:(id<LynxListCell>)cell {
  cell.isInStickyStatus = NO;
  if ([self.pool objectForKey:cell.reuseIdentifier]) {
    NSMutableArray *identifiedCells = [self.pool objectForKey:cell.reuseIdentifier];
    [identifiedCells addObject:cell];
  } else {
    NSMutableArray *identifiedCells = [NSMutableArray array];
    [identifiedCells addObject:cell];
    [self.pool setObject:identifiedCells forKey:cell.reuseIdentifier];
  }

  [cell.contentView.subviews makeObjectsPerformSelector:@selector(removeFromSuperview)];
  [(UIView *)cell removeFromSuperview];
}

- (id<LynxListCell>)dequeueReusableCellInIndex:(NSInteger)index
                           withReuseIdentifier:(NSString *)reuseIdentifier {
  Class cellClass = [self.reuseIdentifierMap objectForKey:reuseIdentifier];
  if (![cellClass conformsToProtocol:@protocol(LynxListCell)] || nil == cellClass) {
    return nil;
  }
  LynxListViewCellLight *cell;
  if (self.pool && self.pool.count > 0) {
    NSMutableArray *identifiedCells = [self.pool objectForKey:reuseIdentifier];
    if (identifiedCells) {
      cell = identifiedCells.lastObject;
      [identifiedCells removeLastObject];
    }
  }
  if (!cell) {
    cell = [[cellClass alloc] init];
  }
  return cell;
}
@end
