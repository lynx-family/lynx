// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_PARSER_HANDLER_DEFINES_H_
#define CORE_RENDERER_CSS_PARSER_HANDLER_DEFINES_H_

#include <array>

#include "core/renderer/css/css_property.h"
#include "core/renderer/css/parser/css_parser_configs.h"
#include "core/runtime/vm/lepus/lepus_value.h"

#define HANDLER_REGISTER_DECLARE() \
  void Register(std::array<pHandlerFunc, kCSSPropertyCount> &array)

#define HANDLER_REGISTER_IMPL() \
  void Register(std::array<pHandlerFunc, kCSSPropertyCount> &array)

#define HANDLER_REGISTER_IMPL_INL() \
  inline void Register(std::array<pHandlerFunc, kCSSPropertyCount> &array)

#define HANDLER_IMPL()                                                        \
  bool Handle(CSSPropertyID key, const lepus::Value &input, StyleMap &output, \
              const CSSParserConfigs &configs)

#define HANDLER_DECLARE()                                                     \
  bool Handle(CSSPropertyID key, const lepus::Value &input, StyleMap &output, \
              const CSSParserConfigs &configs)

namespace lynx {
namespace tasm {

using pHandlerFunc = bool (*)(CSSPropertyID key, const lepus::Value &input,
                              StyleMap &output,
                              const CSSParserConfigs &configs);
}  // namespace tasm

}  // namespace lynx

#endif  // CORE_RENDERER_CSS_PARSER_HANDLER_DEFINES_H_
