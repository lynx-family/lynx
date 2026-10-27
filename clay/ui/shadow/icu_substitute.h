// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_ICU_SUBSTITUTE_H_
#define CLAY_UI_SHADOW_ICU_SUBSTITUTE_H_

#include <cstdint>
#include <string>

namespace clay {

namespace icu_substitute {

bool IsCJKCharacter(uint32_t unicode);
bool IsRTLCharacter(const std::u16string& text);

}  // namespace icu_substitute

}  // namespace clay

#endif  // CLAY_UI_SHADOW_ICU_SUBSTITUTE_H_
