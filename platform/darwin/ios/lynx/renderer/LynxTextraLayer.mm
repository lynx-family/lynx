// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTextraLayer.h"
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceTextProtocol.h>

@implementation LynxTextraLayer {
  void *_page;
}

- (instancetype)initWithPage:(void *)page {
  if (self = [super init]) {
    _page = page;
    self.delegate = self;
    self.contentsScale = [UIScreen mainScreen].scale;
  }
  return self;
}

- (void)dealloc {
  if (_page != nullptr) {
    id<LynxServiceTextProtocol> textService = LynxService(LynxServiceTextProtocol);
    if (textService) {
      [textService destroyPage:_page];
    }
    _page = nullptr;
  }
}

- (void)drawLayer:(CALayer *)layer inContext:(CGContextRef)ctx {
  if (!_page) return;

  id<LynxServiceTextProtocol> textService = LynxService(LynxServiceTextProtocol);
  if (textService) {
    [textService drawPage:_page OnContext:ctx];
  }
}

@end
