// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_TEXT_SELECTION_UTILS_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_TEXT_SELECTION_UTILS_H_

#include <string>

namespace lynx {
namespace tasm {
namespace harmony {

std::string GetTextSelectionLocalizedString(const char* resource_name,
                                            const char* fallback);

bool CopyTextSelectionToPasteboard(const std::string& selected_text);

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_TEXT_SELECTION_UTILS_H_
