// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/renderer/css/ng/invalidation/rule_invalidation_set.h"

#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/renderer/css/ng/parser/css_parser_token_range.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"
#include "core/renderer/css/ng/selector/css_parser_context.h"
#include "core/renderer/css/ng/selector/css_selector_parser.h"
#include "core/renderer/css/ng/style/style_rule.h"
#include "core/renderer/tasm/testing/mock_attribute_holder.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

using testing::AssertionFailure;
using testing::AssertionResult;
using testing::AssertionSuccess;

namespace lynx {
namespace css {

class RuleInvalidationSetTest : public testing::Test {
 public:
  RuleInvalidationSetTest() = default;

  void SetUp() override {
    document_ = std::make_unique<tasm::MockAttributeHolder>("html");
    auto first = std::make_unique<tasm::MockAttributeHolder>("b");
    auto first_ptr = first.get();
    document_->AddChild(std::move(first));
    auto inner = std::make_unique<tasm::MockAttributeHolder>("i");
    first_ptr->AddChild(std::move(inner));
  }

  void MergeInto(RuleInvalidationSet& rule_feature_set) {
    rule_feature_set.Merge(rule_invalidation_set_);
  }

  RuleInvalidationSet& GetRuleInvalidationSet() {
    return rule_invalidation_set_;
  }

  void AddSelector(const std::string& selector_text) {
    return AddSelector(selector_text, rule_invalidation_set_);
  }

  static void AddSelector(std::unique_ptr<css::LynxCSSSelector[]> selector_arr,
                          RuleInvalidationSet& set) {
    auto style_rule =
        std::make_unique<StyleRule>(std::move(selector_arr), nullptr);
    return AddSelector(std::move(style_rule), set);
  }

  static void AddSelector(std::unique_ptr<StyleRule> style_rule,
                          RuleInvalidationSet& set) {
    for (const LynxCSSSelector* s = style_rule->FirstSelector(); s;
         s = LynxCSSSelectorList::Next(*s)) {
      set.AddSelector(*s);
    }
  }

  static void AddSelector(const std::string& selector_text,
                          RuleInvalidationSet& set) {
    std::vector<LynxCSSSelector> arena;
    CSSParserContext context;
    CSSTokenizer tokenizer(selector_text);
    const auto tokens = tokenizer.TokenizeToEOF();
    CSSParserTokenRange range(tokens);
    LynxCSSSelectorVector selector_vector =
        CSSSelectorParser::ParseSelector(range, &context);
    size_t flattened_size = CSSSelectorParser::FlattenedSize(selector_vector);
    if (!flattened_size) {
      return;
    }

    auto selector_array = std::make_unique<LynxCSSSelector[]>(flattened_size);
    CSSSelectorParser::AdoptSelectorVector(
        selector_vector, selector_array.get(), flattened_size);
    return AddSelector(std::move(selector_array), set);
  }

  void ClearInvalidations() { rule_invalidation_set_.Clear(); }

  void CollectClass(InvalidationLists& invalidation_lists,
                    const std::string& class_name) const {
    rule_invalidation_set_.CollectClass(invalidation_lists, class_name);
  }

  void CollectId(InvalidationLists& invalidation_lists,
                 const std::string& id) const {
    rule_invalidation_set_.CollectId(invalidation_lists, id);
  }

  void CollectPseudoClass(InvalidationLists& invalidation_lists,
                          LynxCSSSelector::PseudoType pseudo) const {
    rule_invalidation_set_.CollectPseudoClass(invalidation_lists, pseudo);
  }

  using BackingType = InvalidationSet::BackingType;

  template <BackingType type>
  std::unordered_set<std::string> ToHashSet(
      typename InvalidationSet::Backing<type>::Range range) {
    std::unordered_set<std::string> hash_set;
    for (auto& str : range) hash_set.insert(str);
    return hash_set;
  }

  std::unordered_set<std::string> ClassSet(
      const InvalidationSet& invalidation_set) {
    return ToHashSet<BackingType::kClasses>(invalidation_set.Classes());
  }

  std::unordered_set<std::string> IdSet(
      const InvalidationSet& invalidation_set) {
    return ToHashSet<BackingType::kIds>(invalidation_set.Ids());
  }

  std::unordered_set<std::string> TagNameSet(
      const InvalidationSet& invalidation_set) {
    return ToHashSet<BackingType::kTagNames>(invalidation_set.TagNames());
  }

  AssertionResult HasNoInvalidation(InvalidationSetVector& invalidation_sets) {
    if (!invalidation_sets.empty()) {
      return AssertionFailure() << "has " << invalidation_sets.size()
                                << " invalidation set(s), should have 0";
    }
    return AssertionSuccess();
  }

