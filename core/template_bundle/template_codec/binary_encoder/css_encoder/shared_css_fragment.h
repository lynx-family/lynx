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
#include "core/template_bundle/template_codec/template_binary.h"

namespace lynx {
namespace encoder {

using CSSKeyframesTokenMapForEncode =
    std::unordered_map<std::string, fml::RefPtr<CSSKeyframesToken>>;

using CSSFontFaceTokenMapForEncode =
    std::unordered_map<std::string,
                       std::vector<std::shared_ptr<tasm::CSSFontFaceToken>>>;

struct LynxCSSSelectorTuple {
  size_t flattened_size{0};
  std::string selector_key;
  std::unique_ptr<css::LynxCSSSelector[]> selector_arr{nullptr};
  fml::RefPtr<tasm::CSSParseToken> parse_token{nullptr};
};

struct LynxStyleRuleBase {
  using RuleType = tasm::CSSRuleType;

  explicit LynxStyleRuleBase(RuleType t) : type(t) {}
  virtual ~LynxStyleRuleBase() = default;
  RuleType type = RuleType::kUnknown;
};

struct LynxStyleRule : LynxStyleRuleBase {
  LynxStyleRule(size_t size, size_t pos,
                std::unique_ptr<css::LynxCSSSelector[]> arr,
                fml::RefPtr<tasm::CSSParseToken> properties)
      : LynxStyleRuleBase(RuleType::kStyle),
        flattened_size(size),
        position(pos),
        selector_arr(std::move(arr)),
        properties(std::move(properties)) {}
  ~LynxStyleRule() override = default;
  size_t flattened_size = 0;
  size_t position = 0;
  std::unique_ptr<css::LynxCSSSelector[]> selector_arr{nullptr};
  fml::RefPtr<tasm::CSSParseToken> properties{nullptr};
};

struct LynxStyleRuleGroup : LynxStyleRuleBase {
  std::vector<std::unique_ptr<LynxStyleRuleBase>> child_rules;
  explicit LynxStyleRuleGroup(RuleType t) : LynxStyleRuleBase(t) {}
};

struct LynxStyleRuleCondition : LynxStyleRuleGroup {
  explicit LynxStyleRuleCondition(RuleType t) : LynxStyleRuleGroup(t) {}
  std::string condition;
};

struct LynxStyleRuleFontFace : LynxStyleRuleBase {
  explicit LynxStyleRuleFontFace() : LynxStyleRuleBase(RuleType::kFontFace) {}
  std::string family;
  std::vector<std::shared_ptr<tasm::CSSFontFaceToken>> properties;
};

struct LynxStyleRuleKeyframes : LynxStyleRuleBase {
  explicit LynxStyleRuleKeyframes() : LynxStyleRuleBase(RuleType::kKeyframes) {}
  std::string name;
  fml::RefPtr<CSSKeyframesToken> properties;
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

  SharedCSSFragment(int32_t id, const std::vector<int32_t>& dependent_ids,
                    std::vector<std::unique_ptr<LynxStyleRuleBase>> rules)
      : tasm::SharedCSSFragment(id, dependent_ids, {}, {}, {}, nullptr),
        rules_(std::move(rules)) {}

  explicit SharedCSSFragment(int32_t id)
      : SharedCSSFragment(id, {}, {}, {}, {}) {}

  SharedCSSFragment() : SharedCSSFragment(-1) {}

  ~SharedCSSFragment() override = default;

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

  const std::vector<std::unique_ptr<LynxStyleRuleBase>>& rules() const {
    return rules_;
  }

 private:
  std::vector<LynxCSSSelectorTuple> selector_tuple_;
  std::vector<std::unique_ptr<LynxStyleRuleBase>> rules_;

  CSSKeyframesTokenMapForEncode keyframes_for_encode_;
  CSSFontFaceTokenMapForEncode fontfaces_for_encode_;
};

}  // namespace encoder
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSS_ENCODER_SHARED_CSS_FRAGMENT_H_
