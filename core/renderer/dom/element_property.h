// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_ELEMENT_PROPERTY_H_
#define CORE_RENDERER_DOM_ELEMENT_PROPERTY_H_

#include "base/include/value/base_string.h"

namespace lynx {
namespace tasm {

// This defines the min value for builtin attribute id. The range 0~999 is
// reserved for CSS Property IDs, while values greater than 999 are used for
// other IDs, such as built-in attribute IDs. Defining these ID ranges helps
// prevent conflicts, allowing them to coexist in the map buffer and
// facilitating future data transmission optimizations.
static constexpr int BUILTIN_ATTRIBUTE_MIN_ID = 1000;
static constexpr int BUILTIN_TAG_EMPTY_ID = 13;

enum ElementBuiltInTagEnum {
  ELEMENT_VIEW,
  ELEMENT_TEXT,
  ELEMENT_RAW_TEXT,
  ELEMENT_IMAGE,
  ELEMENT_SCROLL_VIEW,
  ELEMENT_LIST,
  ELEMENT_COMPONENT,
  ELEMENT_PAGE,
  ELEMENT_NONE,
  ELEMENT_WRAPPER,
  ELEMENT_OTHER,
  ELEMENT_X_TEXT,
  ELEMENT_X_SCROLL_VIEW,
  // BUILTIN_TAG_EMPTY_ID == 13
  ELEMENT_EMPTY = BUILTIN_TAG_EMPTY_ID,
  ELEMENT_INLINE_TEXT,
  ELEMENT_X_INLINE_TEXT,
  ELEMENT_X_NESTED_SCROLL_VIEW,
};

enum class ElementBuiltInAttributeEnum {
  // BUILTIN_ATTRIBUTE_MIN_ID == 1000
  COMPONENT_ID = BUILTIN_ATTRIBUTE_MIN_ID,
  COMPONENT_NAME,
  COMPONENT_PATH,
  CSS_ID,
  NODE_INDEX,
  DIRTY_ID
};

enum class ElementSectionEnum {
  ELEMENT_CONSTRUCTION_INFO = 0,
  ELEMENT_TAG_ENUM,
  ELEMENT_TAG_STR,
  ELEMENT_BUILTIN_ATTRIBUTE,
  ELEMENT_ID_SELECTOR,
  ELEMENT_CHILDREN,
  ELEMENT_CLASS,
  ELEMENT_STYLES,
  ELEMENT_ATTRIBUTES,
  ELEMENT_EVENTS,
  ELEMENT_DATA_SET,
  ELEMENT_PARSED_STYLES,
  ELEMENT_PARSED_STYLES_KEY,
  ELEMENT_PIPER_EVENTS,
};

class ElementProperty {
 public:
  static ElementBuiltInTagEnum ConvertStringTagToEnumTag(
      const base::static_string::GenericCacheKey& string_tag);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_ELEMENT_PROPERTY_H_