  AssertionResult HasSelfInvalidation(
      InvalidationSetVector& invalidation_sets) {
    if (invalidation_sets.size() != 1) {
      return AssertionFailure() << "has " << invalidation_sets.size()
                                << " invalidation set(s), should have 1";
    }
    if (!invalidation_sets[0]->InvalidatesSelf()) {
      return AssertionFailure() << "should invalidate self";
    }
    return AssertionSuccess();
  }

  AssertionResult HasNoSelfInvalidation(
      InvalidationSetVector& invalidation_sets) {
    if (invalidation_sets.size() != 1u) {
      return AssertionFailure() << "has " << invalidation_sets.size()
                                << " invalidation set(s), should have 1";
    }
    if (invalidation_sets[0]->InvalidatesSelf()) {
      return AssertionFailure() << "should not invalidate self";
    }
    return AssertionSuccess();
  }

  AssertionResult HasSelfInvalidationSet(
      InvalidationSetVector& invalidation_sets) {
    if (invalidation_sets.size() != 1u) {
      return AssertionFailure() << "has " << invalidation_sets.size()
                                << " invalidation set(s), should have 1";
    }
    if (!invalidation_sets[0]->IsSelfInvalidationSet()) {
      return AssertionFailure() << "should be the self-invalidation set";
    }
    return AssertionSuccess();
  }

  AssertionResult HasNotSelfInvalidationSet(
      InvalidationSetVector& invalidation_sets) {
    if (invalidation_sets.size() != 1u) {
      return AssertionFailure() << "has " << invalidation_sets.size()
                                << " invalidation set(s), should have 1";
    }
    if (invalidation_sets[0]->IsSelfInvalidationSet()) {
      return AssertionFailure() << "should not be the self-invalidation set";
    }
    return AssertionSuccess();
  }

  AssertionResult HasWholeSubtreeInvalidation(
      InvalidationSetVector& invalidation_sets) {
    if (invalidation_sets.size() != 1u) {
      return AssertionFailure() << "has " << invalidation_sets.size()
                                << " invalidation set(s), should have 1";
    }
    if (!invalidation_sets[0]->WholeSubtreeInvalid()) {
      return AssertionFailure() << "should invalidate whole subtree";
    }
    return AssertionSuccess();
  }

  AssertionResult HasClassInvalidation(
      const std::string& class_name, InvalidationSetVector& invalidation_sets) {
    if (invalidation_sets.size() != 1u) {
      return AssertionFailure() << "has " << invalidation_sets.size()
                                << " invalidation set(s), should have 1";
    }
    std::unordered_set<std::string> classes = ClassSet(*invalidation_sets[0]);
    if (classes.size() != 1u) {
      return AssertionFailure() << classes.size() << " should be 1";
    }
    if (classes.find(class_name) == classes.end()) {
      return AssertionFailure() << "should invalidate class " << class_name;
    }
    return AssertionSuccess();
  }

  AssertionResult HasClassInvalidation(
      const std::string& first_class_name, const std::string& second_class_name,
      InvalidationSetVector& invalidation_sets) {
    if (invalidation_sets.size() != 1u) {
      return AssertionFailure() << "has " << invalidation_sets.size()
                                << " invalidation set(s), should have 1";
    }
    std::unordered_set<std::string> classes = ClassSet(*invalidation_sets[0]);
    if (classes.size() != 2u) {
      return AssertionFailure() << classes.size() << " should be 2";
    }
    if (classes.find(first_class_name) == classes.end()) {
      return AssertionFailure()
             << "should invalidate class " << first_class_name;
    }
    if (classes.find(second_class_name) == classes.end()) {
      return AssertionFailure()
             << "should invalidate class " << second_class_name;
    }
    return AssertionSuccess();
  }

  AssertionResult HasClassInvalidation(
      const std::string& first_class_name, const std::string& second_class_name,
      const std::string& third_class_name,
      InvalidationSetVector& invalidation_sets) {
    if (invalidation_sets.size() != 1u) {
      return AssertionFailure() << "has " << invalidation_sets.size()
                                << " invalidation set(s), should have 1";
    }
    std::unordered_set<std::string> classes = ClassSet(*invalidation_sets[0]);
    if (classes.size() != 3u) {
      return AssertionFailure() << classes.size() << " should be 3";
    }
    if (classes.find(first_class_name) == classes.end()) {
      return AssertionFailure()
             << "should invalidate class " << first_class_name;
    }
    if (classes.find(second_class_name) == classes.end()) {
      return AssertionFailure()
             << "should invalidate class " << second_class_name;
    }
    if (classes.find(third_class_name) == classes.end()) {
      return AssertionFailure()
             << "should invalidate class " << third_class_name;
    }
    return AssertionSuccess();
  }

