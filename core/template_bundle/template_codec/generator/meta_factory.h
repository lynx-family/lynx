// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_META_FACTORY_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_META_FACTORY_H_

#include <string>

#include "core/template_bundle/template_codec/generator/base_struct.h"
#include "third_party/rapidjson/rapidjson.h"

namespace lynx {
namespace tasm {
class MetaFactory {
 public:
  static EncoderOptions GetEncoderOptions(rapidjson::Document& document);

 private:
  static void GetSourceContent(rapidjson::Value& document,
                               EncoderOptions& encoder_options);

  static void GetCSSMeta(rapidjson::Value& document,
                         EncoderOptions& encoder_options);

  static void GetAndCheckTargetSdkVersion(rapidjson::Value& compiler_options,
                                          EncoderOptions& encoder_options);

  static void GetTrialOptions(rapidjson::Document& document,
                              EncoderOptions& encoder_options);

  static void GetTemplateInfo(rapidjson::Document& document,
                              EncoderOptions& encoder_options);

  static void GetLepusCode(rapidjson::Document& document,
                           EncoderOptions& encoder_options);

  static void GetJSCode(rapidjson::Document& document,
                        EncoderOptions& encoder_options);

  static void GetConfig(EncoderOptions& encoder_options);

  static void GetTemplateScript(rapidjson::Document& document,
                                EncoderOptions& encoder_options);

  static void GetElementTemplate(rapidjson::Document& document,
                                 EncoderOptions& encoder_options);

  static void GetCustomSections(rapidjson::Document& document,
                                EncoderOptions& encoder_options);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_META_FACTORY_H_
