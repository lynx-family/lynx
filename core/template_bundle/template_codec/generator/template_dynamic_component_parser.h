// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_TEMPLATE_DYNAMIC_COMPONENT_PARSER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_TEMPLATE_DYNAMIC_COMPONENT_PARSER_H_

#include <string>

#include "core/template_bundle/template_codec/generator/template_parser.h"

namespace lynx {
namespace tasm {

class TemplateDynamicComponentParser : public TemplateParser {
 public:
  TemplateDynamicComponentParser(const EncoderOptions& encoder_options);
  ~TemplateDynamicComponentParser() override;

  void Parse() override;

 protected:
  std::string GenDynamicComponentSource(
      DynamicComponent* dynamic_component) override;

  std::string GenDynamicComponentSourceForTT(
      DynamicComponent* dynamic_component);
  std::string GenDynamicComponentSourceForReactCompilerNG(
      DynamicComponent* dynamic_component);
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_TEMPLATE_DYNAMIC_COMPONENT_PARSER_H_
