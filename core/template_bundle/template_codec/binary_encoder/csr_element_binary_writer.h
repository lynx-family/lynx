// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSR_ELEMENT_BINARY_WRITER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSR_ELEMENT_BINARY_WRITER_H_

#include <cstddef>

#include "core/runtime/vm/lepus/context_binary_writer.h"
#include "core/template_bundle/template_codec/template_binary.h"

namespace lynx {
namespace tasm {

class CSRElementBinaryWriter : public lepus::ContextBinaryWriter {
 public:
  explicit CSRElementBinaryWriter(
      lepus::Context* context, const tasm::CompileOptions& compile_options = {},
      const lepus::Value& trial_options = lepus::Value{},
      bool enableDebugInfo = false)
      : ContextBinaryWriter(context, compile_options, trial_options,
                            enableDebugInfo) {}
  virtual ~CSRElementBinaryWriter() = default;

  // This API is used to encode an element tree into binary.
  void EncodeSingleTemplateToBinary(const rapidjson::Value* single_template);

  // This API is used to encode multiple element trees into binary. The first
  // parameter is defined as `Record<string, Array<RootElement>>`.
  void EncodeTemplatesToBinary(const rapidjson::Document* templates);

  // This API is used to encode the parsed styles. The
  // shared parsed styles among the elements can be extracted into a separate
  // map and passed as the parameter, which is formatted as
  // `Record<string, Array<ParsedStyle>>`.
  void EncodeParsedStylesToBinary(const rapidjson::Value* parsed_styles);

 private:
  void EncodeParsedStyles(const rapidjson::Value* parsed_styles,
                          uint32_t router_start_pos);

  void EncodeTemplatesBody(const rapidjson::Document* templates);

  void EncodeStringKeyRouter(tasm::StringKeyRouter& router);

  void EncodeOrderedStringKeyRouter(tasm::OrderedStringKeyRouter& router);

  void EncodeElementRecursively(const rapidjson::Value* element);

  void EncodeElementParsedStylesInternal(const rapidjson::Value* parsed_styles,
                                         tasm::StringKeyRouter& router);

  void EncodeElementTagSection(const rapidjson::Value* element);

  void EncodeElementBuiltinAttrSection(const rapidjson::Value* element);

  void EncodeElementIDSelectorSection(const rapidjson::Value* element);

  void EncodeElementInlineStyleSection(const rapidjson::Value* element);

  void EncodeElementClassSection(const rapidjson::Value* element);

  void EncodeElementJSEventSection(const rapidjson::Value* element);

  void EncodeElementAttributeSection(const rapidjson::Value* element);

  void EncodeElementDatasetSection(const rapidjson::Value* element);

  void EncodeElementParsedStyleKeySection(const rapidjson::Value* element);

  void EncodeElementParsedStyleSection(const rapidjson::Value* element);

  void EncodeCountAndInsertAhead(size_t count, size_t insert_pos);

  void EncodeParsedStyle(const lepus::Value& style);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSR_ELEMENT_BINARY_WRITER_H_
