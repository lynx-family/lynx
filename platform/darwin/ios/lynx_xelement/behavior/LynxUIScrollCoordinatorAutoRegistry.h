// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <XElement/LynxUIScrollCoordinator.h>
#import <XElement/LynxUIScrollCoordinatorHeader.h>
#import <XElement/LynxUIScrollCoordinatorSlot.h>
#import <XElement/LynxUIScrollCoordinatorSlotDrag.h>
#import <XElement/LynxUIScrollCoordinatorToolbar.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxUIScrollCoordinatorAutoRegistry : LynxUIScrollCoordinator
@end

@interface LynxUIScrollCoordinatorHeaderAutoRegistry : LynxUIScrollCoordinatorHeader
@end

@interface LynxUIScrollCoordinatorSlotAutoRegistry : LynxUIScrollCoordinatorSlot
@end

@interface LynxUIScrollCoordinatorSlotDragAutoRegistry : LynxUIScrollCoordinatorSlotDrag
@end

@interface LynxUIScrollCoordinatorToolbarAutoRegistry : LynxUIScrollCoordinatorToolbar
@end

NS_ASSUME_NONNULL_END
