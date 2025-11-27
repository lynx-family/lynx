// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterThreadSynchronizer.h"

#import <QuartzCore/QuartzCore.h>

#include <mutex>
#include <vector>

#import "base/include/fml/synchronization/waitable_event.h"
#import "clay/fml/logging.h"

@interface FlutterThreadSynchronizer () {
  std::mutex _mutex;
  BOOL _shuttingDown;
  CGSize _contentSize;
  std::vector<dispatch_block_t> _scheduledBlocks;

  BOOL _beginResizeWaiting;

  // Used to block [beginResize:].
  std::condition_variable _condBlockBeginResize;

  BOOL _visible;
}

@end

@implementation FlutterThreadSynchronizer

- (instancetype)init {
  self = [super init];
  if (self) {
    _visible = YES;
  }
  return self;
}

- (void)drain {
  FML_DCHECK([NSThread isMainThread]);

  [CATransaction begin];
  [CATransaction setDisableActions:YES];
  for (dispatch_block_t block : _scheduledBlocks) {
    block();
  }
  [CATransaction commit];
  _scheduledBlocks.clear();
}

- (void)setVisible:(BOOL)visible {
  _visible = visible;
}

- (void)blockUntilFrameAvailable {
  std::unique_lock<std::mutex> lock(_mutex);

  _beginResizeWaiting = YES;

  while (CGSizeEqualToSize(_contentSize, CGSizeZero) && !_shuttingDown) {
    _condBlockBeginResize.wait(lock);
    [self drain];
  }

  _beginResizeWaiting = NO;
}

- (void)beginResize:(CGSize)size notify:(nonnull dispatch_block_t)notify {
  std::unique_lock<std::mutex> lock(_mutex);

  if (CGSizeEqualToSize(_contentSize, CGSizeZero) || _shuttingDown) {
    // No blocking until framework produces at least one frame
    notify();
    return;
  }
  if (!_visible) {
    // Synchronizer is not visible, no need to block.
    notify();
    return;
  }

  [self drain];

  notify();

  _contentSize = CGSizeMake(-1, -1);

  _beginResizeWaiting = YES;

  while (!CGSizeEqualToSize(_contentSize, size) &&  //
         !CGSizeEqualToSize(_contentSize, CGSizeZero) && !_shuttingDown) {
    _condBlockBeginResize.wait(lock);
    [self drain];
  }

  _beginResizeWaiting = NO;
}

- (void)performCommit:(CGSize)size notify:(nonnull dispatch_block_t)notify {
  fml::AutoResetWaitableEvent event;
  {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_shuttingDown) {
      // Engine is shutting down, main thread may be blocked by the engine
      // waiting for raster thread to finish.
      return;
    }
    fml::AutoResetWaitableEvent& e = event;
    _scheduledBlocks.push_back(^{
      notify();
      _contentSize = size;
      e.Signal();
    });
    if (_beginResizeWaiting) {
      _condBlockBeginResize.notify_all();
    } else {
      dispatch_async(dispatch_get_main_queue(), ^{
        std::unique_lock<std::mutex> lock(_mutex);
        [self drain];
      });
    }
  }
  event.Wait();
}

- (void)shutdown {
  std::unique_lock<std::mutex> lock(_mutex);
  _shuttingDown = YES;
  _condBlockBeginResize.notify_all();
  [self drain];
}

@end
