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
#include "clay/ui/common/text_selection.h"
#include "clay/ui/common/text_input_type_traits.h"
#include "clay/ui/component/editable/ime_listener.h"
#include "clay/ui/component/editable/caret_blink_controller.h"
#include "clay/ui/component/editable/text_input_controller.h"
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

namespace clay {

class TextShadowNode;
struct EditableInlineAtomicDelete;
enum class EditableInlineStreamItemType;
class BaseTextShadowNode;
class BaseView;
class RawTextShadowNode;
class ShadowNode;

class TextView : public WithTypeInfo<TextView, BaseTextView>,
                 public GestureRecognizer::Delegate,
                 public IMEListener,
                 public TextInputClient {
 public:
  TextView(int id, PageView* page_view);
  TextView(int id, const std::string& tag,
           std::unique_ptr<RenderObject> render_object, PageView* page_view);
  ~TextView() override;

  void SetAttribute(const char* attr, const clay::Value& value) override;
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
  void OnCommitText(std::string text) override;
  void OnComposingText(std::string text) override;
  void OnKeyboardEvent(std::unique_ptr<KeyEvent> key_event) override;
  void OnDeleteSurroundingText(int before_length, int after_length) override;
  void OnPerformAction(KeyboardAction action) override;
  void OnFinishInput() override;
  void UpdateEditingState(std::string text, TextSelection selection,
                          TextRange composing, Affinity affinity) override;
  void PerformAction() override;

  bool IsContentEditable() const { return content_editable_; }
  bool InsertEditableText(const std::u16string& text);
  void SetEditableTextShadowNodeForTesting(TextShadowNode* node);
  void SetEditableCaretOffsetForTesting(size_t offset);

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
  void MoveCaretTo(FloatPoint point);
  void SelectWordAt(FloatPoint point);
  void SelectLineAt(FloatPoint point);

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
  void HideSelectionHandle();
  void UpdateSelectionHandle(FloatPoint point,
                             SelectionHandleView* handle_bar = nullptr);

  FloatRect GetDisplayRect();

  void HandleCopy();
  void HandleSelectAll();

  void BringIntoView(TextBox* text_box);

  ScrollView* FindScrollView();

  bool IsPointerAllowed(const GestureRecognizer& gesture_recognizer,
                        const PointerEvent& event) override;

  void OnViewPostionUpdate(FloatPoint scroll_offset) override;

 private:
  BaseView* GetTopViewToAcceptEvent(const FloatPoint& position,
                                    FloatPoint* relative_position,
                                    int platform_try_hit_id = -1) override;
  BaseView* GetViewAtPosition(const FloatPoint& point_by_paragraph,
                              const FloatPoint& point_by_page,
                              int platform_try_hit_id = -1);
  bool ClickOnText(size_t glyph_index, const FloatPoint& point_by_paragraph,
                   txt::Paragraph* paragraph);
  bool HandleEditableKeyEvent(const KeyEvent* key_event);
  FloatPoint ViewPointToParagraphPoint(FloatPoint point_by_self);
  TextShadowNode* GetEditableTextShadowNode();
  bool IsRuntimeContentEditable();
  void EnsureRuntimeContentEditableState();
  size_t GetEditableStreamLength(TextShadowNode* shadow_node);
  std::u16string GetEditableStreamText(TextShadowNode* shadow_node);
  size_t GetEditableOffsetForPoint(const FloatPoint& point_by_paragraph);
  bool GetEditableAtomicOffsetForPoint(const FloatPoint& point_by_paragraph,
                                       size_t* editable_offset);
  size_t GetEditableOffsetForRenderOffset(size_t render_offset);
  size_t GetRenderOffsetForEditableOffset(size_t editable_offset);
  size_t GetEditableOffsetForPlaceholderId(int placeholder_id);
  std::pair<size_t, size_t> GetEditableSelectionRange(size_t stream_length);
  void SetEditableSelectionRange(size_t base, size_t extent);
  bool ReplaceEditableRange(size_t start, size_t end,
                            const std::u16string& replacement,
                            size_t selection_offset);
  bool UndoEditableMutation();
  void PushEditableUndoSnapshot(TextShadowNode* shadow_node);
  void RemoveEditableAtomicViews(
      const std::vector<EditableInlineAtomicDelete>& deletes);
  void BeginEditableInput();
  void EndEditableInput();
  void SyncEditableInputStateToPlatform();
  void UpdateEditableCaretRectToPlatform();
  void MoveEditableCaretToOffset(size_t offset);
  void RefreshEditableTextView(TextShadowNode* shadow_node);
#ifndef ENABLE_CLAY_LITE
  void UpdateContentEditableCaretBlinking();
  void StopContentEditableCaretBlinking();
#endif

  bool is_text_selection_ = false;
  bool content_editable_ = false;
  bool editable_caret_initialized_ = false;
  size_t editable_caret_offset_ = 0;
  size_t editable_selection_base_offset_ = 0;
  size_t editable_selection_extent_offset_ = 0;
  bool syncing_editable_input_state_ = false;
  bool applying_editable_selection_to_render_ = false;
  bool ignore_next_collapsed_render_selection_change_ = false;
  TextShadowNode* editable_text_shadow_node_for_testing_ = nullptr;
  std::unique_ptr<TextInputController> text_input_controller_;
  struct EditableRawTextSnapshot {
    RawTextShadowNode* node = nullptr;
    std::u16string text;
  };
  struct EditableAtomicSnapshot {
    EditableInlineStreamItemType type;
    ShadowNode* node = nullptr;
    BaseTextShadowNode* parent = nullptr;
    int child_index = -1;
    BaseView* view_parent = nullptr;
    int view_child_index = -1;
    int placeholder_id = -1;
  };
  struct EditableUndoSnapshot {
    std::vector<EditableRawTextSnapshot> raw_texts;
    std::vector<EditableAtomicSnapshot> atomic_children;
    size_t selection_base = 0;
    size_t selection_extent = 0;
  };
  std::vector<EditableUndoSnapshot> editable_undo_stack_;
  MultiTapGestureRecognizer* multi_tap_recognizer_ = nullptr;
#if defined(OS_ANDROID) || defined(OS_IOS)
  LongPressGestureRecognizer* long_press_recognizer_ = nullptr;
#else
  DragGestureRecognizer* drag_recognizer_ = nullptr;
#endif
#ifndef ENABLE_CLAY_LITE
  SelectionPopupView* selection_popup_ = nullptr;
  OverlayView* selection_handle_container_ = nullptr;
  SelectionHandleView* start_selection_handle_ = nullptr;
  SelectionHandleView* end_selection_handle_ = nullptr;
  std::unique_ptr<CaretBlinkController> caret_blink_controller_;
  int selection_start_pos_ = -1;
  int selection_end_pos_ = -1;
  FloatPoint scroll_offset_;
#endif

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
