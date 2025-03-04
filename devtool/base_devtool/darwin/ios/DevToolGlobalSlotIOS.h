// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_DARWIN_IOS_DEVTOOLGLOBALSLOTIOS_H_
#define DEVTOOL_BASE_DEVTOOL_DARWIN_IOS_DEVTOOLGLOBALSLOTIOS_H_

#import <Foundation/Foundation.h>
#include <memory>

#include "devtool/base_devtool/native/devtool_global_slot.h"

@interface DevToolGlobalSlotIOS : NSObject
- (instancetype _Nullable)initWithSlotPtr:
    (const std::shared_ptr<lynx::devtool::DevToolGlobalSlot> &)ptr;
- (void)sendMessage:(nonnull NSString *)message withType:(nonnull NSString *)type;
@end

#endif  // DEVTOOL_BASE_DEVTOOL_DARWIN_IOS_DEVTOOLGLOBALSLOTIOS_H_
