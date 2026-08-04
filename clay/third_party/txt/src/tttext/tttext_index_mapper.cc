// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/third_party/txt/src/tttext/tttext_index_mapper.h"

#include <algorithm>

namespace txt {

void TTTextIndexMapper::AppendText(const std::u16string& content) {
  const size_t utf16_offset = utf16_size_;
  utf16_size_ += content.size();
  for (size_t index = 0; index < content.size(); ++index) {
    if (content[index] >= 0xD800 && content[index] <= 0xDBFF &&
        index + 1 < content.size() && content[index + 1] >= 0xDC00 &&
        content[index + 1] <= 0xDFFF) {
      ++index;
    }
    tttext_to_utf16_.push_back(utf16_offset + index + 1);
  }
}

size_t TTTextIndexMapper::ToTTTextPosition(size_t utf16_position) const {
  utf16_position = std::min(utf16_position, utf16_size_);
  auto position = std::upper_bound(tttext_to_utf16_.begin(),
                                   tttext_to_utf16_.end(), utf16_position);
  return position == tttext_to_utf16_.begin()
             ? 0
             : static_cast<size_t>(position - tttext_to_utf16_.begin() - 1);
}

size_t TTTextIndexMapper::ToTTTextRangeEnd(size_t utf16_position) const {
  utf16_position = std::min(utf16_position, utf16_size_);
  return static_cast<size_t>(std::lower_bound(tttext_to_utf16_.begin(),
                                              tttext_to_utf16_.end(),
                                              utf16_position) -
                             tttext_to_utf16_.begin());
}

size_t TTTextIndexMapper::ToUTF16Position(size_t tttext_position) const {
  return tttext_to_utf16_[std::min(tttext_position,
                                   tttext_to_utf16_.size() - 1)];
}

}  // namespace txt
