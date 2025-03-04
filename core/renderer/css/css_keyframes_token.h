// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_CSS_KEYFRAMES_TOKEN_H_
#define CORE_RENDERER_CSS_CSS_KEYFRAMES_TOKEN_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/debug/lynx_assert.h"
#include "base/include/string/string_utils.h"
#include "core/renderer/css/css_parser_token.h"
#include "core/renderer/css/unit_handler.h"
#include "core/template_bundle/template_codec/compile_options.h"

namespace lynx {

namespace starlight {
class CSSStyleUtils;
}  // namespace starlight

namespace tasm {

typedef std::unordered_map<std::string, std::shared_ptr<StyleMap>>
    CSSKeyframesMap;
typedef std::unordered_map<std::string, std::shared_ptr<RawStyleMap>>
    CSSRawKeyframesMap;

typedef std::unordered_map<float, std::shared_ptr<StyleMap>>
    CSSKeyframesContent;
typedef std::unordered_map<float, std::shared_ptr<RawStyleMap>>
    CSSRawKeyframesContent;

class CSSKeyframesToken {
 public:
  CSSKeyframesToken(const CSSParserConfigs& parser_configs)
      : parser_configs_(parser_configs) {}

  virtual ~CSSKeyframesToken() {}

  void SetKeyframesContent(CSSKeyframesContent&& content) {
    content_ = std::move(content);
  }
  void SetRawKeyframesContent(CSSRawKeyframesContent&& content) {
    raw_content_ = std::move(content);
  }

  static float ParseKeyStr(const std::string& key_text,
                           bool enableCSSStrictMode = false) {
    float key = 0;
    if (key_text == "from") {
      key = 0;
    } else if (key_text == "to") {
      key = 1;
    } else {
      key = atof(key_text.c_str()) / 100.0;
    }
    if (key > 1 || key < 0) {
      UnitHandler::CSSWarning(false, enableCSSStrictMode,
                              "key frames must >=0 && <=0. error input:%s",
                              key_text.c_str());
      return 0;
    }
    return key;
  }

  CSSKeyframesContent& GetKeyframesContent() {
    if (!raw_content_.empty()) {
      for (auto keyframe = raw_content_.begin(); keyframe != raw_content_.end();
           keyframe++) {
        auto key = keyframe->first;
        StyleMap* temp_map = content_[key].get();
        if (temp_map == nullptr) {
          continue;
        }
        auto& raw_style_map = keyframe->second;
        for (auto style : *raw_style_map) {
          UnitHandler::ProcessCSSValue(style.first, style.second, *temp_map,
                                       parser_configs_);
        }
      }
      raw_content_.clear();
    }
    return content_;
  }

 protected:
  // for decode css.
  friend class LynxBinaryBaseCSSReader;

  CSSKeyframesContent content_;
  CSSRawKeyframesContent raw_content_;

  const CSSParserConfigs parser_configs_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_KEYFRAMES_TOKEN_H_
