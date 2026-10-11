// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNX_RESOURCE_HANDLE_INTERNAL_H_
#define DARWIN_COMMON_LYNX_LYNX_RESOURCE_HANDLE_INTERNAL_H_

#import <Lynx/LynxResourceHandle.h>

#include "core/resource/lynx_resource_handle.h"

/// @hide
@interface LynxResourceHandle (Internal)

- (std::shared_ptr<lynx::pub::LynxResourceHandle>)rawResourceHandle;

@end
/// @endhide

#endif  // DARWIN_COMMON_LYNX_LYNX_RESOURCE_HANDLE_INTERNAL_H_
