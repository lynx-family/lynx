// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTextraLayer.h"
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceTextProtocol.h>
#import "LynxRendererContext.h"

@implementation LynxTextraLayer {
  int32_t _textID;
  __weak LynxRendererContext *_rendererContext;
}

- (instancetype)initWithTextID:(int32_t)textID
               rendererContext:(LynxRendererContext *)rendererContext {
  if (self = [super init]) {
    _textID = textID;
    _rendererContext = rendererContext;
    self.delegate = self;
    self.contentsScale = [UIScreen mainScreen].scale;
  }
  return self;
}

- (void)drawLayer:(CALayer *)layer inContext:(CGContextRef)ctx {
  void *page = [_rendererContext getTextBundle:_textID];
  if (page == nullptr) return;

  id<LynxServiceTextProtocol> textService = LynxService(LynxServiceTextProtocol);
  if (textService) {
    [textService drawPage:page OnContext:ctx];
  }
}

@end
