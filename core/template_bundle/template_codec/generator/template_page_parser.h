// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_TEMPLATE_PAGE_PARSER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_TEMPLATE_PAGE_PARSER_H_

#include <string>

#include "core/template_bundle/template_codec/generator/template_parser.h"

namespace lynx {
namespace tasm {

class TemplatePageParser : public TemplateParser {
 public:
  TemplatePageParser(const EncoderOptions& encoder_options);
  ~TemplatePageParser() override;

  void Parse() override;

 protected:
  std::string GenPageSource(Page* page) override;
  std::string GenPageSourceForTT(Page* page);
  std::string GenPageSourceForReactCompilerNG(Page* page);
  std::string GenPageRenderer(Page* page);

  void CheckPageElementValid(const rapidjson::Value& element);

 private:
  // only check whether the first element is page element.
  bool new_page_element_enabled_{false};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_TEMPLATE_PAGE_PARSER_H_
