// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/platform/text_emoji_provider.h"

namespace clay {

std::optional<InlineEmojiBitmap> ResolveInlineEmojiBitmap(
    uintptr_t view_context, const std::u16string& text, float font_size) {
  (void)view_context;
  (void)text;
  (void)font_size;
  return std::nullopt;
}

}  // namespace clay
