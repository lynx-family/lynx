// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_UNIT_HANDLER_H_
#define CORE_RENDERER_CSS_UNIT_HANDLER_H_

#include <array>
#include <cctype>
#include <cmath>
#include <string>

#include "base/include/base_export.h"
#include "base/include/debug/lynx_assert.h"
#include "core/renderer/css/css_debug_msg.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/parser/css_parser_configs.h"
#include "core/renderer/css/parser/handler_defines.h"

namespace lynx {
namespace tasm {

#define CSS_HANDLER_FAIL_IF(COND, STRICT, ...)                       \
  if (UNLIKELY(COND)) {                                              \
    if (UNLIKELY(STRICT)) {                                          \
      lynx::tasm::UnitHandler::CSSWarningUnconditional(__VA_ARGS__); \
    }                                                                \
    return false;                                                    \
  }

#define CSS_HANDLER_FAIL_IF_NOT(COND, STRICT, ...)                   \
  if (UNLIKELY(!(COND))) {                                           \
    if (UNLIKELY(STRICT)) {                                          \
      lynx::tasm::UnitHandler::CSSWarningUnconditional(__VA_ARGS__); \
    }                                                                \
    return false;                                                    \
  }

class UnitHandler {
 public:
  // only for NoDestructor.
  UnitHandler();

  static void ReportError(std::string error_msg, std::string fix_suggestion,
                          CSSPropertyID key, const std::string& input);

  // Log formatted warning message unconditionally and always returns false.
  static bool CSSWarningUnconditional(const char* fmt...);

  // If !expression returns false and log formatted message if in strict mode.
  static bool CSSWarning(bool expression, bool enableCSSStrictMode,
                         const char* fmt...);

  // Log custom unreachable message in strict mode and always returns false.
  static bool CSSUnreachable(bool enableCSSStrictMode, const char* fmt...);

  // Log unreachable message in strict mode and always returns false
  static bool CSSMethodUnreachable(bool enableCSSStrictMode);

  BASE_EXPORT_FOR_DEVTOOL static bool Process(const CSSPropertyID key,
                                              const lepus::Value& input,
                                              StyleMap& output,
                                              const CSSParserConfigs& configs);
  BASE_EXPORT static StyleMap Process(const CSSPropertyID key,
                                      const lepus::Value& input,
                                      const CSSParserConfigs& configs);

  BASE_EXPORT_FOR_DEVTOOL static bool ProcessCSSValue(
      const CSSPropertyID key, const tasm::CSSValue& input, StyleMap& output,
      const CSSParserConfigs& configs);

 private:
  static UnitHandler& Instance();

  std::array<pHandlerFunc, kCSSPropertyCount> interceptors_;
};
}  // namespace tasm

}  // namespace lynx

#endif  // CORE_RENDERER_CSS_UNIT_HANDLER_H_
