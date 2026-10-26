// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxTextLayer.h>

@implementation LynxTextLayer {
  LynxTextRenderer *_textRenderer;
}

- (instancetype)initWithLynxTextRenderer:(LynxTextRenderer *)textRenderer {
  self = [super init];
  if (self) {
    _textRenderer = textRenderer;
    self.delegate = self;
    // TODO(xiamengfei.moonface): [ResizableWindowScale] Check whether this should use window or
    // physical screen metrics.
    self.contentsScale = [UIScreen mainScreen].scale;
  }
  return self;
}

// CSS animations belong to the renderer host. Reusing a backing layer must not
// animate its frame or cross-fade contents when a redraw is required.
- (id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event {
  return (id<CAAction>)[NSNull null];
}

- (void)drawLayer:(CALayer *)layer inContext:(CGContextRef)ctx {
  CGRect frame = CGRectMake(0, 0, layer.frame.size.width, layer.frame.size.height);

  UIGraphicsPushContext(ctx);
  CGContextSetShouldAntialias(ctx, YES);
  // TODO(songshourui.null): pass the padding and border to the text renderer later.
  [_textRenderer drawRect:frame padding:UIEdgeInsetsZero border:UIEdgeInsetsZero];
  UIGraphicsPopContext();
}

@end
