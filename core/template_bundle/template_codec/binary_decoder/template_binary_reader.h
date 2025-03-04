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
#include "core/template_bundle/template_codec/moulds.h"
#include "core/template_bundle/template_codec/template_binary.h"

namespace lynx {
namespace lepus {
class InputStream;
}  // namespace lepus
namespace tasm {

class VirtualNode;
class VirtualComponent;
class TemplateAssembler;
class TemplateEntry;
class PageConfig;

class TemplateBinaryReader : public LynxBinaryReader,
                             public LynxBinaryLazyReaderDelegate {
 public:
  class PageConfigger {
   public:
    PageConfigger() = default;
    virtual ~PageConfigger() = default;

    virtual void SetSupportComponentJS(bool support) = 0;
    virtual void SetTargetSdkVersion(const std::string& targetSdkVersion) = 0;
    virtual std::shared_ptr<PageConfig> GetPageConfig() = 0;
    virtual void SetPageConfig(const std::shared_ptr<PageConfig>& config) = 0;
    virtual struct Themed& Themed() = 0;
  };

  // TODO(zhoupeng.z): configger_ and entry_ are only used for initialization.
  // It seems to be a better choice to decouple them from the decoder.
  TemplateBinaryReader(PageConfigger* configger, TemplateEntry* entry,
                       std::unique_ptr<lepus::InputStream> stream);
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
  bool IsLepusngBinary() { return is_lepusng_binary_; }

  LynxTemplateBundle& template_bundle() override;

 protected:
  // Async CSS Descriptor
  virtual bool DecodeCSSDescriptor() override;
  bool DecodeCSSFragmentAsync(std::shared_ptr<CSSStyleSheetManager> manager);
  bool GetCSSLazyDecode();
  bool GetCSSAsyncDecode();

  // At runtime decoding, no need to prepare context
  void PrepareContext() override {}

  virtual bool DidDecodeTemplate() override;

  // parsed styles
  bool DecodeParsedStylesSection() override;

  // element template
  bool DecodeElementTemplateSection() override;

  // lepus chunk
  bool DecodeLepusChunk() override;
  bool DecodeLepusChunkAsync(std::shared_ptr<LepusChunkManager> manager);

  // should only be used in DidDecodeTemplate
  PageConfigger* configger_;
  TemplateEntry* entry_;

 private:
  // create a new template binary reader from binary
  static std::unique_ptr<TemplateBinaryReader> Create(const uint8_t* begin,
                                                      size_t size);

  void CopyForCSSAsyncDecode(const TemplateBinaryReader& other);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_TEMPLATE_BINARY_READER_H_
