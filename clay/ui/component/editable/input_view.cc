// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/editable/input_view.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "clay/ui/component/editable/editable_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/lynx_module/type_utils.h"

namespace clay {
namespace {

LYNX_UI_METHOD_BEGIN(InputView) {
  LYNX_UI_METHOD(InputView, addText);
  LYNX_UI_METHOD(InputView, sendDelEvent);
  LYNX_UI_METHOD(InputView, beginEdit);
  LYNX_UI_METHOD(InputView, quitEdit);
  LYNX_UI_METHOD(InputView, controlKeyBoard);
  LYNX_UI_METHOD(InputView, setInputFilter);
  LYNX_UI_METHOD(InputView, select);
}
LYNX_UI_METHOD_END(InputView);

}  // namespace

InputView::InputView(int id, PageView* page_view, bool is_multiline,
                     bool layout_root_candidate)
    : InputView(id, id, page_view, is_multiline, layout_root_candidate) {
  // max_lines 1 by default, refactored from view_factory.cc
  SetMaxLines(1);
}

InputView::InputView(int id, int callback_id, PageView* page_view,
                     bool is_multiline, bool layout_root_candidate)
    : WithTypeInfo(id, callback_id, "input", page_view, is_multiline,
                   layout_root_candidate) {}

void InputView::SetAttribute(const char* attr_c, const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  switch (kw) {
    case KeywordID::kEditOnFocus:
      begin_edit_on_focus_ = attribute_utils::GetBool(value);
      break;
    case KeywordID::kFocus: {
      bool focus = attribute_utils::GetBool(value);
      if (focus) {
        // NOLINTNEXTLINE
        page_view()->GetTaskRunner()->PostTask([this] { RequestFocus(); });
      } else {
        ClearFocus();
      }
    } break;
    case KeywordID::kValue:
      SetContent(attribute_utils::GetCString(value));
      break;
    case KeywordID::kFontSize: {
      double font_size = 0.0;
      if (attribute_utils::TryGetNum(value, font_size)) {
        SetFontSize(static_cast<float>(font_size));
      }
    } break;
    case KeywordID::kLineHeight:
      SetLineHeight(attribute_utils::GetDouble(value, GetDefaultFontSize()));
      break;
    case KeywordID::kSendComposingInput:
      SetSendComposingInput(attribute_utils::GetBool(value));
      break;
    default:
      EditableView::SetAttribute(attr_c, value);
      break;
  }
}

void InputView::SetContent(const std::string& content) {
  auto text_editing_value = GetTextEditingValue();
  text_editing_value.SetTextAndReserveSelectionState(content);
  FilterInputIfNeeded(&text_editing_value);
  SetTextEditingValue(text_editing_value);
  MarkNeedsLayout();
}

void InputView::SetFontSize(float font_size) {
  EditableView::SetFontSize(font_size);
  UpdateLineHeightIfNeeded();
}

void InputView::UpdateLineHeightIfNeeded() {
  if (line_height_.has_value()) {
    text_style_.line_height = *line_height_ / *text_style_.font_size;
  }
}

void InputView::SetLineHeight(float line_height) {
  line_height_ = line_height;
  UpdateLineHeightIfNeeded();
  MarkNeedsLayout();
}

void InputView::addText(const LynxModuleValues& args,
                        const LynxUIMethodCallback& callback) {
  FML_DCHECK(args.names.size() == args.values.size());

  std::string content;
  CastNamedLynxModuleArgs({"text"}, args, content);
  OnCommitText(content);
  callback(LynxUIMethodResult::kSuccess, clay::Value());
}

void InputView::sendDelEvent(const LynxModuleValues& args,
                             const LynxUIMethodCallback& callback) {
  FML_DCHECK(args.names.size() == args.values.size());

  int action = 0;
  int length = 0;
  CastNamedLynxModuleArgs({"action", "length"}, args, action, length);
  if (action == 1) {
    // According to lynx input UI method definition, if action == 1, delete
    // text with length 1.
    length = 1;
  }
  DeleteSurroundingText(length, 0);
  callback(LynxUIMethodResult::kSuccess, clay::Value());
}

void InputView::controlKeyBoard(const LynxModuleValues& args,
                                const LynxUIMethodCallback& callback) {
  auto res = LynxUIMethodResult::kUnknown;
  // null is not acceptable, so don't use `CastLynxModuleArgs`
  if (auto it = std::find(args.names.begin(), args.names.end(), "action");
      it != args.names.end()) {
    int index = std::distance(args.names.begin(), it);
    double action_num;
    if (attribute_utils::TryGetNum(args.values.at(index), action_num)) {
      switch (static_cast<int>(action_num)) {
        case 0:  // Focus & show keyboard
          BeginEditingIfNeeded();
          res = LynxUIMethodResult::kSuccess;
          break;
        case 1:  // Focus & hide keyboard
          FML_DLOG(WARNING) << "controlKeyBoard(action = 1) not implemented";
          // TODO(renjiayi): complete this
          break;
        case 2:  // Focus & keep keyboard unchanged
          RequestFocus();
          res = LynxUIMethodResult::kSuccess;
          break;
        case 3:  // Blur
          ClearFocus();
          res = LynxUIMethodResult::kSuccess;
          break;
        default:
          break;
      }
    }
  }
  callback(res, clay::Value());
}

void InputView::setInputFilter(const LynxModuleValues& args,
                               const LynxUIMethodCallback& callback) {
  std::string pattern;
  CastNamedLynxModuleArgs({"pattern"}, args, pattern);
  if (pattern.empty()) {
    input_filter_pattern_ = {};
    ApplyFilter(&InputView::FilterInputTextByUser, false);
  } else {
    input_filter_pattern_ = pattern;
    ApplyFilter(&InputView::FilterInputTextByUser, true);
  }
  callback(LynxUIMethodResult::kSuccess, clay::Value());
}

void InputView::select(const LynxModuleValues& args,
                       const LynxUIMethodCallback& callback) {
  auto text_editing_value = GetTextEditingValue();
  text_editing_value.SetSelection(text_editing_value.text_range());
  SetTextEditingValue(text_editing_value);
  MarkNeedsLayout();
  callback(LynxUIMethodResult::kSuccess, clay::Value());
}

void InputView::beginEdit(const LynxModuleValues&,
                          const LynxUIMethodCallback& callback) {
  BeginEditingIfNeeded();
  callback(LynxUIMethodResult::kSuccess, clay::Value());
}

void InputView::quitEdit(const LynxModuleValues&,
                         const LynxUIMethodCallback& callback) {
  if (editing_) {
    QuitEditing();
  }
  callback(LynxUIMethodResult::kSuccess, clay::Value());
}

void InputView::UpdateEditingState(std::string text, TextSelection selection,
                                   TextRange composing, Affinity affinity) {
  bool is_composing = !(composing.collapsed());
  const auto& old_value = GetTextEditingValue();
  auto new_value = TextEditingValue(
      text, TextRange(selection.base_offset(), selection.extent_offset()),
      composing, is_composing, affinity);
  if (old_value == new_value) {
    return;
  }
  if (!is_composing) {
    FilterInputIfNeeded(&new_value);
  }
  SetTextEditingValue(new_value, send_composing_input_ ? false : is_composing,
                      true);
  MarkNeedsLayout();
}

}  // namespace clay
