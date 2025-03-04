// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "core/renderer/css/ng/css_ng_utils.h"
#include "core/renderer/css/ng/selector/lynx_css_selector.h"
#include "core/renderer/css/ng/selector/lynx_css_selector_list.h"

namespace lynx {
namespace css {

static constexpr unsigned kIdSpecificity = 0x010000;
static constexpr unsigned kClassSpecificity = 0x000100;
static constexpr unsigned kTagSpecificity = 0x000001;

void LynxCSSSelector::UpdatePseudoType(PseudoType pseudo_type) {
  DCHECK(match_ == kPseudoClass || match_ == kPseudoElement);
  SetPseudoType(pseudo_type);

  switch (GetPseudoType()) {
    case kPseudoAfter:
    case kPseudoBefore:
    case kPseudoFirstLetter:
    case kPseudoFirstLine:
      if (match_ == kPseudoClass) match_ = kPseudoElement;
      [[fallthrough]];
    // For pseudo elements
    case kPseudoPlaceholder:
    case kPseudoSelection:
      if (match_ != kPseudoElement) pseudo_type_ = kPseudoUnknown;
      break;
    // For pseudo classes
    case kPseudoActive:
    case kPseudoChecked:
    case kPseudoDefault:
    case kPseudoDir:
    case kPseudoDisabled:
    case kPseudoEmpty:
    case kPseudoEnabled:
    case kPseudoFirstChild:
    case kPseudoFirstOfType:
    case kPseudoFocus:
    case kPseudoHas:
    case kPseudoHover:
    case kPseudoIs:
    case kPseudoLang:
    case kPseudoLastChild:
    case kPseudoLastOfType:
    case kPseudoLink:
    case kPseudoNot:
    case kPseudoNthChild:
    case kPseudoNthLastChild:
    case kPseudoNthLastOfType:
    case kPseudoNthOfType:
    case kPseudoOnlyChild:
    case kPseudoOnlyOfType:
    case kPseudoRoot:
    case kPseudoState:
    case kPseudoUnknown:
    case kPseudoVisited:
    case kPseudoWhere:
      if (match_ != kPseudoClass) pseudo_type_ = kPseudoUnknown;
      break;
    default:
      pseudo_type_ = kPseudoUnknown;
      break;
  }
}

unsigned LynxCSSSelector::CalcSpecificity() const {
  // make sure the result doesn't overflow
  static const unsigned kIdMask = 0xff0000;
  static const unsigned kClassMask = 0x00ff00;
  static const unsigned kElementMask = 0x0000ff;

  unsigned total = 0;
  unsigned temp = 0;

  for (const LynxCSSSelector* selector = this; selector;
       selector = selector->TagHistory()) {
    temp = total + selector->CalcSpecificityForSimple();
    // Clamp each component to its max in the case of overflow.
    if ((temp & kIdMask) < (total & kIdMask))
      total |= kIdMask;
    else if ((temp & kClassMask) < (total & kClassMask))
      total |= kClassMask;
    else if ((temp & kElementMask) < (total & kElementMask))
      total |= kElementMask;
    else
      total = temp;
  }
  return total;
}

unsigned LynxCSSSelector::CalcSpecificityForSimple() const {
  switch (Match()) {
    case kId:
      return kIdSpecificity;
    case kPseudoClass:
      switch (GetPseudoType()) {
        case kPseudoWhere:
          return 0;
        case kPseudoNot:
          DCHECK(SelectorList());
          [[fallthrough]];
        case kPseudoIs:
          return SelectorList() ? SelectorList()->CalcSpecificity() : 0;
        case kPseudoHas:
          return SelectorList() ? SelectorList()->CalcSpecificity() : 0;
        default:
          break;
      }
      return kClassSpecificity;
    case kPseudoElement:
      return kClassSpecificity;
    case kClass:
    case kAttributeExact:
    case kAttributeSet:
    case kAttributeList:
    case kAttributeHyphen:
    case kAttributeContain:
    case kAttributeBegin:
    case kAttributeEnd:
      return kClassSpecificity;
    case kTag:
      if (Value() == CSSGlobalEmptyString()) return 0;
      return kTagSpecificity;
    case kUnknown:
      return 0;
  }
  NOTREACHED();
  return 0;
}

}  // namespace css
}  // namespace lynx
