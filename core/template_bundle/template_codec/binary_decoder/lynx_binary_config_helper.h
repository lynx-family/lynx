// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_LYNX_BINARY_CONFIG_HELPER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_LYNX_BINARY_CONFIG_HELPER_H_

#include <memory>

#include "core/renderer/page_config.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {

class LynxBinaryConfigHelper {
 public:
  void HandlePageConfig(const rapidjson::Document& doc,
                        std::shared_ptr<PageConfig>& page_config);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_LYNX_BINARY_CONFIG_HELPER_H_
