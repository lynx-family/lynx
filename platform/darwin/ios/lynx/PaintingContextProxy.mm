// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "PaintingContextProxy.h"

@implementation PaintingContextProxy {
  // not owned, LynxShell released after platform, ensure life cycle
  lynx::tasm::PaintingContextDarwin* painting_context_;
}

- (instancetype)initWithPaintingContext:(lynx::tasm::PaintingContextDarwin*)paintingContext {
  if (self = [super init]) {
    painting_context_ = paintingContext;
  }
  return self;
}

- (void)updateExtraData:(NSInteger)sign value:(id)value {
  // this method can be removed
}

- (void)updateLayout:(NSInteger)sign
          layoutLeft:(CGFloat)left
                 top:(CGFloat)top
               width:(CGFloat)width
              height:(CGFloat)height {
  painting_context_->UpdateLayout((int)sign, left, top, width, height, nullptr, nullptr, nullptr,
                                  nullptr, nullptr, 0);
}

- (void)finishLayoutOperation {
  painting_context_->LayoutDidFinish();
  painting_context_->Flush();
}

- (BOOL)isLayoutFinish {
  return painting_context_->IsLayoutFinish();
}

- (void)resetLayoutStatus {
  painting_context_->ResetLayoutStatus();
}

@end
