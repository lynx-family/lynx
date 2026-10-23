// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_IOS_LYNX_EVENT_LYNX_EVENT_EMITTER_INTERNAL_H_
#define DARWIN_IOS_LYNX_EVENT_LYNX_EVENT_EMITTER_INTERNAL_H_

#import <Lynx/LynxEventEmitter.h>

#include <vector>

#include "core/public/event/pseudo_status.h"

@interface LynxEventEmitter ()

- (void)onPseudoStatusesChanged:(std::vector<lynx::event::PseudoStatusChange>)changes;

@end

#endif  // DARWIN_IOS_LYNX_EVENT_LYNX_EVENT_EMITTER_INTERNAL_H_
