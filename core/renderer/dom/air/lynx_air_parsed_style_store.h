// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_AIR_LYNX_AIR_PARSED_STYLE_STORE_H_
#define CORE_RENDERER_DOM_AIR_LYNX_AIR_PARSED_STYLE_STORE_H_

#include <string>

#include "base/include/no_destructor.h"
#include "core/renderer/css/css_property.h"

namespace lynx {
namespace tasm {

class LynxAirParsedStyleStore {
 public:
  static LynxAirParsedStyleStore& GetInstance();

  const std::string& GetCurrentUrl() const { return url_; }
  const AirParsedStylesMap& GetAirParsedStyleStore() const {
    return air_parsed_styles_;
  }

  void StoreAirParsedStyle(const std::string& url,
                           const AirParsedStylesMap& styles);

 private:
  std::string url_;
  AirParsedStylesMap air_parsed_styles_;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_AIR_LYNX_AIR_PARSED_STYLE_STORE_H_
