// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_TEXT_TEXT_VIEW_H_
#define CLAY_UI_COMPONENT_TEXT_TEXT_VIEW_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/gfx/geometry/float_point.h"
#include "clay/third_party/txt/src/txt/paragraph.h"
#include "clay/ui/common/measure_constraint.h"
#include "clay/ui/common/text_input_type_traits.h"
#include "clay/ui/component/overlay_view.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/selection_handle_view.h"
#include "clay/ui/component/selection_popup_view.h"
#include "clay/ui/component/text/base_text_view.h"
#include "clay/ui/component/text/inline_emoji_bitmap.h"
#include "clay/ui/component/text/layout_client.h"
#include "clay/ui/gesture/drag_gesture_recognizer.h"
#include "clay/ui/gesture/long_press_gesture_recognizer.h"
#include "clay/ui/gesture/multi_tap_gesture_recognizer.h"
#include "clay/ui/rendering/text/render_text.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck

namespace clay {

class TextView : public WithTypeInfo<TextView, BaseTextView>,
                 public GestureRecognizer::Delegate {
 public:
  TextView(int id, PageView* page_view);
  TextView(int id, const std::string& tag,
           std::unique_ptr<RenderObject> render_object, PageView* page_view);
  ~TextView() override;

  void SetAttribute(const char* attr, const clay::Value& value) override;
  void ClearInlineIndexes();
  void PushInlineImageIndex(int id, int placeholder_id);
  void PushInlineViewIndex(int id, int placeholder_id);
  void SetInlineEmojiInfo(std::vector<InlineEmojiInfo> inline_emoji_info);
  void SetBorderWidth(std::vector<Side> sides,
                      std::vector<float> widths) override;
  void SetPaddings(float padding_left, float padding_top, float padding_right,
                   float padding_bottom) override;

  /**
   * @brief Change the foreground color of view.
   *
   * In CSS, the color property is mainly used to set the foreground color of an
   * element, which is the color of the text in BaseView.
   *
   * Because the process of text layout is mainly completed in the asynchronous
   * threads of Lynx, this interface is currently only available for Compositor
   * animations.
   *
   * @param color The target color of text or indicator.
   */
  void SetColor(Color color);

  bool OnKeyEvent(const KeyEvent* event) override;
  bool ApplyHotKey(const KeyEvent* key_event);
  void UpdateHotKeyTag(LogicalKeyboardKey key_code, bool is_up);
  void HandleCommandHotKey(LogicalKeyboardKey key_code);
  void HandleCtrlHotKey(LogicalKeyboardKey key_code);
  void HandleWinCtrlAndMacCommandHotKey(LogicalKeyboardKey key_code);

  void ResetGestureRecognizers();
  void ClearGestureRecognizers();

  void FocusHasChanged(bool focused, bool is_leaf) override;
  bool OnScrollToVisible() override { return false; }

  void PerformBeginSelection(FloatPoint point);
  void PerformMoveSelection(FloatPoint point,
                            SelectionHandleView* handle_bar = nullptr);
  void PerformCancelSelection();

  TextRange SelectWord(size_t pos);
  TextRange SelectWord(size_t pos, Affinity affinity);
  TextRange SelectLine(size_t pos);
  TextRange SelectLine(size_t pos, Affinity affinity);
  TextRange SelectParagraph(size_t pos);
  TextRange SelectParagraph(size_t pos, Affinity affinity);

  void OnSelectionChanged(int selection_start, int selection_end);

  void setTextSelection(const LynxModuleValues& args,
                        const LynxUIMethodCallback& callback);
  void getTextBoundingRect(const LynxModuleValues& args,
                           const LynxUIMethodCallback& callback);
  void getSelectedText(const LynxUIMethodCallback& callback);

  void OnContentSizeChanged(const FloatRect& old_rect,
                            const FloatRect& new_rect) override;
  void OnBoundsChanged(const FloatRect& old_bounds,
                       const FloatRect& new_bounds) override;

  void SetParagraph(std::unique_ptr<txt::Paragraph> paragraph,
                    std::u16string text) {
    paragraph_ = std::move(paragraph);
    GetRenderText()->SetParagraph(paragraph_.get(), text);
  }

  void SetParagraph(txt::Paragraph* paragraph, std::u16string text) {
    GetRenderText()->SetParagraph(paragraph, text);
  }

#ifdef ENABLE_ACCESSIBILITY
  std::u16string GetAccessibilityLabel() const override;
#endif

  RenderText* GetRenderText();

  std::vector<FloatPoint> GetAnchorPosition();

  void ShowSelectionPopup();
  void HideSelectionPopup();
  void ShowSelectionHandle(bool show_start_handle = true,
                           bool show_end_handle = true);
  void SetSelectionBackgroundColor(Color selection_background_color);
  void SetSelectionHandleColor(Color selection_handle_color);
  void SetSelectionHandleSize(float selection_handle_size);
  void HideSelectionHandle();
  void UpdateSelectionHandle(FloatPoint point, SelectionHandleView* handle_bar);
  void UpdateSelectionHandleTypes();
  void UpdateSelectionRange(int selection_start, int selection_end);
  void ResetSelectionRange();
  void ClearSelection();

  FloatRect GetDisplayRect(int* overflow = nullptr);

  void HandleCopy();
  void HandleSelectAll();

  void BringIntoView(TextBox* text_box);

  ScrollView* FindScrollView();

  bool IsPointerAllowed(const GestureRecognizer& gesture_recognizer,
                        const PointerEvent& event) override;

  void OnViewPostionUpdate(FloatPoint scroll_offset) override;

 private:
  FRIEND_TEST(TextSelectionTest, SetSelectionHandleSizeRebuildsVisibleHandles);
  FRIEND_TEST(TextSelectionTest,
              SetTextSelectionKeepsHandlesAtVisualSelectionEnds);
  FRIEND_TEST(TextSelectionTest, SetTextSelectionHidesVisualStartHandle);
  FRIEND_TEST(TextSelectionTest,
              SetAttributeUpdatesVisibleSelectionHandleColors);
  FRIEND_TEST(TextSelectionTest, SelectWordCrossesNestedInlineTextBoundaries);

  void UpdateSelectionHandleLayout(SelectionHandleView* handle);
  void BeginSelectionHandleDrag(const PointerEvent& event,
                                SelectionHandleView* handle_bar);
  void EndSelectionHandleDrag(SelectionHandleView* handle_bar);

  BaseView* GetTopViewToAcceptEvent(const FloatPoint& position,
                                    FloatPoint* relative_position,
                                    int platform_try_hit_id = -1) override;
  BaseView* GetViewAtPosition(const FloatPoint& point_by_paragraph,
                              const FloatPoint& point_by_page,
                              FloatPoint* relative_position,
                              int platform_try_hit_id = -1);
  bool ClickOnText(size_t glyph_index, const FloatPoint& point_by_paragraph,
                   txt::Paragraph* paragraph);
  bool is_text_selection_ = false;
#if defined(OS_WIN) || defined(OS_OSX)
  MultiTapGestureRecognizer* multi_tap_recognizer_ = nullptr;
  DragGestureRecognizer* drag_recognizer_ = nullptr;
  FloatPoint drag_start_point_;
#elif defined(OS_ANDROID) || defined(OS_IOS)
  MultiTapGestureRecognizer* double_tap_recognizer_ = nullptr;
  LongPressGestureRecognizer* long_press_recognizer_ = nullptr;
#else
  DragGestureRecognizer* drag_recognizer_ = nullptr;
#endif
  int selection_start_pos_ = -1;
  int selection_end_pos_ = -1;
#ifndef ENABLE_CLAY_LITE
  SelectionPopupView* selection_popup_ = nullptr;
  OverlayView* selection_handle_container_ = nullptr;
  SelectionHandleView* start_selection_handle_ = nullptr;
  SelectionHandleView* end_selection_handle_ = nullptr;
  bool is_in_selection_ = false;
  FloatPoint scroll_offset_;
  FloatPoint selection_handle_drag_offset_;
  SelectionHandleView* dragging_selection_handle_ = nullptr;
  Color selection_handle_color_ = Color::kTransparent();
  float selection_handle_size_ = 0;
#endif

  bool selection_direction_forward_ = true;
  bool custom_text_selection_ = false;
  bool custom_context_menu_ = false;
  uint32_t hot_key_tag_ = 0;

  fml::WeakPtrFactory<TextView> weak_factory_;
  std::unique_ptr<txt::Paragraph> paragraph_;
  std::unordered_map<int, int> inline_images_index_ = {};
  std::unordered_map<int, int> inline_views_index_ = {};
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_TEXT_TEXT_VIEW_H_
