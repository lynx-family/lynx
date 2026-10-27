// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/fml/ascii_trie.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

using fml::AsciiTrie;

TEST(AsciiTableTest, Simple) {
  AsciiTrie trie;
  auto entries = std::vector<std::string>{"foo"};
  trie.Fill(entries);
  ASSERT_TRUE(trie.Query("foobar"));
  ASSERT_FALSE(trie.Query("google"));
}

TEST(AsciiTableTest, ExactMatch) {
  AsciiTrie trie;
  auto entries = std::vector<std::string>{"foo"};
  trie.Fill(entries);
  ASSERT_TRUE(trie.Query("foo"));
}

TEST(AsciiTableTest, Empty) {
  AsciiTrie trie;
  ASSERT_TRUE(trie.Query("foo"));
}

TEST(AsciiTableTest, MultipleEntries) {
  AsciiTrie trie;
  auto entries = std::vector<std::string>{"foo", "bar"};
  trie.Fill(entries);
  ASSERT_TRUE(trie.Query("foozzz"));
  ASSERT_TRUE(trie.Query("barzzz"));
}
