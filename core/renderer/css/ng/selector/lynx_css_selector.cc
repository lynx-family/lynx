// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/selector/lynx_css_selector.h"

#include <limits>
#include <memory>
#include <utility>

#include "core/renderer/css/ng/css_ng_utils.h"
#include "core/renderer/css/ng/selector/lynx_css_selector_list.h"
#include "core/runtime/vm/lepus/array.h"

namespace lynx {
namespace css {

void LynxCSSSelector::FromLepus(LynxCSSSelector& s, const lepus_value& value) {
  if (!value.IsArray()) {
    return;
  }
  const auto& arr = value.Array();
  uint32_t bit = arr->get(0).UInt32();
  s.relation_ = bit & 0xf;
  s.match_ = bit >> 4 & 0xf;
  s.pseudo_type_ = bit >> 8 & 0xff;
  s.is_last_in_selector_list_ = bit >> 16 & 1;
  s.is_last_in_tag_history_ = bit >> 17 & 1;
  s.has_extra_data_ = bit >> 18 & 1;
  s.tag_is_implicit_ = bit >> 19 & 1;

  s.specificity_ = arr->get(1).UInt32();
  if (s.has_extra_data_) {
    const auto& extra_arr = arr->get(2).Array();
    const auto& v = extra_arr->get(0).StdString();
    s.extra_data_ = std::make_unique<LynxCSSSelectorExtraData>(v);
    s.extra_data_->match_type_ =
        static_cast<LynxCSSSelectorExtraData::MatchType>(
            extra_arr->get(1).UInt32());
    const auto& bits = extra_arr->get(2).Array();
    if (s.extra_data_->match_type_ ==
        LynxCSSSelectorExtraData::MatchType::kNth) {
      s.extra_data_->bits_.nth_.a_ = bits->get(0).Int32();
      s.extra_data_->bits_.nth_.b_ = bits->get(1).Int32();
    } else if (s.extra_data_->match_type_ ==
               LynxCSSSelectorExtraData::MatchType::kAttr) {
      s.extra_data_->bits_.attr_.attribute_match_ =
          static_cast<AttributeMatchType>(bits->get(0).UInt32());
      s.extra_data_->bits_.attr_.is_case_sensitive_attribute_ =
          bits->get(1).Bool();
    }

    s.extra_data_->attribute_ = extra_arr->get(3).StdString();
    s.extra_data_->argument_ = extra_arr->get(4).StdString();
    bool has_selector_list = extra_arr->get(5).IsArray();
    if (has_selector_list) {
      const auto& selector_list_arr = extra_arr->get(5).Array();
      auto selector_array =
          selector_list_arr->size() == 0
              ? nullptr
              : std::make_unique<LynxCSSSelector[]>(selector_list_arr->size());
      for (size_t i = 0; i < selector_list_arr->size(); ++i) {
        FromLepus(selector_array[i], selector_list_arr->get(i));
      }
      s.extra_data_->selector_list_ =
          std::make_unique<LynxCSSSelectorList>(std::move(selector_array));
    }
  } else {
    s.value_ = arr->get(2).StdString();
  }
}

lepus_value LynxCSSSelector::ToLepus() const {
  auto arr = lepus::CArray::Create();

  uint32_t bit = 0;
  bit |= relation_;
  bit |= match_ << 4;
  bit |= pseudo_type_ << 8;
  bit |= is_last_in_selector_list_ << 16;
  bit |= is_last_in_tag_history_ << 17;
  bit |= has_extra_data_ << 18;
  bit |= tag_is_implicit_ << 19;

  arr->emplace_back(bit);
  arr->emplace_back(specificity_);
  if (has_extra_data_) {
    auto extra_arr = lepus::CArray::Create();
    extra_arr->emplace_back(extra_data_->value_);
    extra_arr->emplace_back(static_cast<uint32_t>(extra_data_->match_type_));

    {
      auto bits = lepus::CArray::Create();
      if (extra_data_->match_type_ ==
          LynxCSSSelectorExtraData::MatchType::kNth) {
        bits->emplace_back(extra_data_->NthAValue());
        bits->emplace_back(extra_data_->NthBValue());
      } else if (extra_data_->match_type_ ==
                 LynxCSSSelectorExtraData::MatchType::kAttr) {
        bits->emplace_back(
            static_cast<uint32_t>(extra_data_->bits_.attr_.attribute_match_));
        bits->emplace_back(
            extra_data_->bits_.attr_.is_case_sensitive_attribute_);
      } else if (extra_data_->match_type_ ==
                 LynxCSSSelectorExtraData::MatchType::kHas) {
        bits->emplace_back(extra_data_->bits_.has_.contains_pseudo_);
        bits->emplace_back(
            extra_data_->bits_.has_.contains_complex_logical_combinations_);
      }
      extra_arr->emplace_back(std::move(bits));
    }

    extra_arr->emplace_back(extra_data_->attribute_);
    extra_arr->emplace_back(extra_data_->argument_);
    if (SelectorList()) {
      auto selector_list_arr = lepus::CArray::Create();
      auto current = SelectorList()->First();
      while (current) {
        selector_list_arr->emplace_back(current->ToLepus());
        if (current->IsLastInTagHistory() && current->IsLastInSelectorList()) {
          break;
        }
        current++;
      }
      extra_arr->emplace_back(std::move(selector_list_arr));
    } else {
      extra_arr->emplace_back(false);
    }

    arr->emplace_back(std::move(extra_arr));
  } else {
    arr->emplace_back(value_);
  }

  return lepus_value(std::move(arr));
}

void LynxCSSSelector::CreateExtraData() {
  if (has_extra_data_) return;
  extra_data_ = std::make_unique<LynxCSSSelectorExtraData>(value_);
  value_.clear();
  has_extra_data_ = true;
}

void LynxCSSSelector::SetAttribute(const std::string& value,
                                   AttributeMatchType match_type) {
  CreateExtraData();
  extra_data_->attribute_ = value;
  extra_data_->match_type_ = LynxCSSSelectorExtraData::MatchType::kAttr;
  extra_data_->bits_.attr_.attribute_match_ = match_type;
  extra_data_->bits_.attr_.is_case_sensitive_attribute_ = true;
}

void LynxCSSSelector::SetArgument(const std::string& value) {
  CreateExtraData();
  extra_data_->argument_ = value;
}

void LynxCSSSelector::SetSelectorList(
    std::unique_ptr<LynxCSSSelectorList> selector_list) {
  CreateExtraData();
  extra_data_->selector_list_ = std::move(selector_list);
}

void LynxCSSSelector::SetNth(int a, int b) {
  CreateExtraData();
  extra_data_->match_type_ = LynxCSSSelectorExtraData::MatchType::kNth;
  extra_data_->bits_.nth_.a_ = a;
  extra_data_->bits_.nth_.b_ = b;
}

bool LynxCSSSelector::MatchNth(unsigned count) const {
  DCHECK(has_extra_data_);
  return extra_data_->MatchNth(count);
}

const std::string& LynxCSSSelector::Argument() const {
  return has_extra_data_ ? extra_data_->argument_ : CSSGlobalEmptyString();
}

const LynxCSSSelector* LynxCSSSelector::SelectorListSelector() const {
  if (has_extra_data_ && extra_data_->selector_list_) {
    return extra_data_->selector_list_->First();
  } else {
    return nullptr;
  }
}

const LynxCSSSelector* LynxCSSSelector::SerializeCompound(
    std::string& result) const {
  if (Match() == kTag && !tag_is_implicit_) {
    result += Value();
  }

  for (const LynxCSSSelector* selector = this; selector;
       selector = selector->TagHistory()) {
    if (selector->Match() == kId) {
      result.push_back('#');
      result += selector->Value();
    } else if (selector->Match() == kClass) {
      result.push_back('.');
      result += selector->Value();
    } else if (selector->Match() == kPseudoClass) {
      result.push_back(':');
      result += selector->Value();
    } else if (selector->Match() == kPseudoElement) {
      result += "::";
      result += selector->Value();
    } else if (selector->IsAttributeSelector()) {
      // Attribute selectors are not supported
    }

    if (selector->SelectorList()) {
      result.push_back('(');
      const LynxCSSSelector* first_selector = selector->SelectorList()->First();
      for (const LynxCSSSelector* sub_selector = first_selector; sub_selector;
           sub_selector = LynxCSSSelectorList::Next(*sub_selector)) {
        if (sub_selector != first_selector) result += ", ";
        result += sub_selector->ToString();
      }
      result.push_back(')');
    }

    if (selector->Relation() != kSubSelector) return selector;
  }
  return nullptr;
}

std::string LynxCSSSelector::ToString() const {
  std::string result;
  for (const LynxCSSSelector* compound = this; compound;
       compound = compound->TagHistory()) {
    std::string compound_result;
    compound = compound->SerializeCompound(compound_result);
    if (!compound) return compound_result + result;

    DCHECK(compound->Relation() != kSubSelector);
    switch (compound->Relation()) {
      case kDescendant:
        result = " " + compound_result + result;
        break;
      case kChild:
        result = " > " + compound_result + result;
        break;
      case kDirectAdjacent:
        result = " + " + compound_result + result;
        break;
      case kIndirectAdjacent:
        result = " ~ " + compound_result + result;
        break;
      case kSubSelector:
        NOTREACHED();
        break;
      case kUAShadow:
        result = compound_result + result;
        break;
      default:
        break;
    }
  }
  return result;
}

}  // namespace css
}  // namespace lynx
