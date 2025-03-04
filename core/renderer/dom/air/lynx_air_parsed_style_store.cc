// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/air/lynx_air_parsed_style_store.h"

namespace lynx {
namespace tasm {

LynxAirParsedStyleStore& LynxAirParsedStyleStore::GetInstance() {
  static base::NoDestructor<LynxAirParsedStyleStore> instance;
  return *instance;
}

void LynxAirParsedStyleStore::StoreAirParsedStyle(
    const std::string& url, const AirParsedStylesMap& styles) {
  url_ = url;
  air_parsed_styles_ = styles;
}

}  // namespace tasm
}  // namespace lynx
