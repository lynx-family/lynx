// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_PATH_PARSER_H_
#define CORE_RENDERER_DOM_FRAGMENT_PATH_PARSER_H_

#include <string>

#include "core/renderer/dom/fragment/path.h"

namespace lynx {
namespace tasm {
// Convert SVG path data in CSS px to local-space display-list commands.
bool ParseSvgPath(const std::string& data, float scale, Path& out);
}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_DOM_FRAGMENT_PATH_PARSER_H_
