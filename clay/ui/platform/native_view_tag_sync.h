// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PLATFORM_NATIVE_VIEW_TAG_SYNC_H_
#define CLAY_UI_PLATFORM_NATIVE_VIEW_TAG_SYNC_H_

#include <string>
#include <unordered_set>

namespace clay {

void SyncNativeViewTagsForContext(void* view_context,
                                  std::unordered_set<std::string> tags);

}  // namespace clay

#endif  // CLAY_UI_PLATFORM_NATIVE_VIEW_TAG_SYNC_H_
