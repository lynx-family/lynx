// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/fml/platform/win/wstring_conversion.h"

#include "base/include/string/string_conversion_win.h"

namespace fml {

std::string WideStringToUtf8(const std::wstring_view str) {
  return lynx::base::Utf8FromUtf16(str);
}

std::wstring Utf8ToWideString(const std::string_view str) {
  return lynx::base::Utf16FromUtf8(str);
}

}  // namespace fml
