// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/ListNodeInfoFetcher.h>
#import <Lynx/LynxTemplateRender.h>
#include "core/shell/lynx_shell.h"

@interface ListNodeInfoFetcher ()

@property(nonatomic, assign, readwrite) int64_t shellPtr;

@end

@implementation ListNodeInfoFetcher

- (instancetype)initWithShell:(int64_t)shellPtr {
  self = [super init];
  if (self) {
    self.shellPtr = shellPtr;
  }
  return self;
}

/**
 *  notify the scrolled distance to C++
 */
- (void)scrollByListContainer:(int)containerSign
                            x:(float)x
                            y:(float)y
                    originalX:(float)originalX
                    originalY:(float)originalY {
  if (_shellPtr) {
    reinterpret_cast<lynx::shell::LynxShell *>(_shellPtr)->ScrollByListContainer(
        containerSign, x, y, originalX, originalY);
  }
}

/**
 *  notify the target scroll position to C++
 *
 */
- (void)scrollToPosition:(int)containerSign
                position:(int)position
                  offset:(float)offset
                   align:(int)align
                  smooth:(BOOL)smooth {
  if (_shellPtr) {
    reinterpret_cast<lynx::shell::LynxShell *>(_shellPtr)->ScrollToPosition(containerSign, position,
                                                                            offset, align, smooth);
  }
}

/**
 notify the  stopped status to C++
 */
- (void)scrollStopped:(int)containerSign {
  if (_shellPtr) {
    reinterpret_cast<lynx::shell::LynxShell *>(_shellPtr)->ScrollStopped(containerSign);
  }
}

@end
