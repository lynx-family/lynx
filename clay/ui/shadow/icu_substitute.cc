// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/shadow/icu_substitute.h"

namespace clay {

namespace icu_substitute {

bool IsCJKCharacter(uint32_t unicode) {
  return (unicode >= 0x4e00 && unicode <= 0x9FFF) ||
         (unicode >= 0x3400 && unicode <= 0x4dbf) ||
         (unicode >= 0x20000 && unicode <= 0x2ffff);
}

bool IsRTLCharacter(const std::u16string& text) { return false; }

}  // namespace icu_substitute

}  // namespace clay