  AssertionResult HasIdInvalidation(const std::string& id,
                                    InvalidationSetVector& invalidation_sets) {
    if (invalidation_sets.size() != 1u) {
      return AssertionFailure() << "has " << invalidation_sets.size()
                                << " invalidation set(s), should have 1";
    }
    std::unordered_set<std::string> ids = IdSet(*invalidation_sets[0]);
    if (ids.size() != 1u) {
      return AssertionFailure() << ids.size() << " should be 1";
    }
    if (ids.find(id) == ids.end()) {
      return AssertionFailure() << "should invalidate id " << id;
    }
    return AssertionSuccess();
  }

  AssertionResult HasIdInvalidation(const std::string& first_id,
                                    const std::string& second_id,
                                    InvalidationSetVector& invalidation_sets) {
    if (invalidation_sets.size() != 1u) {
      return AssertionFailure() << "has " << invalidation_sets.size()
                                << " invalidation set(s), should have 1";
    }
    std::unordered_set<std::string> ids = IdSet(*invalidation_sets[0]);
    if (ids.size() != 2u) {
      return AssertionFailure() << ids.size() << " should be 2";
    }
    if (ids.find(first_id) == ids.end()) {
      return AssertionFailure() << "should invalidate id " << first_id;
    }
    if (ids.find(second_id) == ids.end()) {
      return AssertionFailure() << "should invalidate id " << second_id;
    }
    return AssertionSuccess();
  }

  AssertionResult HasTagNameInvalidation(
      const std::string& tag_name, InvalidationSetVector& invalidation_sets) {
    if (invalidation_sets.size() != 1u) {
      return AssertionFailure() << "has " << invalidation_sets.size()
                                << " invalidation set(s), should have 1";
    }
    std::unordered_set<std::string> tag_names =
        TagNameSet(*invalidation_sets[0]);
    if (tag_names.size() != 1u) {
      return AssertionFailure() << tag_names.size() << " should be 1";
    }
    if (tag_names.find(tag_name) == tag_names.end()) {
      return AssertionFailure() << "should invalidate tag " << tag_name;
    }
    return AssertionSuccess();
  }

  AssertionResult HasTagNameInvalidation(
      const std::string& first_tag_name, const std::string& second_tag_name,
      InvalidationSetVector& invalidation_sets) {
    if (invalidation_sets.size() != 1u) {
      return AssertionFailure() << "has " << invalidation_sets.size()
                                << " invalidation set(s), should have 1";
    }
    std::unordered_set<std::string> tag_names =
        TagNameSet(*invalidation_sets[0]);
    if (tag_names.size() != 2u) {
      return AssertionFailure() << tag_names.size() << " should be 2";
    }
    if (tag_names.find(first_tag_name) == tag_names.end()) {
      return AssertionFailure() << "should invalidate tag " << first_tag_name;
    }
    if (tag_names.find(second_tag_name) == tag_names.end()) {
      return AssertionFailure() << "should invalidate tag " << second_tag_name;
    }
    return AssertionSuccess();
  }

