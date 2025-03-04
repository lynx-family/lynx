// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSS_ENCODER_SHARED_CSS_FRAGMENT_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSS_ENCODER_SHARED_CSS_FRAGMENT_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/renderer/css/shared_css_fragment.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_font_face_token.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_keyframes_token.h"

namespace lynx {
namespace encoder {

using CSSKeyframesTokenMapForEncode =
    std::unordered_map<std::string, std::shared_ptr<CSSKeyframesToken>>;

using CSSFontFaceTokenMapForEncode =
    std::unordered_map<std::string,
                       std::vector<std::shared_ptr<tasm::CSSFontFaceToken>>>;

struct LynxCSSSelectorTuple {
  LynxCSSSelectorTuple()
      : selector_key(), flattened_size(0), selector_arr(), parse_token() {}
  std::string selector_key;
  size_t flattened_size;
  std::unique_ptr<css::LynxCSSSelector[]> selector_arr;
  std::shared_ptr<tasm::CSSParseToken> parse_token;
};

// TODO(songshourui.null): Subsequently, `encoder::SharedCSSFragment` will be
// renamed to `encoder::StyleSheetForEncode`.
class SharedCSSFragment : public tasm::SharedCSSFragment {
 public:
  SharedCSSFragment(int32_t id, const std::vector<int32_t>& dependent_ids,
                    tasm::CSSParserTokenMap css,
                    CSSKeyframesTokenMapForEncode keyframes,
                    CSSFontFaceTokenMapForEncode fontfaces)
      : tasm::SharedCSSFragment(id, dependent_ids, std::move(css), {}, {},
                                nullptr),
        keyframes_for_encode_(std::move(keyframes)),
        fontfaces_for_encode_(std::move(fontfaces)) {}

  explicit SharedCSSFragment(int32_t id)
      : SharedCSSFragment(id, {}, {}, {}, {}) {}

  SharedCSSFragment() : SharedCSSFragment(-1) {}

  ~SharedCSSFragment() override{};

  void SetSelectorTuple(std::vector<LynxCSSSelectorTuple> selector_tuple) {
    selector_tuple_ = std::move(selector_tuple);
  }

  const std::vector<LynxCSSSelectorTuple>& selector_tuple() {
    return selector_tuple_;
  }

  const CSSKeyframesTokenMapForEncode& GetKeyframesRuleMapForEncode() {
    return keyframes_for_encode_;
  }

  const CSSFontFaceTokenMapForEncode& GetFontFaceTokenMapForEncode() {
    return fontfaces_for_encode_;
  }

 private:
  std::vector<LynxCSSSelectorTuple> selector_tuple_;

  CSSKeyframesTokenMapForEncode keyframes_for_encode_;
  CSSFontFaceTokenMapForEncode fontfaces_for_encode_;
};

}  // namespace encoder
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSS_ENCODER_SHARED_CSS_FRAGMENT_H_
