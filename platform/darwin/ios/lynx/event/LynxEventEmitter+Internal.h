// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxEventEmitter.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, LynxElementPositionUpdateType) {
  LynxElementPositionUpdateScrollOffset = 0,
  LynxElementPositionUpdateStickyTranslation = 1,
};

@interface LynxEventEmitter (Internal)

- (void)beginElementPositionStateBatch;

- (void)updateElementPositionState:(NSInteger)tag
                              type:(LynxElementPositionUpdateType)type
                                 x:(CGFloat)x
                                 y:(CGFloat)y;

- (void)endElementPositionStateBatch;

- (void)updatePageCoordinateSnapshotWithWindowX:(CGFloat)windowX
                                        windowY:(CGFloat)windowY
                                hasWindowOffset:(BOOL)hasWindowOffset
                                        screenX:(CGFloat)screenX
                                        screenY:(CGFloat)screenY
                                hasScreenOffset:(BOOL)hasScreenOffset
                            forcePositionChange:(BOOL)forcePositionChange;

@end

NS_ASSUME_NONNULL_END
