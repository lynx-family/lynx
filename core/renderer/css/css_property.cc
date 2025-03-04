// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/css_property.h"

#include <array>
#include <mutex>
#include <set>

#include "base/include/no_destructor.h"
#include "core/public/prop_bundle.h"

namespace lynx {
namespace tasm {

const char* GetPropertyNameCStr(CSSPropertyID id) {
  return CSSProperty::GetPropertyNameCStr(id);
}

const std::set<CSSPropertyID> shorthandCSSProperties{
    kPropertyIDBorder,
    kPropertyIDBorderTop,
    kPropertyIDBorderRight,
    kPropertyIDBorderBottom,
    kPropertyIDBorderLeft,
    kPropertyIDMarginInlineStart,
    kPropertyIDMarginInlineEnd,
    kPropertyIDPaddingInlineStart,
    kPropertyIDPaddingInlineEnd,
    kPropertyIDBorderInlineStartWidth,
    kPropertyIDBorderInlineEndWidth,
    kPropertyIDBorderInlineStartColor,
    kPropertyIDBorderInlineEndColor,
    kPropertyIDBorderInlineStartStyle,
    kPropertyIDBorderInlineEndStyle,
    kPropertyIDBorderStartStartRadius,
    kPropertyIDBorderEndStartRadius,
    kPropertyIDBorderStartEndRadius,
    kPropertyIDBorderEndEndRadius,
    kPropertyIDFlex,
    kPropertyIDFlexFlow,
    kPropertyIDPadding,
    kPropertyIDMargin,
    kPropertyIDInsetInlineStart,
    kPropertyIDInsetInlineEnd,
    kPropertyIDBorderWidth,
    kPropertyIDBackground,
    kPropertyIDBorderColor,
    kPropertyIDBorderStyle,
    kPropertyIDOutline};

using CSSPropertyNameToIDMap =
    std::unordered_map<base::static_string::GenericCacheKey, CSSPropertyID>;

CSSPropertyID CSSProperty::GetPropertyID(
    const base::static_string::GenericCacheKey& key) {
  const static base::NoDestructor<CSSPropertyNameToIDMap> kPropertyNameMapping{{
#define DECLARE_PROPERTY_NAME(name, c, value) \
  {kPropertyName##name, kPropertyID##name},
      FOREACH_ALL_PROPERTY(DECLARE_PROPERTY_NAME)
#undef DECLARE_PROPERTY_NAME
  }};
  auto it = kPropertyNameMapping->find(key);
  return it != kPropertyNameMapping->end() ? it->second : kPropertyEnd;
}

const base::static_string::GenericCache& CSSProperty::GetPropertyName(
    CSSPropertyID id) {
  // The reason using std::call_once is that GenericCache is not copy
  // constructible and `NoDestructor<PropertyIDMappingArray> xx{{}}`
  // won't compile
  using PropertyIDMappingArray = std::array<base::static_string::GenericCache,
                                            CSSPropertyID::kPropertyEnd + 1>;
  static PropertyIDMappingArray* kPropertyIdMapping;
  static std::once_flag prop_id_mapping_init_flag;
  std::call_once(prop_id_mapping_init_flag, []() {
    kPropertyIdMapping = new PropertyIDMappingArray{
        "",  // start
#define DECLARE_PROPERTY_ID(name, c, value) kPropertyName##name,
        FOREACH_ALL_PROPERTY(DECLARE_PROPERTY_ID)
#undef DECLARE_PROPERTY_ID
            ""  // end
    };
  });
  if (id >= kPropertyStart && id <= kPropertyEnd) {
    return (*kPropertyIdMapping)[id];
  }
  return (*kPropertyIdMapping)[kPropertyStart];  // Empty string
}

size_t CSSProperty::GetShorthandExpand(CSSPropertyID id) {
  using PropertyIDShorthandArray =
      std::array<uint8_t, CSSPropertyID::kPropertyEnd + 1>;
  static PropertyIDShorthandArray* kPropertyIdShorthand;
  static std::once_flag prop_id_mapping_init_flag;
  std::call_once(prop_id_mapping_init_flag, []() {
    kPropertyIdShorthand = new PropertyIDShorthandArray();
    std::memset(kPropertyIdShorthand->data(), 0,
                sizeof(uint8_t) * kPropertyIdShorthand->size());
    (*kPropertyIdShorthand)[kPropertyIDPadding] = 4;
    (*kPropertyIdShorthand)[kPropertyIDMargin] = 4;
    (*kPropertyIdShorthand)[kPropertyIDFlex] = 3;
    (*kPropertyIdShorthand)[kPropertyIDBackground] = 8;
    (*kPropertyIdShorthand)[kPropertyIDBorder] = 12;
    (*kPropertyIdShorthand)[kPropertyIDBorderWidth] = 4;
    (*kPropertyIdShorthand)[kPropertyIDBorderRadius] = 4;
    (*kPropertyIdShorthand)[kPropertyIDBorderColor] = 4;
    (*kPropertyIdShorthand)[kPropertyIDBorderStyle] = 4;
    (*kPropertyIdShorthand)[kPropertyIDBorderRight] = 3;
    (*kPropertyIdShorthand)[kPropertyIDBorderLeft] = 3;
    (*kPropertyIdShorthand)[kPropertyIDBorderTop] = 3;
    (*kPropertyIdShorthand)[kPropertyIDBorderBottom] = 3;
    (*kPropertyIdShorthand)[kPropertyIDOutline] = 3;
    (*kPropertyIdShorthand)[kPropertyIDFlexFlow] = 2;
    (*kPropertyIdShorthand)[kPropertyIDTransition] = 5;
    (*kPropertyIdShorthand)[kPropertyIDMask] = 8;
    (*kPropertyIdShorthand)[kPropertyIDAnimation] = 9;
  });
  if (id >= kPropertyStart && id <= kPropertyEnd) {
    return (*kPropertyIdShorthand)[id];
  }
  return 0;
}

CSSPropertyID CSSProperty::GetTimingOptionsPropertyID(
    const base::static_string::GenericCacheKey& key) {
#define DECLARE_ANIMATIONAPI_PROPERTY_NAME(name, alias) \
  static constexpr const char kAnimationAPIPropertyName##name[] = alias;
  FOREACH_ALL_ANIMATIONAPI_PROPERTY(DECLARE_ANIMATIONAPI_PROPERTY_NAME)
#undef DECLARE_ANIMATIONAPI_PROPERTY_NAME

  const static base::NoDestructor<CSSPropertyNameToIDMap>
      kAnimationPropertyNameMapping{{
#define DECLARE_PROPERTY_NAME(name, alias) \
  {kAnimationAPIPropertyName##name, kPropertyID##name},
          FOREACH_ALL_ANIMATIONAPI_PROPERTY(DECLARE_PROPERTY_NAME)
#undef DECLARE_PROPERTY_NAME
      }};
  auto it = kAnimationPropertyNameMapping->find(key);
  return it != kAnimationPropertyNameMapping->end() ? it->second : kPropertyEnd;
}

const std::unordered_map<std::string, std::string>&
CSSProperty::GetComputeStyleMap() {
  static const base::NoDestructor<std::unordered_map<std::string, std::string>>
      kComputeStyleMap{{
#define DECLARE_PROPERTY_ID(name, c, value) {c, value},
          FOREACH_ALL_PROPERTY(DECLARE_PROPERTY_ID)
#undef DECLARE_PROPERTY_ID
              {"", ""}}};
  return *kComputeStyleMap;
}

bool CSSProperty::IsShorthand(CSSPropertyID id) {
  return shorthandCSSProperties.find(id) != shorthandCSSProperties.end();
}

}  // namespace tasm
}  // namespace lynx
