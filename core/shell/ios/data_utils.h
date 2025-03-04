// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_IOS_DATA_UTILS_H_
#define CORE_SHELL_IOS_DATA_UTILS_H_

#import <Foundation/Foundation.h>
#include <vector>

std::vector<uint8_t> ConvertNSBinary(NSData* binary);

#endif  // CORE_SHELL_IOS_DATA_UTILS_H_
