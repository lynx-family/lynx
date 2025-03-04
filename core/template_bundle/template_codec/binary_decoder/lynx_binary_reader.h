// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_LYNX_BINARY_READER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_LYNX_BINARY_READER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/template_themed.h"
#include "core/template_bundle/lynx_template_bundle.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_base_template_reader.h"
#include "core/template_bundle/template_codec/template_binary.h"

namespace lynx {
namespace tasm {

class LynxBinaryReader : public LynxBinaryBaseTemplateReader {
 public:
  ~LynxBinaryReader() override = default;

  LynxBinaryReader(const LynxBinaryReader&) = delete;
  LynxBinaryReader& operator=(const LynxBinaryReader&) = delete;

  LynxBinaryReader(LynxBinaryReader&&) = default;
  LynxBinaryReader& operator=(LynxBinaryReader&&) = default;

  static LynxBinaryReader CreateLynxBinaryReader(std::vector<uint8_t> binary);

  LynxTemplateBundle GetTemplateBundle();

 protected:
  LynxBinaryReader(std::unique_ptr<lepus::InputStream> stream)
      : LynxBinaryBaseTemplateReader(std::move(stream)) {
    enable_pre_process_attributes_ = true;
  };

  virtual bool DidDecodeHeader() override;
  virtual bool DidDecodeAppType() override;
  virtual bool DidDecodeTemplate() override;

  // decode lepus
  virtual bool DecodeContext() override;
  // in predecoding, try to create a context pool in advance
  virtual void PrepareContext();
  // decode lepus chunk
  bool DecodeLepusChunkRoute();
  virtual bool DecodeLepusChunk() override;
  bool GreedyDecodeLepusChunk(LepusChunkManager::LepusChunkMap& chunk_map);

  // decode css
  virtual bool DecodeCSSDescriptor() override;
  bool DecodeCSSDescriptorRoute();
  bool GreedyDecodeCSSDescriptor(
      CSSStyleSheetManager::CSSFragmentMap& css_fragment_map);

  // decode themed
  virtual struct Themed& Themed() override;

  // parsed styles
  bool DecodeParsedStylesSection() override;
  bool GreedyDecodeParsedStylesSection();

  // element template
  bool DecodeElementTemplateSection() override;
  bool GreedyDecodeElementTemplateSection();

  // custom sections
  bool DecodeCustomSectionsSection() override;
  bool DecodeCustomSectionsByRoute(const CustomSectionRoute& route);

  ParsedStylesMap& GetParsedStylesMap() override {
    return template_bundle().parsed_styles_map_;
  }

  AirParsedStylesMap& GetAirParsedStylesMap() override {
    return template_bundle().air_parsed_styles_map_;
  }

  std::vector<base::String>& string_list() override;

  virtual LynxTemplateBundle& template_bundle();

  StringKeyRouter lepus_chunk_route_;
  Range lepus_chunk_range_;

 private:
  void RecordBinary();

  LynxTemplateBundle template_bundle_;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_LYNX_BINARY_READER_H_
