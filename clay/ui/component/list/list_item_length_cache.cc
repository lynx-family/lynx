// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_item_length_cache.h"

#include <algorithm>
#include <sstream>
#include <string>

#include "clay/fml/logging.h"

namespace clay {

ListItemLengthCache::ListItemLengthCache() = default;
ListItemLengthCache::~ListItemLengthCache() = default;

void ListItemLengthCache::Clear() { sections_.clear(); }

void ListItemLengthCache::SetLength(int position, float length) {
  FML_DCHECK(position >= 0);

  auto itr = FindPosition(position);

  // The iterator pointing to the target position after updating. It will be
  // used in merge phase.
  auto new_itr = itr;
  if (itr == sections_.end()) {
    if (sections_.empty()) {
      if (position > 0) {
        // Insert placement between[0, position).
        sections_.emplace_back(0, position);
      }
    } else {
      if (sections_.back().to_pos != position) {
        if (sections_.back().length == kInvalidLength) {
          // Last section is placement, just extend this section.
          sections_.back().to_pos = position;
        } else {
          // Insert placement between [last_section.to_pos, position)
          sections_.emplace_back(sections_.back().to_pos, position);
        }
      }
    }
    // Insert single element section.
    new_itr =
        sections_.emplace(sections_.end(), position, position + 1, length);
  } else {
    // `itr->from_pos` <= `position` && `position` < `itr->to_pos`
    Section& cur = *itr;
    if (cur.length == length) {
      return;
    }

    const Section cur_copy = cur;
    if (cur_copy.from_pos == position) {
      if (cur_copy.to_pos == position + 1) {
        // Single element section, just update length.
        cur.length = length;
        new_itr = itr;
      } else {
        // Split [position, to_pos) to [position, position+1) [position+1,
        // to_pos)
        new_itr = sections_.emplace(itr, position, position + 1, length);
        cur.from_pos = position + 1;
      }
    } else if (cur_copy.to_pos == position + 1) {
      // Split [from_pos, position+1) to [from_pos, position) [position,
      // position+1)
      sections_.emplace(itr, cur.from_pos, position, cur.length);
      itr->from_pos = position;
      itr->length = length;
      new_itr = itr;
    } else {
      // Split [from_pos, to_pos) to [from_pos, position) [position, position+1)
      // [position+1, to_pos)
      cur.from_pos = position + 1;
      new_itr = sections_.emplace(itr, position, position + 1, length);
      itr = sections_.emplace(new_itr, cur_copy.from_pos, position,
                              cur_copy.length);
    }
  }

  // Try to merge. If the length of the target section is the same as the
  // previous or the next section, it will be merged into it. Try to merge
  // previous.
  if (new_itr != sections_.begin()) {
    auto prev = new_itr;
    prev--;
    if (prev->length == length) {
      prev->to_pos = new_itr->to_pos;
      sections_.erase(new_itr);
      new_itr = prev;
    }
  }

  // Try to merge next.
  auto next = new_itr;
  next++;
  if (next != sections_.end()) {
    if (next->length == length) {
      next->from_pos = new_itr->from_pos;
      sections_.erase(new_itr);
    }
  }
}

void ListItemLengthCache::InvalidateLength(int position) {
  if (position < GetSize()) {
    SetLength(position, kInvalidLength);
  }
}

std::optional<float> ListItemLengthCache::GetLength(int position) {
  auto itr = FindPosition(position);
  if (itr == sections_.end()) {
    return std::nullopt;
  }

  FML_DCHECK(itr->from_pos <= position && position < itr->to_pos);
  return itr->length;
}

void ListItemLengthCache::OnItemsAdded(int position_start, int item_count) {
  for (int i = position_start; i < position_start + item_count; ++i) {
    auto itr = FindPosition(i);
    if (itr == sections_.end()) {
      SetLength(i, kInvalidLength);
    } else {
      if (itr->from_pos == i) {
        // Split [i, to_pos) to [i, i+1) [i+1, to_pos + 1).
        sections_.emplace(itr, i, i + 1);
      } else {
        // Split [from_pos, to_pos) to [from_pos, i) [i, i+1) [i+1, to_pos + 1).
        sections_.emplace(itr, itr->from_pos, i, itr->length);
        itr->from_pos = i;
        itr->to_pos = i + 1;
        itr->length = kInvalidLength;
        itr++;
      }
      int from = i + 1;
      while (itr != sections_.end()) {
        itr->from_pos = from;
        itr->to_pos += 1;
        from = itr->to_pos;
        itr++;
      }
    }
  }
}

void ListItemLengthCache::OnItemsRemoved(int position_start, int item_count) {
  for (int i = position_start; i < position_start + item_count; ++i) {
    auto itr = FindPosition(i);
    if (itr == sections_.end()) {
      continue;
    }
    // `itr->from_pos` <= `i` && `i` < `itr->to_pos`.
    int from = 0;
    if (itr->from_pos == i && itr->to_pos == i + 1) {
      // Remove single element section and update following sections.
      itr = sections_.erase(itr);
      from = i;
    } else {
      // Update current section and following sections.
      itr->to_pos -= 1;
      from = itr->to_pos;
      itr++;
    }
    while (itr != sections_.end()) {
      itr->from_pos = from;
      itr->to_pos -= 1;
      from = itr->to_pos;
      itr++;
    }
  }
}

void ListItemLengthCache::OnItemsUpdated(int position_start, int item_count) {
  for (int i = position_start; i < position_start + item_count; ++i) {
    SetLength(i, kInvalidLength);
  }
}

void ListItemLengthCache::OnItemsMoved(int from, int to, int item_count) {
  auto from_itr = FindPosition(from);
  auto to_itr = FindPosition(to);
  if (from_itr == sections_.end() || to_itr == sections_.end()) {
    return;
  }
  if (from == to) {
    // In-place move operation means content of cell changes, we just keep the
    // length cache same instead of invalidate it. Generally speaking, content
    // change won't contribute to dramatic length change. So keeping length
    // unchanged is more beneficial to ListLayoutManager.GetTotalLength.
    return;
  }
  int step = from < to ? 1 : -1;
  while (from != to) {
    SetLength(from, kInvalidLength);
    from += step;
  }
  SetLength(to, kInvalidLength);
}

int ListItemLengthCache::GetSize() const {
  if (sections_.size() == 0) {
    return 0;
  }
  return sections_.back().to_pos;
}

// static
std::string ListItemLengthCache::ToString(const Sections& sections) {
#if (DEBUG_LIST)
  std::stringstream ss;
  for (const Section& section : sections) {
    ss << "[" << section.from_pos << "," << section.to_pos << ") "
       << section.length << " | ";
  }
  return ss.str();
#else
  return "";
#endif
}

/**
 * @brief
 * return the first iterator of section whose `from_pos` <= `position` and
 * `to_pos` > `position` or iterator::end() if not found.
 * @param position list item position
 * @return ListItemLengthCache::Sections::iterator
 */
ListItemLengthCache::Sections::iterator ListItemLengthCache::FindPosition(
    int position) {
  if (position < GetSize()) {
    return std::lower_bound(
        sections_.begin(), sections_.end(), position,
        [](const Section& section, int pos) { return section.to_pos <= pos; });
  }
  return sections_.end();
}

}  // namespace clay
