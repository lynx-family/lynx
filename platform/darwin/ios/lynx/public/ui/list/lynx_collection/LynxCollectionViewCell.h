// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxListDebug.h"

NS_ASSUME_NONNULL_BEGIN

@class LynxUI;
@class LynxUIComponent;
@interface LynxCollectionViewCell : UICollectionViewCell
@property(nonatomic, nullable) LynxUIComponent *ui;
@property(nonatomic, nullable) NSIndexPath *updateToPath;
// a cell is loading until it has been successfully updated/rendered.
@property(nonatomic) BOOL loading;
@property(nonatomic, strong) NSString *itemKey;
@property(nonatomic) int64_t operationID;
@property(nonatomic, assign) BOOL isPartOnLayout;
#if LYNX_LIST_DEBUG_LABEL
@property(nonatomic) UILabel *label;
#endif
- (LynxUI *)removeLynxUI;
- (void)addLynxUI:(LynxUI *)ui;
- (void)adjustComponentFrame;
- (void)restartAnimation;
@end

NS_ASSUME_NONNULL_END
