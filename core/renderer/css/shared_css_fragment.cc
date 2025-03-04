// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/shared_css_fragment.h"

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/css/css_style_sheet_manager.h"

namespace lynx {
namespace tasm {

SharedCSSFragment::~SharedCSSFragment() = default;

SharedCSSFragment::SharedCSSFragment(int32_t id,
                                     const std::vector<int32_t>& dependent_ids,
                                     CSSParserTokenMap css,
                                     CSSKeyframesTokenMap keyframes,
                                     CSSFontFaceRuleMap fontfaces,
                                     CSSStyleSheetManager* manager)
    : CSSFragment(std::move(keyframes), std::move(fontfaces)),
      id_(id),
      is_baked_(false),
      dependent_ids_(dependent_ids),
      css_(std::move(css)),
      manager_(manager) {
  if (manager_) {
    enable_css_lazy_import_ = manager_->GetEnableCSSLazyImport();
  }
}

bool SharedCSSFragment::HasCSSStyle() {
  if (has_css_style_.has_value()) {
    return has_css_style_.value_or(false);
  }
  if (!css_.empty()) {
    has_css_style_ = true;
    return true;
  }
  if (enable_css_lazy_import_) {
    auto dependent_fragment_ids = dependent_ids();
    for (auto id = dependent_fragment_ids.rbegin();
         id != dependent_fragment_ids.rend(); ++id) {
      auto dependent_fragment = manager_->GetCSSStyleSheet(*id);
      if (dependent_fragment->HasCSSStyle()) {
        has_css_style_ = true;
        return true;
      }
    }
    has_css_style_ = false;
    return false;
  } else {
    has_css_style_ = !css_.empty();
    return !css_.empty();
  }
}

CSSParseToken* SharedCSSFragment::GetCSSStyle(const std::string& key) {
  auto it = css_.find(key);
  if (it != css_.end()) {
    return it->second.get();
  }
  if (enable_css_lazy_import_) {
    auto dependent_fragment_ids = dependent_ids();
    for (auto id = dependent_fragment_ids.rbegin();
         id != dependent_fragment_ids.rend(); ++id) {
      auto dependent_fragment = manager_->GetCSSStyleSheet(*id);
      auto result = dependent_fragment->GetCSSStyle(key);
      if (result != nullptr) {
        return result;
      }
    }
  }
  return nullptr;
}

std::shared_ptr<CSSParseToken> SharedCSSFragment::GetSharedCSSStyle(
    const std::string& key) {
  auto it = css_.find(key);
  if (it != css_.end()) {
    return it->second;
  }
  if (enable_css_lazy_import_) {
    auto dependent_fragment_ids = dependent_ids();
    for (auto id = dependent_fragment_ids.rbegin();
         id != dependent_fragment_ids.rend(); ++id) {
      auto dependent_fragment = manager_->GetCSSStyleSheet(*id);
      auto result = dependent_fragment->GetSharedCSSStyle(key);
      if (result != nullptr) {
        return result;
      }
    }
  }
  return nullptr;
}

#define SHARED_CSS_FRAGMENT_GET_STYLE(field, name)                             \
  CSSParseToken* SharedCSSFragment::Get##name##Style(const std::string& key) { \
    auto it = field.find(key);                                                 \
    if (it != field.end()) {                                                   \
      return it->second.get();                                                 \
    }                                                                          \
    return nullptr;                                                            \
  }

SHARED_CSS_FRAGMENT_GET_STYLE(pseudo_map_, Pseudo)
SHARED_CSS_FRAGMENT_GET_STYLE(cascade_map_, Cascade)
SHARED_CSS_FRAGMENT_GET_STYLE(id_map_, Id)
SHARED_CSS_FRAGMENT_GET_STYLE(tag_map_, Tag)
SHARED_CSS_FRAGMENT_GET_STYLE(universal_map_, Universal)
#undef SHARED_CSS_FRAGMENT_GET_STYLE

void SharedCSSFragment::ImportOtherFragment(const SharedCSSFragment* fragment) {
  if (fragment == nullptr) return;
  if (fragment->HasTouchPseudoToken()) {
    // When ImportOtherFragment, if the previous fragment contains a touch
    // pseudo, mark the current fragment also has a touch pseudo. So that the
    // platform layer can judge whether to execute the pseudo related functions
    // according to whether it has a touch state pseudo-class
    MarkHasTouchPseudoToken();
  }
  css_.reserve(fragment->css_.size());
  for (auto& css : fragment->css_) {
    if (!enable_class_merge_) {
      if (!enable_css_lazy_import_) {
        css_[css.first] = css.second;
      }
      continue;
    }
    const auto& selector = css.first;
    if (css_.find(selector) != css_.end()) {
      auto& depend_attribute = css.second->GetAttributes();
      StyleMap cur_attribute = css_[selector]->GetAttributes();
      for (auto& it : depend_attribute) {
        if (cur_attribute.find(it.first) == cur_attribute.end()) {
          cur_attribute[it.first] = std::move(it.second);
        }
      }
      css_[selector]->SetAttributes(std::move(cur_attribute));
    } else {
      css_[css.first] = css.second;
    }
  }
  for (auto& pseudo : fragment->pseudo_map_) {
    pseudo_map_[pseudo.first] = pseudo.second;
  }
  for (auto& child_pseudo : fragment->child_pseudo_map_) {
    child_pseudo_map_[child_pseudo.first] = child_pseudo.second;
  }
  for (auto& cascade : fragment->cascade_map_) {
    cascade_map_[cascade.first] = cascade.second;
  }
  for (auto& id : fragment->id_map_) {
    id_map_[id.first] = id.second;
  }
  for (auto& tag : fragment->tag_map_) {
    tag_map_[tag.first] = tag.second;
  }
  for (auto& universal : fragment->universal_map_) {
    universal_map_[universal.first] = universal.second;
  }
  for (auto& frame : fragment->keyframes_) {
    keyframes_[frame.first] = frame.second;
  }
  for (auto& face : fragment->fontfaces_) {
    fontfaces_[face.first] = face.second;
  }

  if (rule_set_ && fragment->rule_set_) {
    rule_set_->Merge(*fragment->rule_set_);
    if (rule_invalidation_set_ && fragment->rule_invalidation_set_)
      rule_invalidation_set_->Merge(*fragment->rule_invalidation_set_);
  }
}

void SharedCSSFragment::InitPseudoNotStyle() {
  if (pseudo_map_.empty()) {
    return;
  }
  if (pseudo_not_style_) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SharedCSSFragment::InitPseudoNotStyle");
  pseudo_not_style_ = PseudoNotStyle();
  PseudoClassStyleMap global_pseudo_not_tag, global_pseudo_not_class,
      global_pseudo_not_id;

  for (auto& it : pseudo_map_) {
    const std::string& key_name = it.first;

    // mark if has pseudo style
    if (!it.second || !it.second->IsPseudoStyleToken()) {
      // if :not is not used, do not run the code bellow
      continue;
    }

    size_t pseudo_not_loc = key_name.find(":not(");
    if (pseudo_not_loc != std::string::npos) {
      has_pseudo_not_style_ = true;
      size_t loc = key_name.find_first_of("(");
      std::string scope_for_pseudo_not =
          key_name.substr(loc + 1, key_name.size() - loc - 2);
      std::string selector_key = key_name.substr(0, pseudo_not_loc);

      PseudoNotContent content;
      content.selector_key = selector_key;
      content.scope = scope_for_pseudo_not;
      std::string selector_key_type = selector_key.substr(0, 1);
      std::string scope_value_type = scope_for_pseudo_not.substr(0, 1);
      bool is_global_pseudo_not_css = selector_key.compare("") == 0;
      if (selector_key.compare(scope_for_pseudo_not) == 0) {
        continue;
      }

      if (scope_value_type.compare(".") == 0) {
        content.scope_type = CSSSheet::CLASS_SELECT;
        if (is_global_pseudo_not_css) {
          global_pseudo_not_class.insert({key_name, content});
        }
      } else if (scope_value_type.compare("#") == 0) {
        content.scope_type = CSSSheet::ID_SELECT;
        if (is_global_pseudo_not_css) {
          global_pseudo_not_id.insert({key_name, content});
        }
      } else {
        content.scope_type = CSSSheet::NAME_SELECT;
        if (is_global_pseudo_not_css) {
          global_pseudo_not_tag.insert({key_name, content});
        }
      }

      if (is_global_pseudo_not_css) {
        pseudo_not_style_->pseudo_not_global_map.insert(
            {CSSSheet::NAME_SELECT, global_pseudo_not_tag});
        pseudo_not_style_->pseudo_not_global_map.insert(
            {CSSSheet::CLASS_SELECT, global_pseudo_not_class});
        pseudo_not_style_->pseudo_not_global_map.insert(
            {CSSSheet::ID_SELECT, global_pseudo_not_id});
      } else if (selector_key_type.compare(".") == 0) {
        pseudo_not_style_->pseudo_not_for_class.insert({key_name, content});
      } else if (selector_key_type.compare("#") == 0) {
        pseudo_not_style_->pseudo_not_for_id.insert({key_name, content});
      } else {
        pseudo_not_style_->pseudo_not_for_tag.insert({key_name, content});
      }
    }
  }
}

void SharedCSSFragment::FindSpecificMapAndAdd(
    const std::string& key, const std::shared_ptr<CSSParseToken>& parse_token) {
  if (parse_token->IsCascadeSelectorStyleToken()) {
    cascade_map_.emplace(key, parse_token);
  }
  int type = parse_token->GetStyleTokenType();
  if (type > CSSSheet::NAME_SELECT && type != CSSSheet::ALL_SELECT) {
    pseudo_map_.emplace(key, parse_token);
    if ((type & CSSSheet::FIRST_CHILD_SELECT) ||
        (type & CSSSheet::LAST_CHILD_SELECT)) {
      child_pseudo_map_.emplace(key, parse_token);
    }
  } else if (type == CSSSheet::ID_SELECT) {
    id_map_.emplace(key, parse_token);
  } else if (type == CSSSheet::NAME_SELECT) {
    tag_map_.emplace(key, parse_token);
  } else if (type == CSSSheet::ALL_SELECT) {
    universal_map_.emplace(key, parse_token);
  }
}

void SharedCSSFragment::AddStyleRule(
    std::unique_ptr<css::LynxCSSSelector[]> selector_arr,
    std::shared_ptr<CSSParseToken> parse_token) {
  // We know the pointer is not empty
  rule_set_->AddStyleRule(std::make_shared<css::StyleRule>(
      std::move(selector_arr), std::move(parse_token)));
}

void SharedCSSFragment::CollectInvalidationSetsForId(
    css::InvalidationLists& lists, const std::string& id) {
  if (rule_invalidation_set_) {
    rule_invalidation_set_->CollectId(lists, id);
  }
}

void SharedCSSFragment::CollectInvalidationSetsForClass(
    css::InvalidationLists& lists, const std::string& class_name) {
  if (rule_invalidation_set_) {
    rule_invalidation_set_->CollectClass(lists, class_name);
  }
}

void SharedCSSFragment::CollectInvalidationSetsForPseudoClass(
    css::InvalidationLists& lists, css::LynxCSSSelector::PseudoType pseudo) {
  if (rule_invalidation_set_) {
    rule_invalidation_set_->CollectPseudoClass(lists, pseudo);
  }
}

}  // namespace tasm
}  // namespace lynx
