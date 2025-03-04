// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_LYNX_BINARY_LAZY_READER_DELEGATE_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_LYNX_BINARY_LAZY_READER_DELEGATE_H_

#include <memory>
#include <string>

#include "core/renderer/utils/base/element_template_info.h"
#include "core/template_bundle/lynx_template_bundle.h"

namespace lynx {
namespace tasm {

// A class used to assist in recycling template bundles. Its main function is to
// complete the lazy-decoding part of template bundles.
class LynxBinaryRecyclerDelegate {
 public:
  LynxBinaryRecyclerDelegate() = default;
  virtual ~LynxBinaryRecyclerDelegate() = default;
  LynxBinaryRecyclerDelegate(const LynxBinaryRecyclerDelegate&) = delete;
  LynxBinaryRecyclerDelegate& operator=(const LynxBinaryRecyclerDelegate&) =
      delete;
  LynxBinaryRecyclerDelegate(LynxBinaryRecyclerDelegate&&) = delete;
  LynxBinaryRecyclerDelegate& operator=(LynxBinaryRecyclerDelegate&&) = delete;

  // get a template bundle recycler
  virtual std::unique_ptr<LynxBinaryRecyclerDelegate> CreateRecycler() = 0;

  // complete the decoding of all lazy-decode sections
  // the section that needs to be parsed is as follows:
  // 1. css
  // 2. element template
  // 3. parsed styles
  virtual bool CompleteDecode() = 0;

  virtual LynxTemplateBundle GetCompleteTemplateBundle() = 0;
};

// NOTICE:
// If you want to lazy decode anything, please make sure there is a greedy
// decoding implementation in LynxBinaryReader
class LynxBinaryLazyReaderDelegate : public LynxBinaryRecyclerDelegate {
 public:
  LynxBinaryLazyReaderDelegate() = default;
  ~LynxBinaryLazyReaderDelegate() override = default;
  LynxBinaryLazyReaderDelegate(const LynxBinaryLazyReaderDelegate&) = delete;
  LynxBinaryLazyReaderDelegate& operator=(const LynxBinaryLazyReaderDelegate&) =
      delete;
  LynxBinaryLazyReaderDelegate(LynxBinaryLazyReaderDelegate&&) = delete;
  LynxBinaryLazyReaderDelegate& operator=(LynxBinaryLazyReaderDelegate&&) =
      delete;

  // css
  virtual bool DecodeCSSFragmentByIdInRender(int32_t fragment_id) = 0;

  // element template
  virtual std::shared_ptr<ElementTemplateInfo> DecodeElementTemplateInRender(
      const std::string& key) = 0;

  // parsed styles
  virtual const std::shared_ptr<ParsedStyles>& GetParsedStylesInRender(
      const std::string& key) = 0;

  // lepus chunk
  virtual bool DecodeContextBundleInRender(const std::string& key) = 0;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_LYNX_BINARY_LAZY_READER_DELEGATE_H_
