// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/common/text_input_model.h"

#include <algorithm>
#include <string>

#include "base/include/string/string_utils.h"

namespace clay {

using clay::Affinity;

TextInputModel::TextInputModel() = default;

TextInputModel::~TextInputModel() = default;

bool TextInputModel::operator==(const TextInputModel& model) const {
  if (model.GetText() == GetText() && model.selection() == selection() &&
      model.composing_range() == composing_range() &&
      model.SelectionAffinity() == SelectionAffinity()) {
    return true;
  }
  return false;
}
TextInputModel::TextInputModel(const TextInputModel& text_editing_value) =
    default;

TextInputModel::TextInputModel(const std::string& text,
                               const TextRange& selection_range,
                               const TextRange& composing_range, bool composing,
                               clay::Affinity selection_affinity)
    : text_{lynx::base::U8StringToU16(text)},
      selection_{selection_range},
      composing_range_{composing_range},
      composing_(composing),
      selection_affinity_{selection_affinity} {}

bool TextInputModel::SetText(const std::string& text,
                             const TextRange& selection,
                             const TextRange& composing_range) {
  return SetText(lynx::base::U8StringToU16(text), selection, composing_range);
}

bool TextInputModel::SetText(const std::u16string& text,
                             const TextRange& selection,
                             const TextRange& composing_range) {
  text_ = text;
  TrimToLimit();
  if (!text_range().Contains(selection) ||
      !text_range().Contains(composing_range)) {
    return false;
  }

  selection_ = selection;
  composing_range_ = composing_range;
#if !OS_WIN
  composing_ = !composing_range.collapsed();
#endif  // !OS_WIN
  return true;
}

void TextInputModel::SetTextAndReserveSelectionState(const std::string& text) {
  SetTextAndReserveSelectionState(lynx::base::U8StringToU16(text));
}
void TextInputModel::SetTextAndReserveSelectionState(
    const std::u16string& text) {
  text_ = text;
  if (!text_range().Contains(selection_)) {
    selection_ = TextRange{text.length()};
  }
}

bool TextInputModel::SetSelection(const TextRange& range) {
  if (composing_ && !range.collapsed()) {
    return false;
  }
  if (!editable_range().Contains(range)) {
    return false;
  }
  selection_ = range;
  return true;
}

bool TextInputModel::SetComposingRange(const TextRange& range,
                                       size_t cursor_offset) {
  if (!composing_ || !text_range().Contains(range)) {
    return false;
  }
  composing_range_ = range;
  selection_ = TextRange(range.start() + cursor_offset);
  return true;
}

void TextInputModel::BeginComposing() {
  composing_ = true;
  composing_range_ = TextRange(selection_.start());
}

void TextInputModel::UpdateComposingText(const std::u16string& text) {
  // Preserve selection if we get a no-op update to the composing region.
  if (text.length() == 0 && composing_range_.collapsed()) {
    return;
  }
  DeleteSelected();
  text_.replace(composing_range_.start(), composing_range_.length(), text);
  composing_range_.set_end(composing_range_.start() + text.length());
  selection_ = TextRange(composing_range_.end());
  TrimToLimit();
}

void TextInputModel::UpdateComposingText(const std::string& text) {
  UpdateComposingText(lynx::base::U8StringToU16(text));
}

void TextInputModel::CommitComposing() {
  // Preserve selection if no composing text was entered.
  if (composing_range_.collapsed()) {
    return;
  }
  composing_range_ = TextRange(composing_range_.end());
  selection_ = composing_range_;
}

void TextInputModel::EndComposing() {
  composing_ = false;
  composing_range_ = TextRange(0);
}

bool TextInputModel::DeleteSelected() {
  if (selection_.collapsed()) {
    return false;
  }
  size_t start = selection_.start();
  text_.erase(start, selection_.length());
  selection_ = TextRange(start);
  if (composing_) {
    // This occurs only immediately after composing has begun with a selection.
    composing_range_ = selection_;
  }
  return true;
}

void TextInputModel::AddCodePoint(char32_t c) {
  if (c <= 0xFFFF) {
    AddText(std::u16string({static_cast<char16_t>(c)}));
  } else {
    char32_t to_decompose = c - 0x10000;
    AddText(std::u16string({
        // High surrogate.
        static_cast<char16_t>((to_decompose >> 10) + 0xd800),
        // Low surrogate.
        static_cast<char16_t>((to_decompose % 0x400) + 0xdc00),
    }));
  }
}

void TextInputModel::AddText(const std::u16string& text) {
  DeleteSelected();
  if (composing_) {
    // Delete the current composing text, set the cursor to composing start.
    text_.erase(composing_range_.start(), composing_range_.length());
    selection_ = TextRange(composing_range_.start());
    composing_range_.set_end(composing_range_.start() + text.length());
  }
  size_t position = selection_.position();
  text_.insert(position, text);
  selection_ = TextRange(position + text.length());
  TrimToLimit();
}

void TextInputModel::AddText(const std::string& text) {
  AddText(lynx::base::U8StringToU16(text));
}

bool TextInputModel::Backspace() {
  if (DeleteSelected()) {
    return true;
  }
  // There is no selection. Delete the preceding codepoint.
  size_t position = selection_.position();
  if (position != editable_range().start()) {
    int count = IsTrailingSurrogate(text_.at(position - 1)) ? 2 : 1;
    text_.erase(position - count, count);
    selection_ = TextRange(position - count);
    if (composing_) {
      composing_range_.set_end(composing_range_.end() - count);
    }
    return true;
  }
  return false;
}

void TextInputModel::TrimToLimit() {
  if (max_length_ <= 0) {
    return;
  }
  // For performance & convience, use utf16 length instead of codepoint count.
  // `Backspace` would handle surrogate pair case.
  while (text_.size() > max_length_) {
    Backspace();
  }
}

bool TextInputModel::Delete() {
  if (DeleteSelected()) {
    return true;
  }
  // There is no selection. Delete the preceding codepoint.
  size_t position = selection_.position();
  if (position < editable_range().end()) {
    int count = IsLeadingSurrogate(text_.at(position)) ? 2 : 1;
    text_.erase(position, count);
    if (composing_) {
      composing_range_.set_end(composing_range_.end() - count);
    }
    return true;
  }
  return false;
}

bool TextInputModel::DeleteSurrounding(int offset_from_cursor, int count) {
  size_t max_pos = editable_range().end();
  size_t start = selection_.extent();
  if (offset_from_cursor < 0) {
    for (int i = 0; i < -offset_from_cursor; i++) {
      // If requested start is before the available text then reduce the
      // number of characters to delete.
      if (start == editable_range().start()) {
        count = i;
        break;
      }
      start -= IsTrailingSurrogate(text_.at(start - 1)) ? 2 : 1;
    }
  } else {
    for (int i = 0; i < offset_from_cursor && start != max_pos; i++) {
      start += IsLeadingSurrogate(text_.at(start)) ? 2 : 1;
    }
  }

  auto end = start;
  for (int i = 0; i < count && end != max_pos; i++) {
    end += IsLeadingSurrogate(text_.at(start)) ? 2 : 1;
  }

  if (start == end) {
    return false;
  }

  auto deleted_length = end - start;
  text_.erase(start, deleted_length);

  // Cursor moves only if deleted area is before it.
  selection_ = TextRange(offset_from_cursor <= 0 ? start : selection_.start());

  // Adjust composing range.
  if (composing_) {
    composing_range_.set_end(composing_range_.end() - deleted_length);
  }
  return true;
}

bool TextInputModel::MoveCursorToBeginning() {
  size_t min_pos = editable_range().start();
  if (selection_.collapsed() && selection_.position() == min_pos) {
    return false;
  }
  selection_ = TextRange(min_pos);
  return true;
}

bool TextInputModel::MoveCursorToEnd() {
  size_t max_pos = editable_range().end();
  if (selection_.collapsed() && selection_.position() == max_pos) {
    return false;
  }
  selection_ = TextRange(max_pos);
  return true;
}

bool TextInputModel::SelectToBeginning() {
  size_t min_pos = editable_range().start();
  if (selection_.collapsed() && selection_.position() == min_pos) {
    return false;
  }
  selection_ = TextRange(selection_.base(), min_pos);
  return true;
}

bool TextInputModel::SelectToEnd() {
  size_t max_pos = editable_range().end();
  if (selection_.collapsed() && selection_.position() == max_pos) {
    return false;
  }
  selection_ = TextRange(selection_.base(), max_pos);
  return true;
}

bool TextInputModel::MoveCursorForward() {
  // If there's a selection, move to the end of the selection.
  if (!selection_.collapsed()) {
    selection_ = TextRange(selection_.end());
    return true;
  }
  // Otherwise, move the cursor forward.
  size_t position = selection_.position();
  if (position != editable_range().end()) {
    int count = IsLeadingSurrogate(text_.at(position)) ? 2 : 1;
    selection_ = TextRange(position + count);
    return true;
  }
  return false;
}

bool TextInputModel::MoveCursorBack() {
  // If there's a selection, move to the beginning of the selection.
  if (!selection_.collapsed()) {
    selection_ = TextRange(selection_.start());
    return true;
  }
  // Otherwise, move the cursor backward.
  size_t position = selection_.position();
  if (position != editable_range().start()) {
    int count = IsTrailingSurrogate(text_.at(position - 1)) ? 2 : 1;
    selection_ = TextRange(position - count);
    return true;
  }
  return false;
}

std::string TextInputModel::GetText() const {
  return lynx::base::U16StringToU8(text_);
}

int TextInputModel::GetCursorOffset() const {
  // Measure the length of the current text up to the selection extent.
  // There is probably a much more efficient way of doing this.
  auto leading_text = text_.substr(0, selection_.extent());
  return lynx::base::U16StringToU8(leading_text).size();
}

void TextInputModel::SetSelectionAffinity(clay::Affinity selection_affinity) {
  selection_affinity_ = selection_affinity;
}

}  // namespace clay
