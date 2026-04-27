// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PLATFORM_TEXT_EMOJI_PROVIDER_H_
#define CLAY_UI_PLATFORM_TEXT_EMOJI_PROVIDER_H_

#include <cstdint>
#include <optional>
#include <string>

#include "clay/ui/component/text/inline_emoji_bitmap.h"

namespace clay {

std::optional<InlineEmojiBitmap> ResolveInlineEmojiBitmap(
    uintptr_t view_context, const std::u16string& text, float font_size);

}  // namespace clay

#endif  // CLAY_UI_PLATFORM_TEXT_EMOJI_PROVIDER_H_
