// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_ASCII_TRIE_H_
#define CLAY_FML_ASCII_TRIE_H_

#include <memory>
#include <string>
#include <vector>

namespace fml {

/// A trie for looking for ASCII prefixes.
class AsciiTrie {
 public:
  struct TrieNode;
  typedef std::unique_ptr<TrieNode> TrieNodePtr;
  /// The max Ascii value.
  static const int kMaxAsciiValue = 128;

  /// Clear and insert all the entries into the trie.
  void Fill(const std::vector<std::string>& entries);

  /// Returns true if \p argument is prefixed by the contents of the trie.
  inline bool Query(const char* argument) {
    return !node_ || Query(node_.get(), argument);
  }

  struct TrieNode {
    TrieNodePtr children[kMaxAsciiValue];
  };

 private:
  static bool Query(TrieNode* trie, const char* query);
  TrieNodePtr node_;
};
}  // namespace fml

#endif  // CLAY_FML_ASCII_TRIE_H_
