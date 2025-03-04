// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/css_ng_utils.h"

#include "base/include/no_destructor.h"

namespace lynx {
namespace css {

const std::string& CSSGlobalEmptyString() {
  static const base::NoDestructor<std::string> str;
  return *str;
}

const std::u16string& CSSGlobalEmptyU16String() {
  static const base::NoDestructor<std::u16string> str;
  return *str;
}

const std::string& CSSGlobalStarString() {
  static const base::NoDestructor<std::string> str("*");
  return *str;
}

const std::u16string& CSSGlobalStarU16String() {
  static const base::NoDestructor<std::u16string> str(u"*");
  return *str;
}

}  // namespace css
}  // namespace lynx
