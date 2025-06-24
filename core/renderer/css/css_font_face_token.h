// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_CSS_FONT_FACE_TOKEN_H_
#define CORE_RENDERER_CSS_CSS_FONT_FACE_TOKEN_H_

#include <cstdint>
#include <string>

#include "base/include/value/lepus_value.h"
#include "core/public/layout_ctx_platform_impl.h"

static constexpr uint8_t CSS_BINARY_FONT_FACE_TYPE = 0x01;

namespace lynx {
namespace tasm {

CSSFontFaceRule* MakeCSSFontFaceToken(lepus::Value value);
void CSSFontTokenAddAttribute(CSSFontFaceRule* token, const std::string& name,
                              const std::string& val);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_FONT_FACE_TOKEN_H_
