// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIComponent.h"
#import "LynxComponentRegistry.h"
#import "LynxPropsProcessor.h"
#import "LynxUI+Internal.h"

@implementation LynxUIComponent
#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_UI("component")
#else
LYNX_REGISTER_UI("component")
#endif

- (void)setFrame:(CGRect)frame {
  [super setFrame:frame];
  _frameDidSet = YES;
}

- (void)onNodeReady {
  [super onNodeReady];
  if (self.layoutObserver &&
      [self.layoutObserver respondsToSelector:@selector(onComponentLayoutUpdated:)]) {
    [self.layoutObserver onComponentLayoutUpdated:self];
  }
}

LYNX_PROP_SETTER("item-key", setItemKey, NSString*) {
  if (requestReset) {
    value = nil;
  }
  self.itemKey = value;
}

LYNX_PROP_SETTER("z-index", setZIndex, NSInteger) {
  if (requestReset) {
    value = 0;
  }
  _zIndex = value;
}

- (void)asyncListItemRenderFinished:(int64_t)operationID {
  if (self.layoutObserver && [self.layoutObserver respondsToSelector:@selector
                                                  (onAsyncComponentLayoutUpdated:operationID:)]) {
    [self.layoutObserver onAsyncComponentLayoutUpdated:self operationID:operationID];
  }
}
@end
