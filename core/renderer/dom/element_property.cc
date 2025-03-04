// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/element_property.h"

#include <unordered_map>

#include "base/include/no_destructor.h"
namespace lynx {
namespace tasm {

#define FOREACH_ALL_ELEMENT_BUILTIN_TAG(V)                         \
  V("view", ElementBuiltInTagEnum::ELEMENT_VIEW)                   \
  V("text", ElementBuiltInTagEnum::ELEMENT_TEXT)                   \
  V("raw-text", ElementBuiltInTagEnum::ELEMENT_RAW_TEXT)           \
  V("image", ElementBuiltInTagEnum::ELEMENT_IMAGE)                 \
  V("scroll-view", ElementBuiltInTagEnum::ELEMENT_SCROLL_VIEW)     \
  V("list", ElementBuiltInTagEnum::ELEMENT_LIST)                   \
  V("component", ElementBuiltInTagEnum::ELEMENT_COMPONENT)         \
  V("page", ElementBuiltInTagEnum::ELEMENT_PAGE)                   \
  V("none", ElementBuiltInTagEnum::ELEMENT_NONE)                   \
  V("wrapper", ElementBuiltInTagEnum::ELEMENT_WRAPPER)             \
  V("other", ElementBuiltInTagEnum::ELEMENT_OTHER)                 \
  V("x-text", ElementBuiltInTagEnum::ELEMENT_X_TEXT)               \
  V("x-scroll-view", ElementBuiltInTagEnum::ELEMENT_X_SCROLL_VIEW) \
  V("inline-text", ElementBuiltInTagEnum::ELEMENT_INLINE_TEXT)     \
  V("x-inline-text", ElementBuiltInTagEnum::ELEMENT_X_INLINE_TEXT) \
  V("x-nested-scroll-view", ElementBuiltInTagEnum::ELEMENT_X_NESTED_SCROLL_VIEW)

ElementBuiltInTagEnum ElementProperty::ConvertStringTagToEnumTag(
    const base::static_string::GenericCacheKey& string_tag) {
  const static base::NoDestructor<std::unordered_map<
      base::static_string::GenericCacheKey, ElementBuiltInTagEnum>>
      tag_enum_map{{
#define DECLARE_TAG_ENUM(string_tag, enum_tag) {string_tag, enum_tag},
          FOREACH_ALL_ELEMENT_BUILTIN_TAG(DECLARE_TAG_ENUM)
#undef DECLARE_TAG_ENUM
      }};
  auto iter = tag_enum_map->find(string_tag);
  if (iter != tag_enum_map->end()) {
    return iter->second;
  } else {
    return ElementBuiltInTagEnum::ELEMENT_EMPTY;
  }
}
}  // namespace tasm
}  // namespace lynx
