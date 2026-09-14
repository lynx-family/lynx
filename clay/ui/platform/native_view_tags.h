// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PLATFORM_NATIVE_VIEW_TAGS_H_
#define CLAY_UI_PLATFORM_NATIVE_VIEW_TAGS_H_

#include <algorithm>
#include <cstddef>
#include <string_view>

namespace clay {

class NativeViewTagSet {
 public:
  using const_iterator = const std::string_view*;

  constexpr NativeViewTagSet(const std::string_view* tags, std::size_t size)
      : tags_(tags), size_(size) {}

  constexpr const_iterator begin() const { return tags_; }
  constexpr const_iterator end() const {
    return size_ == 0 ? tags_ : tags_ + size_;
  }
  constexpr bool empty() const { return size_ == 0; }
  constexpr std::size_t size() const { return size_; }

  const_iterator find(std::string_view tag) const {
    if (empty()) {
      return end();
    }
    const auto it = std::lower_bound(begin(), end(), tag);
    return it != end() && *it == tag ? it : end();
  }

  std::size_t count(std::string_view tag) const { return find(tag) != end(); }

 private:
  const std::string_view* tags_;
  std::size_t size_;
};

const NativeViewTagSet& InternalPlatformViewTags();
const NativeViewTagSet& InternalPlatformViewShadowNodeTags();
const NativeViewTagSet& InternalPlatformViewWithoutShadowNodeTags();
bool ShouldCreateFallbackNativeViewDirectly();

}  // namespace clay

#endif  // CLAY_UI_PLATFORM_NATIVE_VIEW_TAGS_H_
