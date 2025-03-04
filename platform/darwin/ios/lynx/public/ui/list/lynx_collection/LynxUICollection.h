// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIListLoader.h"
#import "LynxUIListScrollEvent.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, LynxListLayoutOrientation) {
  LynxListLayoutOrientationNone,
  LynxListLayoutOrientationVertical,
  LynxListLayoutOrientationHorizontal,
};

// TODO(hujing.1): move other place from the public folder
@interface LynxUICollection : LynxUIListLoader <UICollectionView *> <LynxUIListScrollEvent>

@property(nonatomic, assign) BOOL noRecursiveLayout;
@property(nonatomic, assign) BOOL forceReloadData;
@property(nonatomic, strong) NSMutableDictionary *listNativeStateCache;
// <cacheKey, Set<initial- props>> Stores flushed initial- props for cacheKey
@property(nonatomic, strong)
    NSMutableDictionary<NSString *, NSMutableSet<NSString *> *> *initialFlushPropCache;
@property(nonatomic) LynxListLayoutOrientation layoutOrientation;
@property(nonatomic) BOOL enableRtl;

// the switch is used for async-list
@property(nonatomic) BOOL enableAsyncList;
// checks if list can render components on next step;
- (BOOL)isNeedRenderComponents;
- (BOOL)shouldGenerateDebugInfo;

@end
NS_ASSUME_NONNULL_END
