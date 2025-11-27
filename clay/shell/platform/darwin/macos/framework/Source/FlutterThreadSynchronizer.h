// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Cocoa/Cocoa.h>

/**
 * Takes care of synchronization between raster and platform thread.
 */
@interface FlutterThreadSynchronizer : NSObject

/**
 * Set whether the synchronizer is visible.
 * If not visible, the synchronizer will not block.
 */
- (void)setVisible:(BOOL)visible;

/**
 * Blocks current thread until there is frame available.
 * Used in FlutterEngineTest.
 */
- (void)blockUntilFrameAvailable;

/**
 * Called from platform thread. Blocks until commit with given size (or empty)
 * is requested.
 */
- (void)beginResize:(CGSize)size notify:(nonnull dispatch_block_t)notify;

/**
 * Called from raster thread. Schedules the given block on platform thread
 * and blocks until it is performed.
 *
 * If platform thread is blocked in `beginResize:` for given size (or size is empty),
 * unblocks platform thread.
 *
 * The notify block is guaranteed to be called within a core animation transaction.
 */
- (void)performCommit:(CGSize)size notify:(nonnull dispatch_block_t)notify;

/**
 * Called when shutting down. Unblocks everything and prevents any further synchronization.
 */
- (void)shutdown;

@end
