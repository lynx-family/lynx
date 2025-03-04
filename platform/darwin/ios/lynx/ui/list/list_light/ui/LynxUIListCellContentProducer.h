// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/LynxListViewCellLight.h>

@class LynxUIContext;
@class LynxListViewLight;

@interface LynxUIListCellContentProducer : NSObject
/*
    UISign for current list. Used in loading method 'componentAtIndex'.
*/
@property(nonatomic, assign) NSInteger sign;

/*
 Native storage
 */
@property(nonatomic) BOOL needsInternalCellAppearNotification;
@property(nonatomic) BOOL needsInternalCellDisappearNotification;
@property(nonatomic) BOOL needsInternalCellPrepareForReuseNotification;

/*
    Used to fetch listNode.
*/
- (void)setUIContext:(LynxUIContext *)UIContext;

/**
 Generate a cell in given index.
 @param listView - current ListView. Should use to call dequeueReusableCellForIndex: to get a
 reusable cell.
 @param index - cell position.
 */
- (LynxListViewCellLight *)listView:(LynxListViewLight *)listView
                 cellForItemAtIndex:(NSInteger)index;

/**
 Recycle cell and LynxUI it holds.
 @param listView - Used to identify async/sync mode.
 @param cell - cell to recycle.
 */
- (void)listView:(LynxListViewLight *)listView enqueueCell:(LynxListViewCellLightLynxUI *)cell;

/**
 Handle updateTo & updateFrom from diffResult.
 @param listView - Used to identify async/sync mode.
 @param cell - Cell to be updated.
 @param index - New cell position.
 @return A valid cell after updating its content with new position info.
 */
- (id<LynxListCell>)listView:(LynxListViewLight *)listView
                  updateCell:(id<LynxListCell>)cell
               toItemAtIndex:(NSInteger)index;
@end
