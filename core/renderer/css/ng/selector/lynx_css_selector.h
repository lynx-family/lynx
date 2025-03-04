// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_SELECTOR_LYNX_CSS_SELECTOR_H_
#define CORE_RENDERER_CSS_NG_SELECTOR_LYNX_CSS_SELECTOR_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/include/base_export.h"
#include "core/renderer/css/ng/selector/lynx_css_selector_extra_data.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace css {

class CSSParserContext;
class LynxCSSSelectorList;

enum PseudoId : uint8_t {
  kPseudoIdNone,
  kPseudoIdFirstLine,
  kPseudoIdFirstLetter,
  kPseudoIdBefore,
  kPseudoIdAfter,
  kPseudoIdBackdrop,
  kPseudoIdSelection,
};

class LynxCSSSelector {
 public:
  using AttributeMatchType = LynxCSSSelectorExtraData::AttributeMatchType;

  LynxCSSSelector();
  LynxCSSSelector(const LynxCSSSelector&) = delete;
  explicit LynxCSSSelector(const std::string&, bool tag_is_implicit = false);

  ~LynxCSSSelector() = default;

  BASE_EXPORT_FOR_DEVTOOL std::string ToString() const;

  bool operator==(const LynxCSSSelector&) const = delete;
  bool operator!=(const LynxCSSSelector&) const = delete;

  unsigned CalcSpecificity() const;

  enum MatchType {
    kUnknown,
    kTag,
    kId,
    kClass,
    kPseudoClass,
    kPseudoElement,
    kAttributeExact,
    kAttributeSet,
    kAttributeHyphen,
    kAttributeList,
    kAttributeContain,
    kAttributeBegin,
    kAttributeEnd,
    kFirstAttributeSelectorMatch = kAttributeExact,
  };

  enum RelationType {
    kSubSelector,
    kDescendant,
    kChild,
    kDirectAdjacent,
    kIndirectAdjacent,
    kUAShadow,
    kRelativeDescendant,
    kRelativeChild,
    kRelativeDirectAdjacent,
    kRelativeIndirectAdjacent
  };

  enum PseudoType {
    kPseudoUnknown,
    kPseudoEmpty,
    kPseudoFirstChild,
    kPseudoFirstOfType,
    kPseudoLastChild,
    kPseudoLastOfType,
    kPseudoOnlyChild,
    kPseudoOnlyOfType,
    kPseudoFirstLine,
    kPseudoFirstLetter,
    kPseudoNthChild,
    kPseudoNthOfType,
    kPseudoNthLastChild,
    kPseudoNthLastOfType,
    kPseudoState,
    kPseudoLink,
    kPseudoVisited,
    kPseudoIs,
    kPseudoWhere,
    kPseudoHover,
    kPseudoFocus,
    kPseudoActive,
    kPseudoChecked,
    kPseudoEnabled,
    kPseudoDefault,
    kPseudoDisabled,
    kPseudoBefore,
    kPseudoAfter,
    kPseudoBackdrop,
    kPseudoLang,
    kPseudoNot,
    kPseudoPlaceholder,
    kPseudoRoot,
    kPseudoSelection,
    kPseudoDir,
    kPseudoHas,
    kPseudoRelativeAnchor
  };

  PseudoType GetPseudoType() const {
    return static_cast<PseudoType>(pseudo_type_);
  }

  void UpdatePseudoType(PseudoType pseudo_type);

  void UpdateSpecificity(uint32_t v) { specificity_ = v; }

  const LynxCSSSelector* TagHistory() const {
    return is_last_in_tag_history_ ? nullptr : this + 1;
  }

  const std::string& Value() const;

  const std::string& Attribute() const;
  LynxCSSSelectorExtraData::AttributeMatchType AttributeMatch() const;
  const std::string& Argument() const;
  const LynxCSSSelectorList* SelectorList() const {
    return has_extra_data_ ? extra_data_->selector_list_.get() : nullptr;
  }

