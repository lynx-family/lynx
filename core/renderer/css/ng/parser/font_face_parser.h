// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_PARSER_FONT_FACE_PARSER_H_
#define CORE_RENDERER_CSS_NG_PARSER_FONT_FACE_PARSER_H_

#include <string>
#include <unordered_map>

#include "base/include/fml/memory/ref_counted.h"
#include "core/renderer/css/ng/font_face/font_face_rule.h"

namespace lynx {
namespace css {

class FontFaceParser {
 public:
  static fml::RefPtr<const FontFaceRule> Parse(
      const std::unordered_map<std::string, std::string>& descriptors);
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_PARSER_FONT_FACE_PARSER_H_
