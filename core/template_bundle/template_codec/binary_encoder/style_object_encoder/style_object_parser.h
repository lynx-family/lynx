// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_STYLE_OBJECT_ENCODER_STYLE_OBJECT_PARSER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_STYLE_OBJECT_ENCODER_STYLE_OBJECT_PARSER_H_

#include <list>

#include "core/renderer/simple_styling/style_object.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/shared_css_fragment.h"
#include "core/template_bundle/template_codec/compile_options.h"
#include "third_party/rapidjson/document.h"

namespace lynx::tasm {

class StyleObjectParser {
 public:
  explicit StyleObjectParser(const CompileOptions &compile_options)
      : compile_options_(compile_options) {}
  bool Parse(const rapidjson::Value &value);
  bool ParseKeyframes(const rapidjson::Value &value);
  bool ParseFontFace(const rapidjson::Value &value);
  const std::list<style::StyleObject> &StyleObjects() { return style_objects_; }
  const encoder::CSSKeyframesTokenMapForEncode &StyleObjectsKeyframes() {
    return style_objects_keyframes_;
  }
  const encoder::CSSFontFaceTokenMapForEncode &StyleObjectsFontFaces() {
    return style_objects_fontfaces_;
  }

 private:
  const CompileOptions &compile_options_;
  std::list<style::StyleObject> style_objects_;
  encoder::CSSKeyframesTokenMapForEncode style_objects_keyframes_;
  encoder::CSSFontFaceTokenMapForEncode style_objects_fontfaces_;
};

}  // namespace lynx::tasm

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_STYLE_OBJECT_ENCODER_STYLE_OBJECT_PARSER_H_
