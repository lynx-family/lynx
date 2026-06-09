// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_SUPPORTS_SUPPORTS_EVALUATOR_H_
#define CORE_RENDERER_CSS_NG_SUPPORTS_SUPPORTS_EVALUATOR_H_

#include <cstdint>

#include "core/renderer/css/ng/supports/supports_condition.h"
#include "core/renderer/css/parser/css_parser_configs.h"

namespace lynx {
namespace css {

inline uint32_t PackVersion(const base::Version& version) {
  return SupportsEngineVersionNode::PackVersion(
      static_cast<uint16_t>(version.Major()),
      static_cast<uint16_t>(version.Minor()));
}

class SupportsEvaluator {
 public:
  explicit SupportsEvaluator(const tasm::CSSParserConfigs& configs)
      : engine_version_(PackVersion(LYNX_VERSION)), configs_(configs) {}
  SupportsEvaluator(uint32_t engine_version,
                    const tasm::CSSParserConfigs& configs)
      : engine_version_(engine_version), configs_(configs) {}

  bool Eval(const SupportsConditionNode& node) const;
  bool Eval(const SupportsConditionNode* node) const {
    return node && Eval(*node);
  }

 private:
  bool EvalDeclaration(const SupportsDeclNode& node) const;
  bool EvalEngineVersion(const SupportsEngineVersionNode& node) const;

  uint32_t engine_version_;
  tasm::CSSParserConfigs configs_;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_SUPPORTS_SUPPORTS_EVALUATOR_H_