 private:
  RuleInvalidationSet rule_invalidation_set_;
  std::unique_ptr<tasm::MockAttributeHolder> document_;
};

TEST_F(RuleInvalidationSetTest, interleavedDescendantSibling1) {
  AddSelector(".p");

  InvalidationLists invalidation_lists;
  CollectClass(invalidation_lists, "p");
  EXPECT_TRUE(HasSelfInvalidation(invalidation_lists.descendants));
}

TEST_F(RuleInvalidationSetTest, id) {
  AddSelector("#a #b");

  InvalidationLists invalidation_lists;
  CollectId(invalidation_lists, "a");
  EXPECT_TRUE(HasIdInvalidation("b", invalidation_lists.descendants));
}

TEST_F(RuleInvalidationSetTest, pseudoClass) {
  AddSelector(":focus");

  InvalidationLists invalidation_lists;
  CollectPseudoClass(invalidation_lists, LynxCSSSelector::kPseudoFocus);
  EXPECT_TRUE(HasSelfInvalidation(invalidation_lists.descendants));
}

TEST_F(RuleInvalidationSetTest, tagName) {
  AddSelector(":focus e");

  InvalidationLists invalidation_lists;
  CollectPseudoClass(invalidation_lists, LynxCSSSelector::kPseudoFocus);
  EXPECT_TRUE(HasTagNameInvalidation("e", invalidation_lists.descendants));
}

TEST_F(RuleInvalidationSetTest, Whole) {
  AddSelector(".a *");

  InvalidationLists invalidation_lists;
  CollectClass(invalidation_lists, "a");
  EXPECT_TRUE(HasWholeSubtreeInvalidation(invalidation_lists.descendants));
}

TEST_F(RuleInvalidationSetTest, SelfInvalidationSet) {
  AddSelector(".a");
  AddSelector("div .b");
  AddSelector("#c");
  AddSelector("[d]");
  AddSelector(":hover");

  InvalidationLists invalidation_lists;
  CollectClass(invalidation_lists, "a");
  EXPECT_TRUE(HasSelfInvalidation(invalidation_lists.descendants));
  EXPECT_TRUE(HasSelfInvalidationSet(invalidation_lists.descendants));

  invalidation_lists.descendants.clear();
  CollectClass(invalidation_lists, "b");
  EXPECT_TRUE(HasSelfInvalidation(invalidation_lists.descendants));
  EXPECT_TRUE(HasSelfInvalidationSet(invalidation_lists.descendants));

  invalidation_lists.descendants.clear();
  CollectId(invalidation_lists, "c");
  EXPECT_TRUE(HasSelfInvalidation(invalidation_lists.descendants));
  EXPECT_TRUE(HasSelfInvalidationSet(invalidation_lists.descendants));

  invalidation_lists.descendants.clear();
  CollectPseudoClass(invalidation_lists, LynxCSSSelector::kPseudoHover);
  EXPECT_TRUE(HasSelfInvalidation(invalidation_lists.descendants));
  EXPECT_TRUE(HasSelfInvalidationSet(invalidation_lists.descendants));
}

TEST_F(RuleInvalidationSetTest, ReplaceSelfInvalidationSet) {
  AddSelector(".a");

  InvalidationLists invalidation_lists;
  CollectClass(invalidation_lists, "a");
  EXPECT_TRUE(HasSelfInvalidation(invalidation_lists.descendants));
  EXPECT_TRUE(HasSelfInvalidationSet(invalidation_lists.descendants));

  AddSelector(".a div");

  invalidation_lists.descendants.clear();
  CollectClass(invalidation_lists, "a");
  EXPECT_TRUE(HasSelfInvalidation(invalidation_lists.descendants));
  EXPECT_TRUE(HasNotSelfInvalidationSet(invalidation_lists.descendants));
}

TEST_F(RuleInvalidationSetTest, Not) {
  AddSelector(".b:not(.a) .c");
  auto& s = GetRuleInvalidationSet();
  InvalidationLists lists;
  s.CollectClass(lists, "b");
  EXPECT_TRUE(HasClassInvalidation("c", lists.descendants));

  lists = InvalidationLists();
  s.CollectClass(lists, "d");
  EXPECT_TRUE(HasNoInvalidation(lists.descendants));
}

TEST_F(RuleInvalidationSetTest, EnsureMutableInvalidationSet2) {
  AddSelector(".a div");
  AddSelector("div");
  auto& s = GetRuleInvalidationSet();
  InvalidationLists lists;
  s.CollectClass(lists, "a");
  EXPECT_TRUE(HasTagNameInvalidation("div", lists.descendants));
}

TEST_F(RuleInvalidationSetTest, EnsureMutableInvalidationSet3) {
  AddSelector(".a ~ *");
  AddSelector(".a div");
  auto& s = GetRuleInvalidationSet();
  InvalidationLists lists;
  s.CollectClass(lists, "a");
  EXPECT_FALSE(HasWholeSubtreeInvalidation(lists.descendants));
}

TEST_F(RuleInvalidationSetTest, Merge) {
  RuleInvalidationSet local;
  AddSelector(".a div");
  AddSelector(":hover *");
  MergeInto(local);
  ClearInvalidations();
  InvalidationLists lists;
  local.CollectClass(lists, "a");
  EXPECT_FALSE(HasWholeSubtreeInvalidation(lists.descendants));

  InvalidationLists lists_pseudo;
  local.CollectPseudoClass(lists_pseudo,
                           LynxCSSSelector::PseudoType::kPseudoHover);
  EXPECT_TRUE(HasWholeSubtreeInvalidation(lists_pseudo.descendants));
}

TEST_F(RuleInvalidationSetTest, IgnoreSibling) {
  AddSelector(".a ~ div");
  AddSelector(".a ~ div .b");
  AddSelector(".a ~ div *");
  InvalidationLists lists;
  CollectClass(lists, "a");
  EXPECT_TRUE(HasNoInvalidation(lists.descendants));
}

}  // namespace css
}  // namespace lynx
