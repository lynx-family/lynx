// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxDisplayListApplier+Internal.h>
#import <Lynx/LynxRenderer+Internal.h>
#import <Lynx/LynxRenderer.h>
#import <Lynx/LynxRendererHost.h>

@implementation LynxRenderer {
  __weak UIView<LynxRendererHost>* _host;
  LynxDisplayListApplier* _applier;

  lynx::tasm::DisplayList* list_;

  int32_t sign_;
}

- (instancetype)initWithRenderHost:(UIView<LynxRendererHost>*)host andSign:(int32_t)sign {
  self = [super init];
  if (self) {
    _host = host;
    sign_ = sign;
  }
  return self;
}

- (int32_t)getSign {
  return sign_;
}

- (void)ensureLynxDisplayListApplier {
  if (_applier != nil) {
    return;
  }
  // TODO(songshourui.null): add init function to LynxDisplayListApplier later.
  // _applier = [[LynxDisplayListApplier alloc] initWithView:_host];
  _applier = [[LynxDisplayListApplier alloc] init];
}

- (void)updateDisplayList:(lynx::tasm::DisplayList*)list {
  list_ = list;

  [self ensureLynxDisplayListApplier];

  // TODO(songshourui.null): add init function to LynxDisplayListApplier later.
  // [_applier applyDisplayList:list_];
}

- (lynx::tasm::DisplayList*)getDisplayList {
  return list_;
}

@end
