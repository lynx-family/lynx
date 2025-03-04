// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/renderer/css/ng/invalidation/invalidation_set.h"

#include <vector>

#include "core/renderer/tasm/testing/mock_attribute_holder.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {

namespace css {
namespace {

using BackingType = InvalidationSet::BackingType;
using BackingFlags = InvalidationSet::BackingFlags;
template <BackingType type>
using Backing = InvalidationSet::Backing<type>;

template <BackingType type>
bool HasAny(const Backing<type>& backing, const BackingFlags& flags,
            std::initializer_list<const char*> args) {
  for (const char* str : args) {
    if (backing.Contains(flags, str)) return true;
  }
  return false;
}

template <BackingType type>
bool HasAll(const Backing<type>& backing, const BackingFlags& flags,
            std::initializer_list<const char*> args) {
  for (const char* str : args) {
    if (!backing.Contains(flags, str)) return false;
  }
  return true;
}

TEST(InvalidationSetTest, Backing_Create) {
  BackingFlags flags;
  Backing<BackingType::kClasses> backing;

  ASSERT_FALSE(backing.IsHashSet(flags));
}

TEST(InvalidationSetTest, Backing_Add) {
  BackingFlags flags;
  Backing<BackingType::kClasses> backing;

  ASSERT_FALSE(backing.IsHashSet(flags));
  backing.Add(flags, std::string("test1"));
  ASSERT_FALSE(backing.IsHashSet(flags));
  backing.Add(flags, std::string("test2"));
  ASSERT_TRUE(backing.IsHashSet(flags));
  backing.Clear(flags);
}

TEST(InvalidationSetTest, Backing_AddSame) {
  BackingFlags flags;
  Backing<BackingType::kClasses> backing;

  ASSERT_FALSE(backing.IsHashSet(flags));
  backing.Add(flags, std::string("test1"));
  ASSERT_FALSE(backing.IsHashSet(flags));
  backing.Add(flags, std::string("test1"));
  // No need to upgrade to HashSet if we're adding the item we already have.
  ASSERT_FALSE(backing.IsHashSet(flags));
  backing.Clear(flags);
}

TEST(InvalidationSetTest, Backing_Independence) {
  BackingFlags flags;

  Backing<BackingType::kClasses> classes;
  Backing<BackingType::kIds> ids;
  Backing<BackingType::kTagNames> tag_names;
  Backing<BackingType::kAttributes> attributes;

  classes.Add(flags, "test1");
  ids.Add(flags, "test2");
  tag_names.Add(flags, "test3");
  attributes.Add(flags, "test4");

  // Adding to set does not affect other backings:
  ASSERT_TRUE(classes.Contains(flags, "test1"));
  ASSERT_FALSE(HasAny(classes, flags, {"test2", "test3", "test4"}));

  ASSERT_TRUE(ids.Contains(flags, "test2"));
  ASSERT_FALSE(HasAny(ids, flags, {"test1", "test3", "test4"}));

  ASSERT_TRUE(tag_names.Contains(flags, "test3"));
  ASSERT_FALSE(HasAny(tag_names, flags, {"test1", "test2", "test4"}));

  ASSERT_TRUE(attributes.Contains(flags, "test4"));
  ASSERT_FALSE(HasAny(attributes, flags, {"test1", "test2", "test3"}));

  // Adding additional items to one set does not affect others:
  classes.Add(flags, "test5");
  tag_names.Add(flags, "test6");

  ASSERT_TRUE(HasAll(classes, flags, {"test1", "test5"}));
  ASSERT_FALSE(HasAny(classes, flags, {"test2", "test3", "test4", "test6"}));

  ASSERT_TRUE(ids.Contains(flags, "test2"));
  ASSERT_FALSE(
      HasAny(ids, flags, {"test1", "test3", "test4", "test5", "test6"}));

  ASSERT_TRUE(HasAll(tag_names, flags, {"test3", "test6"}));
  ASSERT_FALSE(HasAny(tag_names, flags, {"test1", "test2", "test4", "test5"}));

  ASSERT_TRUE(attributes.Contains(flags, "test4"));
  ASSERT_FALSE(HasAny(attributes, flags, {"test1", "test2", "test3"}));

  // Clearing one set does not clear others:

  classes.Clear(flags);
  ids.Clear(flags);
  attributes.Clear(flags);

  auto all_test_strings = {"test1", "test2", "test3",
                           "test4", "test5", "test6"};

  ASSERT_FALSE(HasAny(classes, flags, all_test_strings));
  ASSERT_FALSE(HasAny(ids, flags, all_test_strings));
  ASSERT_FALSE(HasAny(attributes, flags, all_test_strings));

  ASSERT_FALSE(classes.IsHashSet(flags));
  ASSERT_FALSE(ids.IsHashSet(flags));
  ASSERT_FALSE(attributes.IsHashSet(flags));

  ASSERT_TRUE(tag_names.IsHashSet(flags));
  ASSERT_TRUE(HasAll(tag_names, flags, {"test3", "test6"}));
  ASSERT_FALSE(HasAny(tag_names, flags, {"test1", "test2", "test4", "test5"}));
  tag_names.Clear(flags);
}

TEST(InvalidationSetTest, Backing_ClearContains) {
  BackingFlags flags;
  Backing<BackingType::kClasses> backing;

  // Clearing an empty set:
  ASSERT_FALSE(backing.Contains(flags, "test1"));
  ASSERT_FALSE(backing.IsHashSet(flags));
  backing.Clear(flags);
  ASSERT_FALSE(backing.IsHashSet(flags));

  // Add one element to the set, and clear it:
  backing.Add(flags, "test1");
  ASSERT_FALSE(backing.IsHashSet(flags));
  ASSERT_TRUE(backing.Contains(flags, "test1"));
  backing.Clear(flags);
  ASSERT_FALSE(backing.Contains(flags, "test1"));
  ASSERT_FALSE(backing.IsHashSet(flags));

  // Add two elements to the set, and clear them:
  backing.Add(flags, "test1");
  ASSERT_FALSE(backing.IsHashSet(flags));
  ASSERT_TRUE(backing.Contains(flags, "test1"));
  ASSERT_FALSE(backing.Contains(flags, "test2"));
  backing.Add(flags, "test2");
  ASSERT_TRUE(backing.IsHashSet(flags));
  ASSERT_TRUE(backing.Contains(flags, "test1"));
  ASSERT_TRUE(backing.Contains(flags, "test2"));
  backing.Clear(flags);
  ASSERT_FALSE(backing.Contains(flags, "test1"));
  ASSERT_FALSE(backing.Contains(flags, "test2"));
  ASSERT_FALSE(backing.IsHashSet(flags));
}

TEST(InvalidationSetTest, Backing_BackingIsEmpty) {
  BackingFlags flags;
  Backing<BackingType::kClasses> backing;

  ASSERT_TRUE(backing.IsEmpty(flags));
  backing.Add(flags, "test1");
  ASSERT_FALSE(backing.IsEmpty(flags));
  backing.Add(flags, "test2");
  backing.Clear(flags);
  ASSERT_TRUE(backing.IsEmpty(flags));
}

TEST(InvalidationSetTest, Backing_IsEmpty) {
  BackingFlags flags;
  Backing<BackingType::kClasses> backing;

  ASSERT_TRUE(backing.IsEmpty(flags));

  backing.Add(flags, "test1");
  ASSERT_FALSE(backing.IsEmpty(flags));

  backing.Clear(flags);
  ASSERT_TRUE(backing.IsEmpty(flags));
}

TEST(InvalidationSetTest, Backing_Iterator) {
  // Iterate over empty set.
  {
    BackingFlags flags;
    Backing<BackingType::kClasses> backing;

    std::vector<std::string> strings;
    for (const std::string& str : backing.Items(flags)) strings.push_back(str);
    ASSERT_EQ(0u, strings.size());
  }

  // Iterate over set with one item.
  {
    BackingFlags flags;
    Backing<BackingType::kClasses> backing;

    backing.Add(flags, "test1");
    std::vector<std::string> strings;
    for (const std::string& str : backing.Items(flags)) strings.push_back(str);
    ASSERT_EQ(1u, strings.size());
    ASSERT_TRUE(std::find(strings.begin(), strings.end(), "test1") !=
                strings.end());
    backing.Clear(flags);
  }

  // Iterate over set with multiple items.
  {
    BackingFlags flags;
    Backing<BackingType::kClasses> backing;

    backing.Add(flags, "test1");
    backing.Add(flags, "test2");
    backing.Add(flags, "test3");
    std::vector<std::string> strings;
    for (const std::string& str : backing.Items(flags)) strings.push_back(str);
    ASSERT_EQ(3u, strings.size());
    ASSERT_TRUE(std::find(strings.begin(), strings.end(), "test1") !=
                strings.end());
    ASSERT_TRUE(std::find(strings.begin(), strings.end(), "test2") !=
                strings.end());
    ASSERT_TRUE(std::find(strings.begin(), strings.end(), "test3") !=
                strings.end());
    backing.Clear(flags);
  }
}

TEST(InvalidationSetTest, Backing_GetStringImpl) {
  BackingFlags flags;
  Backing<BackingType::kClasses> backing;
  EXPECT_FALSE(backing.GetStringImpl(flags));
  backing.Add(flags, "a");
  EXPECT_EQ("a", *backing.GetStringImpl(flags));
  backing.Add(flags, "b");
  EXPECT_FALSE(backing.GetStringImpl(flags));
  backing.Clear(flags);
}

TEST(InvalidationSetTest, Backing_GetHashSet) {
  BackingFlags flags;
  Backing<BackingType::kClasses> backing;
  EXPECT_FALSE(backing.GetHashSet(flags));
  backing.Add(flags, "a");
  EXPECT_FALSE(backing.GetHashSet(flags));
  backing.Add(flags, "b");
  EXPECT_TRUE(backing.GetHashSet(flags));
  backing.Clear(flags);
}

TEST(InvalidationSetTest, ClassInvalidatesElement) {
  auto set = DescendantInvalidationSet::Create();

  auto element = std::make_unique<tasm::MockAttributeHolder>("div");
  element->SetIdSelector("test");
  element->AddClass("a");
  element->AddClass("b");

  EXPECT_FALSE(set->InvalidatesElement(*element));
  // Adding one string sets the string_impl_ of the classes_ Backing.
  set->AddClass("a");
  EXPECT_TRUE(set->InvalidatesElement(*element));
  // Adding another upgrades to a HashSet.
  set->AddClass("c");
  EXPECT_TRUE(set->InvalidatesElement(*element));

  // These sets should not cause invalidation.
  set = DescendantInvalidationSet::Create();
  set->AddClass("c");
  EXPECT_FALSE(set->InvalidatesElement(*element));
  set->AddClass("d");
  EXPECT_FALSE(set->InvalidatesElement(*element));
}

// TEST(InvalidationSetTest, AttributeInvalidatesElement) {
//   auto element = std::make_unique<tasm::MockAttributeHolder>("div");
//   element->SetIdSelector("test");
//   element->SetStaticAttribute("a", lepus_value());
//   element->SetStaticAttribute("b", lepus_value());
//
//   ASSERT_TRUE(element);
//
//   auto set = DescendantInvalidationSet::Create();
//   EXPECT_FALSE(set->InvalidatesElement(*element));
//   // Adding one string sets the string_impl_ of the classes_ Backing.
//   set->AddAttribute("a");
//   EXPECT_TRUE(set->InvalidatesElement(*element));
//   // Adding another upgrades to a HashSet.
//   set->AddAttribute("c");
//   EXPECT_TRUE(set->InvalidatesElement(*element));
//
//   // These sets should not cause invalidation.
//   set = DescendantInvalidationSet::Create();
//   set->AddAttribute("c");
//   EXPECT_FALSE(set->InvalidatesElement(*element));
//   set->AddAttribute("d");
//   EXPECT_FALSE(set->InvalidatesElement(*element));
// }

// Once we setWholeSubtreeInvalid, we should not keep the HashSets.
TEST(InvalidationSetTest, SubtreeInvalid_AddBefore) {
  auto set = DescendantInvalidationSet::Create();
  set->AddClass("a");
  set->SetWholeSubtreeInvalid();

  ASSERT_TRUE(set->IsEmpty());
}

// Don't (re)create HashSets if we've already setWholeSubtreeInvalid.
TEST(InvalidationSetTest, SubtreeInvalid_AddAfter) {
  auto set = DescendantInvalidationSet::Create();
  set->SetWholeSubtreeInvalid();
  set->AddTagName("a");

  ASSERT_TRUE(set->IsEmpty());
}

// No need to keep the HashSets when combining with a wholeSubtreeInvalid set.
TEST(InvalidationSetTest, SubtreeInvalid_Combine_1) {
  auto set1 = DescendantInvalidationSet::Create();
  auto set2 = DescendantInvalidationSet::Create();

  set1->AddId("a");
  set2->SetWholeSubtreeInvalid();

  set1->Combine(*set2);

  ASSERT_TRUE(set1->WholeSubtreeInvalid());
  ASSERT_TRUE(set1->IsEmpty());
}

// No need to add HashSets from combining set when we already have
// wholeSubtreeInvalid.
// TEST(InvalidationSetTest, SubtreeInvalid_Combine_2) {
//   auto set1 = DescendantInvalidationSet::Create();
//   auto set2 = DescendantInvalidationSet::Create();
//
//   set1->SetWholeSubtreeInvalid();
//   set2->AddAttribute("a");
//
//   set1->Combine(*set2);
//
//   ASSERT_TRUE(set1->WholeSubtreeInvalid());
//   ASSERT_TRUE(set1->IsEmpty());
// }

TEST(InvalidationSetTest, SelfInvalidationSet_Combine) {
  InvalidationSet* self_set = InvalidationSet::SelfInvalidationSet();

  EXPECT_TRUE(self_set->IsSelfInvalidationSet());
  self_set->Combine(*self_set);
  EXPECT_TRUE(self_set->IsSelfInvalidationSet());

  auto set = DescendantInvalidationSet::Create();
  EXPECT_FALSE(set->InvalidatesSelf());
  set->Combine(*self_set);
  EXPECT_TRUE(set->InvalidatesSelf());
}

}  // namespace
}  // namespace css
}  // namespace lynx
