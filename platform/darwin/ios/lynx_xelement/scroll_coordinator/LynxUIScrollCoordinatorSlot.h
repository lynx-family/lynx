// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUI.h>
#import <XElement/LynxUIScrollCoordinatorSlotDrag.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxUIScrollCoordinatorSlot : LynxUI
@property(nonatomic, weak) LynxUIScrollCoordinatorSlotDrag *slotDrag;
@property(nonatomic, assign) BOOL slotHeightChanged;
@end

NS_ASSUME_NONNULL_END
