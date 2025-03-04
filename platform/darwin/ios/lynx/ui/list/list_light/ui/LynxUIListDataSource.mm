// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxListViewCellLight.h>
#import <Lynx/LynxListViewLight.h>
#import <Lynx/LynxUIListCellContentProducer.h>
#import <Lynx/LynxUIListDataSource.h>
#import <Lynx/LynxUIListProtocol.h>

@interface LynxUIListDataSource ()
@property(nonatomic, strong) LynxUIListCellContentProducer *internalDataSourceLight;
@property(nonatomic, weak) LynxUIContext *UIContext;
@end

@implementation LynxUIListDataSource
#pragma mark init
- (instancetype)init {
  self = [super init];
  if (self) {
    self.internalDataSourceLight = [[LynxUIListCellContentProducer alloc] init];
  }
  return self;
}

- (void)setLynxSign:(NSInteger)sign {
  _internalDataSourceLight.sign = sign;
}

- (void)setLynxUIContext:(LynxUIContext *)context {
  [_internalDataSourceLight setUIContext:context];
  _UIContext = context;
}

#pragma mark core load & recycle methods
- (id<LynxListCell>)listView:(LynxListViewLight *)listView cellForItemAtIndex:(NSInteger)index {
  //(MR TODO)fangzhou.fz: support mixed cellProducer
  return [_internalDataSourceLight listView:listView cellForItemAtIndex:index];
}

- (void)listView:(LynxListViewLight *)view recycleCell:(id<LynxListCell>)cell {
  //(MR TODO)fangzhou.fz: support mixed cellProducer
  if ([cell isKindOfClass:LynxListViewCellLightLynxUI.class]) {
    [_internalDataSourceLight listView:view enqueueCell:(LynxListViewCellLightLynxUI *)cell];
  }
}

- (id<LynxListCell>)listView:(LynxListViewLight *)listView
                  updateCell:(id<LynxListCell>)cell
               toItemAtIndex:(NSInteger)index {
  // (MR TODO) now it's direct use of _internalDataSourceLight and need to fetch real DataSource
  // from DataSourceCache.
  return [_internalDataSourceLight listView:listView updateCell:cell toItemAtIndex:index];
}

@end
