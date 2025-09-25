// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/ListNodeInfoFetcher.h>
#import <Lynx/LynxTemplateRender.h>
#import "LynxContext+Internal.h"
#include "core/public/list_element_proxy.h"

@interface ListNodeInfoFetcher ()

@property(nonatomic, weak) LynxContext* context;

@end

@implementation ListNodeInfoFetcher

- (instancetype)initWithContext:(LynxContext*)context {
  self = [super init];
  if (self) {
    _context = context;
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
  if (_context && _context->list_element_proxy_) {
    _context->list_element_proxy_->ScrollByListContainer(containerSign, x, y, originalX, originalY);
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
  if (_context && _context->list_element_proxy_) {
    _context->list_element_proxy_->ScrollToPosition(containerSign, position, offset, align, smooth);
  }
}

/**
 notify the  stopped status to C++
 */
- (void)scrollStopped:(int)containerSign {
  if (_context && _context->list_element_proxy_) {
    _context->list_element_proxy_->ScrollStopped(containerSign);
  }
}

@end
