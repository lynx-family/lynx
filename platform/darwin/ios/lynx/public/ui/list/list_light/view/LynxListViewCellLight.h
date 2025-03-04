// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxListLayoutModelLight.h>
#import <Lynx/LynxUIListProtocol.h>

@class LynxUIComponent;
@class LynxUI;
NS_ASSUME_NONNULL_BEGIN

/*
 Base class for every cell in UIListLightView
*/
@interface LynxListViewCellLight : UIView <LynxListCell>
/*
 We need an extra layer to wrap the content better so we can do animations and apply complex
 background and borders.
 */
@property(nonatomic, strong) UIView *contentView;
@property(nonatomic, strong) NSString *reuseIdentifier;
@property(nonatomic, assign) NSInteger updateToPath;  // Position of this cell.
@property(nonatomic, strong) NSString *itemKey;       // UniqueID for diff
@property(nonatomic, assign) BOOL
    removed;  // Mark removed before it really gets recycled. This is used to adjust the anchor.
@property(nonatomic, assign) NSInteger columnIndex;  // Mark which column it currently locates.
@property(nonatomic, assign) LynxLayoutModelType layoutType;  // Mark if its a fullspan item.
@property(nonatomic, assign) BOOL isInStickyStatus;
@property(nonatomic, assign) CGFloat stickyPosition;
@property(nonatomic, assign) int64_t operationID;

/**
 Use layoutModel to update cell's layout.
 @param model - layoutModel for the cell at its position.
 */
- (void)applyLayoutModel:(LynxListLayoutModelLight *)model;

@end

/*
 Used as lynxUI's container. Will have layers, such as background layers append on contentView.
 */
@interface LynxListViewCellLightLynxUI : LynxListViewCellLight

@property(nonatomic, nullable) LynxUIComponent *ui;

/**
 Use infomation from LynxUI to update its layout and flags.
 @param ui - LynxUI.
 */
- (void)addLynxUI:(LynxUI *)ui;

/**
 Remove LynxUI and clear its content. Used before recycle and enqueue.
 @return LynxUI - The removed LynxUI.
 */
- (LynxUI *)removeLynxUI;

@end

NS_ASSUME_NONNULL_END
