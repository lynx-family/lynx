// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/common/text_editing_delta.h"

#include "base/include/string/string_utils.h"

namespace clay {

TextEditingDelta::TextEditingDelta(const std::u16string& text_before_change,
                                   const TextRange& range,
                                   const std::u16string& text)
    : old_text_(text_before_change),
      delta_text_(text),
      delta_start_(range.start()),
      delta_end_(range.start() + range.length()) {}

TextEditingDelta::TextEditingDelta(const std::string& text_before_change,
                                   const TextRange& range,
                                   const std::string& text)
    : old_text_(lynx::base::U8StringToU16(text_before_change)),
      delta_text_(lynx::base::U8StringToU16(text)),
      delta_start_(range.start()),
      delta_end_(range.start() + range.length()) {}

TextEditingDelta::TextEditingDelta(const std::u16string& text)
    : old_text_(text), delta_text_(u""), delta_start_(-1), delta_end_(-1) {}

TextEditingDelta::TextEditingDelta(const std::string& text)
    : old_text_(lynx::base::U8StringToU16(text)),
      delta_text_(u""),
      delta_start_(-1),
      delta_end_(-1) {}

}  // namespace clay
