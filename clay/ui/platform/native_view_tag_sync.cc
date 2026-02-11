// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/platform/native_view_tag_sync.h"

#include <utility>

#include "clay/ui/component/view_context.h"

namespace clay {

void SyncNativeViewTagsForContext(void* view_context,
                                  std::unordered_set<std::string> tags) {
  if (view_context == nullptr) {
    return;
  }
  auto* context = reinterpret_cast<ViewContext*>(view_context);
  context->SyncNativeViewTags(std::move(tags));
}

}  // namespace clay
