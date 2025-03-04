// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/*
  This dataSource handles different cellProducer, including internal LynxUIListCellContentProducer
  and native cellProducer from outside. It maps 'index -> cell' or 'item-key -> cell' translation.
*/

#import <Lynx/LynxListLayoutManager.h>
#import <Lynx/LynxListViewLight.h>

@class LynxListViewCellLight;
@class LynxUIContext;

NS_ASSUME_NONNULL_BEGIN
@interface LynxUIListDataSource : NSObject
#pragma mark init
/*
    Set UISign and UIContext to internal LynxUIListCellContentProducer
*/
- (void)setLynxSign:(NSInteger)sign;
- (void)setLynxUIContext:(LynxUIContext *)context;

#pragma mark core load methods
/**
 Load cell at index.
 @param index - given new cell position. Should be translated to itemKey to do the mix layout.
 @param listView - use listView to fetch a dequeued reusable cell.
 @return a ready-to-show cell.
 */
- (id<LynxListCell> _Nullable)listView:(LynxListViewLight *)listView
                    cellForItemAtIndex:(NSInteger)index;

/**
 recycle cell if it scrolls out of the preload area and visible area, or deleted.
 */
- (void)listView:(LynxListViewLight *)view recycleCell:(id<LynxListCell>)cell;

/**
 Handle updateTo & updateFrom from diffResult. Pass directly to cellContentProducer
 @param listView - Used to identify async/sync mode.
 @param cell - Cell to be updated.
 @param index - New cell position.
 @return A valid cell after updating its content with new position info.
 */
- (id<LynxListCell>)listView:(LynxListViewLight *)listView
                  updateCell:(id<LynxListCell>)cell
               toItemAtIndex:(NSInteger)index;
@end

NS_ASSUME_NONNULL_END
