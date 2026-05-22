// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/fml/logging.h"
#include "clay/ui/component/list/list_item_length_cache.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {

using Section = ListItemLengthCache::Section;
using Sections = ListItemLengthCache::Sections;

bool operator==(const Sections& l, const Sections& r) {
  if (l.size() != r.size()) {
    return false;
  }

  auto l_itr = l.begin();
  auto r_itr = r.begin();
  for (; l_itr != l.end(); ++l_itr, ++r_itr) {
    if (l_itr->from_pos != r_itr->from_pos || l_itr->to_pos != r_itr->to_pos ||
        l_itr->length != r_itr->length) {
      return false;
    }
  }
  return true;
}

}  // namespace

TEST(ListItemLengthCache, Test) {
  ListItemLengthCache cache;
  cache.SetLength(0, 100);
  auto expected = Sections{{0, 1, 100}};
  EXPECT_TRUE(cache.GetSections() == expected);

  for (int i = 1; i < 5; ++i) {
    cache.SetLength(i, 100);
  }
  // Should merge into 1 section
  expected = Sections{{0, 5, 100}};
  EXPECT_TRUE(cache.GetSections() == expected);

  cache.Clear();
  // The first inserted item does not start from zero. Should insert a section
  // with invalid length starting from 0.
  cache.SetLength(50, 100);
  expected = Sections{{0, 50, -1}, {50, 51, 100}};
  EXPECT_TRUE(cache.GetSections() == expected);

  cache.Clear();
  for (int i = 0; i < 50; ++i) {
    cache.SetLength(i, 10);
  }
  // Break a section into 3 sections.
  cache.SetLength(25, 20);
  expected = Sections{{0, 25, 10}, {25, 26, 20}, {26, 50, 10}};
  EXPECT_TRUE(cache.GetSections() == expected);

  // Merge them back into one.
  cache.SetLength(25, 10);
  expected = Sections{{0, 50, 10}};
  EXPECT_TRUE(cache.GetSections() == expected);
}

}  // namespace clay
