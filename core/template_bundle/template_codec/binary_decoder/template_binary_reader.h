// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_TEMPLATE_BINARY_READER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_TEMPLATE_BINARY_READER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/renderer/css/shared_css_fragment.h"
#include "core/renderer/template_themed.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_base_template_reader.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_lazy_reader_delegate.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_reader.h"
#include "core/template_bundle/template_codec/binary_decoder/parallel_parse_task_scheduler.h"
#include "core/template_bundle/template_codec/moulds.h"
#include "core/template_bundle/template_codec/template_binary.h"

namespace lynx {
namespace lepus {
class InputStream;
}  // namespace lepus
namespace tasm {

class VirtualNode;
class VirtualComponent;
class TemplateBinaryReader : public LynxBinaryReader,
                             public LynxBinaryLazyReaderDelegate {
 public:
  explicit TemplateBinaryReader(std::unique_ptr<lepus::InputStream> stream);

  // Constructs a reader that decodes directly into the provided bundle.
  // This is useful for lazy decode scenarios where the bundle is owned by
  // TemplateEntry.
  explicit TemplateBinaryReader(std::unique_ptr<lepus::InputStream> stream,
                                LynxTemplateBundle* template_bundle);
  ~TemplateBinaryReader() override = default;

  TemplateBinaryReader(const TemplateBinaryReader&) = delete;
  TemplateBinaryReader& operator=(const TemplateBinaryReader&) = delete;

  TemplateBinaryReader(TemplateBinaryReader&&) = delete;
  TemplateBinaryReader& operator=(TemplateBinaryReader&&) = delete;

  // lazy reader delegate;
  bool DecodeCSSFragmentByIdInRender(int32_t fragment_id) override;
  std::shared_ptr<ElementTemplateInfo> DecodeElementTemplateInRender(
      const std::string& key) override;
  const std::shared_ptr<ParsedStyles>& GetParsedStylesInRender(
      const std::string& key) override;
  bool DecodeContextBundleInRender(const std::string& key) override;

  std::unique_ptr<LynxBinaryRecyclerDelegate> CreateRecycler() override;

  bool CompleteDecode() override;

  LynxTemplateBundle GetCompleteTemplateBundle() override;

  // Decode result
  const CompileOptions& GetCompileOptions() { return compile_options_; }
  bool EnableCSSParser() { return enable_css_parser_; }
  bool IsLepusngBinary() {
    return context_type_ == runtime::ContextType::LepusNGContextType;
  }
  bool IsRTSBinary() {
    return context_type_ == runtime::ContextType::RTSContextType;
  }
  bool IsRTSNativeBinary() {
    return context_type_ == runtime::ContextType::RTSNativeContextType;
  }

  LynxTemplateBundle& template_bundle() override;

 protected:
  // Async CSS Descriptor
  virtual bool DecodeCSSDescriptor() override;
  // Async StyleObject decoder
  bool DecodeStyleObjects() override;
  bool DecodeCSSFragmentAsync(std::shared_ptr<CSSStyleSheetManager> manager);
  bool GetCSSLazyDecode();
  bool GetCSSAsyncDecode();

  // parsed styles
  bool DecodeParsedStylesSection() override;

  // element template
  bool DecodeElementTemplateSection() override;
  bool ParallelDecodeElementTemplate();
  ElementTemplateResult GetElementTemplateParseResult(
      const std::string& key) override;

  // lepus chunk
  bool DecodeLepusChunk() override;
  bool DecodeLepusChunkAsync(std::shared_ptr<LepusChunkManager> manager);

 private:
  // create a new template binary reader from binary
  static std::unique_ptr<TemplateBinaryReader> Create(const uint8_t* begin,
                                                      size_t size);

  void CopyForAsyncDecode(TemplateBinaryReader& other);

  void EnsureParallelParseTaskScheduler();

  std::unique_ptr<ParallelParseTaskScheduler> task_schedular_{nullptr};
  // When non-null, decode writes directly to this external bundle.
  // When null, decode writes to the inherited template_bundle_.
  LynxTemplateBundle* target_template_bundle_{nullptr};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_TEMPLATE_BINARY_READER_H_