  uint32_t Specificity() const { return specificity_; }

  void SetValue(const std::string&);
  void SetAttribute(const std::string&, AttributeMatchType);
  void SetArgument(const std::string&);
  void SetSelectorList(std::unique_ptr<LynxCSSSelectorList>);

  void SetNth(int a, int b);
  bool MatchNth(unsigned count) const;

  bool IsAttributeSelector() const {
    return match_ >= kFirstAttributeSelectorMatch;
  }

  static void FromLepus(LynxCSSSelector&, const lepus_value&);
  lepus_value ToLepus() const;

  MatchType Match() const { return static_cast<MatchType>(match_); }
  void SetMatch(MatchType match) { match_ = match; }

  RelationType Relation() const { return static_cast<RelationType>(relation_); }
  void SetRelation(RelationType relation) { relation_ = relation; }

  bool IsLastInTagHistory() const { return is_last_in_tag_history_; }
  void SetLastInTagHistory(bool is_last) { is_last_in_tag_history_ = is_last; }

  bool IsLastInSelectorList() const { return is_last_in_selector_list_; }
  void SetLastInSelectorList(bool is_last) {
    is_last_in_selector_list_ = is_last;
  }

  void SetPseudoType(PseudoType pseudo_type) {
    pseudo_type_ = pseudo_type;
    DCHECK_EQ(static_cast<PseudoType>(pseudo_type_),
              pseudo_type);  // using a bitfield.
  }

  unsigned CalcSpecificityForSimple() const;
  const LynxCSSSelector* SerializeCompound(std::string&) const;

  const LynxCSSSelector* SelectorListSelector() const;

  LynxCSSSelector& operator=(LynxCSSSelector&&) = default;
  LynxCSSSelector& operator=(const LynxCSSSelector&) = delete;

 private:
  void CreateExtraData();

  unsigned relation_ : 4;     // enum RelationType
  unsigned match_ : 4;        // enum MatchType
  unsigned pseudo_type_ : 8;  // enum PseudoType
  unsigned is_last_in_selector_list_ : 1;
  unsigned is_last_in_tag_history_ : 1;
  unsigned has_extra_data_ : 1;
  unsigned tag_is_implicit_ : 1;
  uint32_t specificity_;
  std::string value_;
  std::unique_ptr<LynxCSSSelectorExtraData> extra_data_;
};

inline const std::string& LynxCSSSelector::Attribute() const {
  DCHECK(IsAttributeSelector());
  DCHECK(has_extra_data_);
  return extra_data_->attribute_;
}

inline LynxCSSSelectorExtraData::AttributeMatchType
LynxCSSSelector::AttributeMatch() const {
  DCHECK(IsAttributeSelector());
  DCHECK(has_extra_data_);
  return extra_data_->bits_.attr_.attribute_match_;
}

inline void LynxCSSSelector::SetValue(const std::string& value) {
  if (!has_extra_data_) {
    value_ = value;
    return;
  }
  extra_data_->value_ = value;
}

inline LynxCSSSelector::LynxCSSSelector()
    : relation_(kSubSelector),
      match_(kUnknown),
      pseudo_type_(kPseudoUnknown),
      is_last_in_selector_list_(false),
      is_last_in_tag_history_(true),
      has_extra_data_(false),
      tag_is_implicit_(false),
      specificity_(0),
      value_() {}

inline LynxCSSSelector::LynxCSSSelector(const std::string& tag_name,
                                        bool tag_is_implicit)
    : relation_(kSubSelector),
      match_(kTag),
      pseudo_type_(kPseudoUnknown),
      is_last_in_selector_list_(false),
      is_last_in_tag_history_(true),
      has_extra_data_(false),
      tag_is_implicit_(tag_is_implicit),
      specificity_(0),
      value_(tag_name) {}

inline const std::string& LynxCSSSelector::Value() const {
  if (has_extra_data_) return extra_data_->value_;
  return value_;
}

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_SELECTOR_LYNX_CSS_SELECTOR_H_
