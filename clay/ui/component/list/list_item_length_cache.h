// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LIST_ITEM_LENGTH_CACHE_H_
#define CLAY_UI_COMPONENT_LIST_LIST_ITEM_LENGTH_CACHE_H_

#include <list>
#include <optional>
#include <string>

namespace clay {

// This class is used to cache the length (width/height depending on the
// orientation) of items in a list view.
// The length of an item is known only after it is laid out. `kInvalidLength` is
// used for item that is not laid out yet.
// Instead of representing the length of each item in a vector, a list of
// sections is used. Starting from position 0, a section represents a continuous
// range of items with the same length. For example, a section from 0 to 10 with
// length 2 means items from 0 to 9 have length 2.
// The whole cache is continuous: a section's `from_pos` must be equal to
// `to_pos` of the previous section (if any).
class ListItemLengthCache {
 public:
  static constexpr float kInvalidLength = -1.f;
  struct Section {
    int from_pos;  // closed
    int to_pos;    // open
    float length;

    Section(int f, int t, float l = kInvalidLength)
        : from_pos(f), to_pos(t), length(l) {}
  };
  using Sections = std::list<Section>;

  ListItemLengthCache();
  ~ListItemLengthCache();

  void Clear();
  void SetLength(int position, float length);
  void InvalidateLength(int position);
  std::optional<float> GetLength(int position);

  const Sections& GetSections() const { return sections_; }

  // Returns the to_pos of the last section.
  int GetSize() const;

  // Insert item at `position_start`, and move items following `position_start`
  // backward.
  void OnItemsAdded(int position_start, int item_count);
  // Remove item at `position_start`, and move items following `position_start`
  // forward.
  void OnItemsRemoved(int position_start, int item_count);
  // Update length of item at `position_start` to `kInvalidLength`.
  void OnItemsUpdated(int position_start, int item_count);
  // Update length of item at `to` to length of item at `from`.
  void OnItemsMoved(int from, int to, int item_count);

  static std::string ToString(const Sections& sections);

 private:
  Sections::iterator FindPosition(int position);

  Sections sections_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_LIST_ITEM_LENGTH_CACHE_H_
