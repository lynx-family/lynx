// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_PARSER_FLOW_TOLERANCE_HANDLER_H_
#define CORE_RENDERER_CSS_PARSER_FLOW_TOLERANCE_HANDLER_H_

#include "core/renderer/css/parser/handler_defines.h"

namespace lynx {
namespace tasm {
namespace FlowToleranceHandler {

enum class Keyword : int32_t {
  kNormal,
  kInfinite,
};

HANDLER_REGISTER_DECLARE();

}  // namespace FlowToleranceHandler
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_PARSER_FLOW_TOLERANCE_HANDLER_H_
