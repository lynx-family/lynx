// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_THIRD_PARTY_TXT_SRC_TTTEXT_TTTEXT_INDEX_MAPPER_H_
#define CLAY_THIRD_PARTY_TXT_SRC_TTTEXT_TTTEXT_INDEX_MAPPER_H_

#include <cstddef>
#include <string>
#include <vector>

namespace txt {

// TTText indexes Unicode scalar values, while txt exposes UTF-16 offsets.
class TTTextIndexMapper {
 public:
  void AppendText(const std::u16string& content);

  size_t ToTTTextPosition(size_t utf16_position) const;
  size_t ToTTTextRangeEnd(size_t utf16_position) const;
  size_t ToUTF16Position(size_t tttext_position) const;
  size_t GetUTF16Size() const { return utf16_size_; }

 private:
  size_t utf16_size_ = 0;
  std::vector<size_t> tttext_to_utf16_{0};
};

}  // namespace txt

#endif  // CLAY_THIRD_PARTY_TXT_SRC_TTTEXT_TTTEXT_INDEX_MAPPER_H_
