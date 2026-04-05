// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_base_template_reader.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/timer/time_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/template_bundle/template_codec/binary_decoder/binary_decoder_trace_event_def.h"

namespace lynx {
namespace tasm {

bool LynxBinaryBaseTemplateReader::DecodeMagicWord(uint32_t magic_word) {
  if (magic_word == template_codec::kQuickBinaryMagic) {
    context_type_ = lynx::runtime::ContextType::LepusNGContextType;
    is_lepusng_binary_ = true;
    return true;
  }

  if (magic_word == template_codec::kLepusBinaryMagic) {
    context_type_ = lynx::runtime::ContextType::VMContextType;
    is_lepusng_binary_ = false;
#if ENABLE_JUST_LEPUSNG
    error_message_ =
        "Support lepusNG only. Template file uses lepus. Please add "
        "`useLepusNG: true` in encode section.";
    return false;
#endif
    return true;
  }

  return false;
}

const std::vector<BinarySection> &
LynxBinaryBaseTemplateReader::GetFlexibleTemplateSectionOrder() const {
  static const std::vector<BinarySection> kFiberSectionOrder{
      BinarySection::STRING,
      BinarySection::PARSED_STYLES,
      BinarySection::ELEMENT_TEMPLATE,
      BinarySection::CSS,
      BinarySection::JS,
      BinarySection::JS_BYTECODE,
      BinarySection::CONFIG,
      BinarySection::ROOT_LEPUS,
      BinarySection::LEPUS_CHUNK,
      BinarySection::CUSTOM_SECTIONS,
      BinarySection::NEW_ELEMENT_TEMPLATE,
  };
  return kFiberSectionOrder;
}

bool LynxBinaryBaseTemplateReader::DecodeConfigSection() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              BINARY_BASE_TEMPLATE_READER_DECODE_PAGE_CONFIG);
  page_config_offset_ = stream_->offset();
  DECODE_STDSTR(config_str);
  EnsurePageConfig();
  ERROR_UNLESS(
      config_decoder_->DecodePageConfig(std::move(config_str), page_configs_));
  enable_css_inline_variables_ = page_configs_->GetEnableCSSInlineVariables();
  return true;
}

bool LynxBinaryBaseTemplateReader::DecodeAppDescriptor() { return true; }

bool LynxBinaryBaseTemplateReader::DecodePageDescriptor() { return true; }

bool LynxBinaryBaseTemplateReader::DecodePageMould(PageMould *mould) {
  return true;
}

bool LynxBinaryBaseTemplateReader::DecodePageRoute(PageRoute &route) {
  return true;
}

bool LynxBinaryBaseTemplateReader::DeserializeVirtualNodeSection() {
  return true;
}

bool LynxBinaryBaseTemplateReader::DecodeComponentDescriptor() { return true; }

bool LynxBinaryBaseTemplateReader::DecodeComponentRoute(ComponentRoute &route) {
  return true;
}

bool LynxBinaryBaseTemplateReader::DecodeComponentMould(ComponentMould *mould,
                                                        int offset,
                                                        int length) {
  return true;
}

bool LynxBinaryBaseTemplateReader::DecodeDynamicComponentDescriptor() {
  return true;
}

bool LynxBinaryBaseTemplateReader::DecodeDynamicComponentDeclarations() {
  return true;
}

bool LynxBinaryBaseTemplateReader::DecodeDynamicComponentRoute(
    DynamicComponentRoute &route) {
  return true;
}

bool LynxBinaryBaseTemplateReader::DecodeDynamicComponentMould(
    DynamicComponentMould *mould) {
  return true;
}

bool LynxBinaryBaseTemplateReader::DecodeThemedSection() { return true; }

}  // namespace tasm
}  // namespace lynx
