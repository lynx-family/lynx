// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/base_view.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include "clay/fml/base64.h"
#include "clay/fml/logging.h"
#include "clay/gfx/animation/animation_properties_util.h"
#include "clay/gfx/animation/keyframes_manager.h"
#include "clay/gfx/animation/transition_manager.h"
#include "clay/gfx/geometry/filter_operations.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_rounded_rect.h"
#include "clay/gfx/geometry/path.h"
#include "clay/gfx/geometry/transform.h"
#include "clay/gfx/geometry/transform_operations.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/style/borders_data.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/common/background_data.h"
#include "clay/ui/component/base_view_animation_mutator.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/expose_manager/expose_observer.h"
#include "clay/ui/component/inline_image_view.h"
#include "clay/ui/component/native_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/text/base_text_view.h"
#include "clay/ui/component/text/raw_text_view.h"
#include "clay/ui/component/text/unicode_util.h"
#include "clay/ui/component/view_context.h"
#include "clay/ui/event/event_utils.h"
#include "clay/ui/gesture/mouse_region_manager.h"
#include "clay/ui/gesture_handler/gesture_detector.h"
#include "clay/ui/gesture_handler/handler/base_gesture_handler.h"
#include "clay/ui/lynx_module/type_utils.h"
#include "clay/ui/painter/gradient.h"
#include "clay/ui/resource/image_resource_fetcher.h"

#ifndef ENABLE_CLAY_LITE
#include "clay/ui/component/editable/editable_view.h"
#include "clay/ui/component/editable/textarea_ng_view.h"
#include "clay/ui/component/editable/textarea_view.h"
#endif

namespace clay {
namespace {
namespace utils = attribute_utils;
LYNX_UI_METHOD_BEGIN(BaseView) {
  LYNX_UI_METHOD(BaseView, setFocus);
  LYNX_UI_METHOD(BaseView, interceptBackKeyOnce);
  LYNX_UI_METHOD(BaseView, cancelInterceptBackKey);
  LYNX_UI_METHOD(BaseView, boundingClientRect);
  LYNX_UI_METHOD(BaseView, scrollIntoView);
  LYNX_UI_METHOD(BaseView, takeScreenshot);
}
LYNX_UI_METHOD_END(BaseView);

constexpr int64_t FORCE_CACHE_ANIMATION_DURATION = 500;

bool ShouldPassEventToNativeInherited(BaseView* view) {
  if (view == nullptr) {
    return false;
  } else if (view->CanEventThrough().has_value()) {
    return *view->CanEventThrough();
  } else if (view->Parent() == nullptr) {
    return false;
  } else {
    return ShouldPassEventToNativeInherited(view->Parent());
  }
}

const int PLATFORM_PERSPECTIVE_UNIT_NUMBER = 0;
const int PLATFORM_PERSPECTIVE_UNIT_VW = 1;
const int PLATFORM_PERSPECTIVE_UNIT_VH = 2;
const int PLATFORM_PERSPECTIVE_UNIT_DEFAULT = 3;
const int PLATFORM_PERSPECTIVE_UNIT_PX = 4;
const int DEFAULT_PERSPECTIVE_FACTOR = 100;
// INFO: Lynx sets CAMERA_DISTANCE_NORMALIZATION_MULTIPLIER to sqrt(5)
const float CAMERA_DISTANCE_NORMALIZATION_MULTIPLIER = 1;

}  // namespace

const std::unordered_set<KeywordID> kExposureAttributes = {
    KeywordID::kExposureScene,
    KeywordID::kExposureId,
    KeywordID::kExposureArea,
    KeywordID::kEnableExposureUiMargin,
    KeywordID::kEnableExposureUiClip,
    KeywordID::kExposureUiMarginLeft,
    KeywordID::kExposureUiMarginRight,
    KeywordID::kExposureUiMarginTop,
    KeywordID::kExposureUiMarginBottom,
    KeywordID::kExposureScreenMarginLeft,
    KeywordID::kExposureScreenMarginRight,
    KeywordID::kExposureScreenMarginTop,
    KeywordID::kExposureScreenMarginBottom,
    KeywordID::kUiappear,
    KeywordID::kUidisappear,
};

BaseView::BaseView(std::unique_ptr<RenderObject> render_object,
                   PageView* page_view)
    : BaseView(-1, "", std::move(render_object), page_view) {}

BaseView::BaseView(int id, std::string tag_name,
                   std::unique_ptr<RenderObject> render_object,
                   PageView* page_view)
    : id_(id),
      tag_(tag_name),
      page_view_(page_view),
      render_object_(std::move(render_object)),
      weak_factory_(this),
      overflow_(page_view_->DefaultOverflow()) {
  render_object_->SetID(id_);
  render_object_->SetOverflow(overflow_);
#if defined(ENABLE_MOUSE_TRACKING)
  if (page_view != this) {
    auto* mouse_region_manager = page_view->mouse_region_manager();
    if (mouse_region_manager) {
      mouse_region_manager->RegisterEnterCallback(
          this,
          [this](const PointerEvent& event) { this->OnMouseEnter(event); });
      mouse_region_manager->RegisterLeaveCallback(
          this,
          [this](const PointerEvent& event) { this->OnMouseLeave(event); });
      mouse_region_manager->RegisterHoverCallback(
          this,
          [this](const PointerEvent& event) { this->OnMouseHover(event); });
    }
    // set default cursor for every base view
    SetCursor({"default"});
  }
#endif
  if (page_view_) {
    render_object_->SetImageDecodeWithPriority(
        page_view_->ImageDecodeWithPriority());
  }
}

BaseView::~BaseView() {
  if (page_view() != this) {
    auto mouse_region_manager = page_view()->mouse_region_manager();
    if (mouse_region_manager) {
      mouse_region_manager->UnregisterCallback(this);
    }

    // needs layout is not equal to added to dirty
    if (needs_layout_ || IsLayoutRootCandidate()) {
      if (auto layout_controller = page_view_->GetLayoutController()) {
        bool has_this_node =
            layout_controller->RemoveDirtyNode(this->GetWeakPtr());
        if (has_this_node) {
          FML_LOG(ERROR) << "remove a dirty node in destruct, tag:" << tag_
                         << ", id :" << id_selector_;
        }
      }
    }

    if (has_intersection_observer_) {
      page_view_->intersection_observer_manager()->RemoveAll(this);
      has_intersection_observer_ = false;
    }
#ifdef ENABLE_ACCESSIBILITY
    if (auto* semantics_owner = GetSemanticsOwner()) {
      semantics_owner->RemoveDirtySemanticsForDescendants(this);
    }
#endif
  }
  if (destruct_listener_) {
    destruct_listener_(this);
  }
  // Destruct animation manually, this would trigger SetProperty at end of
  // animators.
  keyframes_mgr_.reset();
  transition_mgr_.reset();
  // KeyframesMgr & TransitionMgr refs this, so we need to release it after.
  animation_mutator_.reset();
}

void BaseView::Destroy() {
  OnDestroy();

  if (attach_to_tree_) {
    OnDetachFromTree();
  }
  FML_DCHECK(render_object());
  render_object()->Destroy();
  SetFocusable(false);
  // incase of double release
  data_set_ = clay::Value::Null();
}

BaseViewAnimationMutator* BaseView::GetAnimationMutator() {
  if (!animation_mutator_) {
    animation_mutator_ = std::make_unique<BaseViewAnimationMutator>(this);
  }
  return animation_mutator_.get();
}

bool BaseView::IsAppRegionDraggable() {
  if (app_region_.compare(attr_value::kAppRegionDrag) == 0) {
    return true;
  } else if (app_region_.compare(attr_value::kAppRegionNoDrag) == 0) {
    return false;
  } else if (app_region_.empty() && Parent()) {
    return Parent()->IsAppRegionDraggable();
  }
  return false;
}

void BaseView::AddChild(BaseView* child) {
  AddChild(child, child_count());

#ifdef ENABLE_ACCESSIBILITY
  MarkRebuildSemanticsTree();
#endif
}

void BaseView::AddChild(BaseView* child, int index) {
  if (static_cast<size_t>(index) > child_count() || index < 0) {
    FML_DCHECK(false) << "AddChild failure: Index out of bounds.";
    return;
  }
  BaseView* parent = child->parent_;
  if (parent) {
    FML_DCHECK(parent != this);
    parent->RemoveChild(child);
  }

  child->parent_ = this;

  FML_DCHECK(render_object());
  RenderObject* before_child = nullptr;
  if (static_cast<size_t>(index) < child_count()) {
    before_child = children_[index]->render_object();
  }

  render_object()->AddChild(child->render_object(), before_child);
  children_.insert(children_.begin() + index, child);

  DirtyChildrenPaintingOrder();

  // Notify insert after parent-child relationship is established and before
  // attach lifecycle notification.
  if (child->Is<NativeView>()) {
    static_cast<NativeView*>(child)->OnInsert(id(), GetChildIndex(child));
  }

  // If the current node is in a layout tree, its descendants must be in a
  // layout tree.
  if (InLayoutTree()) {
    child->SetInLayoutTree(true);
  }

  MarkNeedsLayout();

  if (attach_to_tree_) {
    child->OnAttachToTree();
  }
}

void BaseView::OnLayoutFinish(BaseView* view) {
  if (view == nullptr) {
    return;
  }
  if (view->Is<NativeView>()) {
    view->OnNodeReady();
  }
}

void BaseView::RemoveChild(BaseView* child) {
  std::vector<BaseView*>::iterator iter(
      std::find(children_.begin(), children_.end(), child));
  if (iter == children_.end()) {
    return;
  }

  if (attach_to_tree_ && !child->remove_temporarily_) {
    child->OnDetachFromTree();
  }

  // The child is removed from the layout tree, try to set its flag to false.
  // NOTE: if the child is a layout root candidate, it is still in layout tree.
  // See `SetInLayoutTree()`.
  if (InLayoutTree()) {
    child->SetInLayoutTree(false);
  }

  MarkNeedsLayout();

#ifdef ENABLE_ACCESSIBILITY
  MarkRebuildSemanticsTree();
#endif

  FML_DCHECK(render_object());
  render_object()->RemoveChild(child->render_object());
  child->parent_ = nullptr;
  children_.erase(iter);

  DirtyChildrenPaintingOrder();
}

void BaseView::RemoveChildTemporarily(BaseView* child) {
  child->remove_temporarily_ = true;
  RemoveChild(child);
  child->remove_temporarily_ = false;
}

void BaseView::DestroyAllChildren() {
  FML_DCHECK(render_object());
  for (const auto& child : children_) {
    if (attach_to_tree_) {
      child->OnDetachFromTree();
    }
    if (InLayoutTree()) {
      child->SetInLayoutTree(false);
    }
  }
  DestroyChildrenRecursively(this);
  children_.clear();

  DirtyChildrenPaintingOrder();
}

void BaseView::BringChildToFront(BaseView* child) {
  if (!children_.empty() && children_.back() == child) {
    return;
  }

  std::vector<BaseView*>::iterator iter(
      std::find(children_.begin(), children_.end(), child));
  if (iter == children_.end()) {
    return;
  }

  FML_DCHECK(render_object());
  render_object()->BringChildToFront(child->render_object());
  children_.erase(iter);
  children_.push_back(child);

  DirtyChildrenPaintingOrder();
}

Scrollable* BaseView::FindAncestorScrollableView(BaseView* child) {
  auto parent = child->Parent();
  while (parent) {
    if (parent->Is<Scrollable>()) {
      return static_cast<Scrollable*>(parent);
    }
    parent = parent->Parent();
  }
  return nullptr;
}

void BaseView::AddGestureRecognizer(
    std::unique_ptr<GestureRecognizer> recognizer) {
  gesture_recognizers_.push_back(std::move(recognizer));
}

void BaseView::RemoveGestureRecognizer(GestureRecognizer* recognizer) {
  if (!recognizer) {
    return;
  }
  for (auto it = gesture_recognizers_.begin(); it != gesture_recognizers_.end();
       it++) {
    if (it->get() == recognizer) {
      gesture_recognizers_.erase(it);
      return;
    }
  }
}

void BaseView::ClearGestureRecognizers() { gesture_recognizers_.clear(); }

int BaseView::GetChildIndex(BaseView* child) {
  auto it = std::find(children_.begin(), children_.end(), child);
  if (it != children_.end()) {
    return static_cast<int>(std::distance(children_.begin(), it));
  } else {
    return -1;
  }
}

float BaseView::GetTranslateZ() const {
  return render_object()->GetTranslateZ();
}

void BaseView::SetPaintingOrder(int value) {
  int old_painting_order = render_object()->GetPaintingOrder();
  render_object()->SetPaintingOrder(value);
  if (Parent() && old_painting_order != value) {
    Parent()->DirtyChildrenPaintingOrder();
  }
}

int BaseView::GetPaintingOrder() const {
  return render_object()->GetPaintingOrder();
}

bool BaseView::IsDescendant(BaseView* a_view) const {
  BaseView* to_check = a_view;
  while (to_check != nullptr) {
    if (to_check == this) {
      return true;
    }
    to_check = to_check->Parent();
  }
  return false;
}

void BaseView::SetRepaintBoundary(bool repaint_boundary) {
  render_object()->SetRepaintBoundary(repaint_boundary);
}

void BaseView::SetX(float x) {
  if (left_ == x) {
    return;
  }
  TransitionTo(ClayAnimationPropertyType::kLeft, x);
}

void BaseView::SetY(float y) {
  if (top_ == y) {
    return;
  }
  TransitionTo(ClayAnimationPropertyType::kTop, y);
}

void BaseView::SetWidth(float width) {
  if (width_ == width) {
    return;
  }
  TransitionTo(ClayAnimationPropertyType::kWidth, width);
}

void BaseView::SetHeight(float height) {
  if (height_ == height) {
    return;
  }
  TransitionTo(ClayAnimationPropertyType::kHeight, height);
}

void BaseView::SetBound(float left, float top, float width, float height) {
  FloatRect old_bounds = GetBounds();
  // TODO: Maybe we can use Class 'AutoReset' to save the original value of the
  // variable and set a new value. When the life cycle ends, restore the
  // original value of the variable.
  // https://source.chromium.org/chromium/chromium/src/+/main:base/auto_reset.h;l=25;bpv=1;bpt=1?q=AutoReset&ss=chromium%2Fchromium%2Fsrc
  ignore_size_change_checks_ = true;
  SetX(left);
  SetWidth(width);

  const bool should_couple_top_with_height_transition =
      top_ != top && height_ != height && IsTransitionAnimationReady() &&
      transition_mgr_ &&
      transition_mgr_->Enabled(ClayAnimationPropertyType::kHeight) &&
      !transition_mgr_->Enabled(ClayAnimationPropertyType::kTop);

  if (should_couple_top_with_height_transition) {
    if (transition_mgr_->TransitionTo(ClayAnimationPropertyType::kHeight,
                                      height)) {
      transition_mgr_->TransitionWithTiming(ClayAnimationPropertyType::kTop,
                                            top,
                                            ClayAnimationPropertyType::kHeight);
    } else {
      SetProperty(ClayAnimationPropertyType::kTop, top, false);
      SetProperty(ClayAnimationPropertyType::kHeight, height, false);
    }
  } else {
    SetY(top);
    SetHeight(height);
  }
  ignore_size_change_checks_ = false;
  NotifyBoundChangeIfNeeded(old_bounds);

  OnLayoutChange();
  // Force a request to repaint a frame.
  Invalidate();
#ifdef ENABLE_ACCESSIBILITY
  // TODO: Check if this logic can be moved to `NotifyBoundChangeIfNeeded `.
  FloatRect new_bounds = GetBounds();
  if (old_bounds != new_bounds) {
    // It's ok if this view is not acessible, because `FlushSemantics` will
    // check again.
    if (auto* semantics_owner = GetSemanticsOwner()) {
      semantics_owner->AddDirtySemanticsForDescendants(this);
    }
  }
#endif
}

void BaseView::SetOpacity(float opacity) {
  float old_opacity;
  GetProperty(ClayAnimationPropertyType::kOpacity, old_opacity);

  if (old_opacity == opacity) {
    return;
  }
  TransitionTo(ClayAnimationPropertyType::kOpacity, opacity);
}

float BaseView::Opacity() {
  float res;
  GetProperty(ClayAnimationPropertyType::kOpacity, res);
  return res;
}

void BaseView::SetOverflowWithMask(uint8_t mask, int overflow) {
  int new_val = overflow_;
  if (overflow == static_cast<int>(ClayOverflowType::kVisible)) {
    new_val |= mask;
  } else {
    new_val &= ~mask;
  }
  SetOverflow(new_val);
}

void BaseView::SetOverflow(int overflow) {
  if (overflow_ == overflow) {
    return;
  }
  overflow_ = overflow;
  render_object()->SetOverflow(overflow_);
}

void BaseView::SetBorder(const BordersData& data) {
  render_object()->SetBorders(data);
  OnBorderChanged(data);
}

void BaseView::OnBorderChanged(const BordersData& data) {
  // FIXME: The logic of checking content size has been broken.
  FloatRect old_rect(PaddingLeft() + BorderLeft(), PaddingTop() + BorderTop(),
                     Width() - PaddingRight() - BorderRight(),
                     Height() - PaddingBottom() - BorderBottom());
  render_object()->MarkNeedsPaint();
  FloatRect new_rect(PaddingLeft() + BorderLeft(), PaddingTop() + BorderTop(),
                     Width() - PaddingRight() - BorderRight(),
                     Height() - PaddingBottom() - BorderBottom());
  if (new_rect.width() != old_rect.width() ||
      new_rect.height() != old_rect.height()) {
    OnContentSizeChanged(old_rect, new_rect);
  }
}

void BaseView::AppendShadow(const Shadow& shadow) {
  render_object()->SetShadow(shadow);
}

void BaseView::SetShadows(std::vector<Shadow>&& shadows) {
  render_object()->SetShadows(std::move(shadows));
}

void BaseView::SetBorderStyle(std::vector<Side> sides,
                              std::vector<BorderStyleType> styles) {
  FML_DCHECK(sides.size() == styles.size());
  auto& border = render_object()->MutableBorder();
  for (size_t i = 0; i < sides.size(); i++) {
    Side side = sides[i];
    BorderStyleType style = styles[i];
    switch (side) {
      case Side::kTop:
        border.style_top_ = style;
        break;
      case Side::kRight:
        border.style_right_ = style;
        break;
      case Side::kBottom:
        border.style_bottom_ = style;
        break;
      case Side::kLeft:
        border.style_left_ = style;
        break;
      case Side::kAll:
        border.style_top_ = style;
        border.style_right_ = style;
        border.style_bottom_ = style;
        border.style_left_ = style;
        break;
      default:
        break;
    }
  }
  OnBorderChanged(border);
}

void BaseView::SetBorderWidth(std::vector<Side> sides,
                              std::vector<float> widths) {
  FML_DCHECK(sides.size() == widths.size());
  auto& border = render_object()->MutableBorder();
  for (size_t i = 0; i < sides.size(); i++) {
    Side side = sides[i];
    float width = widths[i];
    switch (side) {
      case Side::kTop:
        border.width_top_ = width;
        break;
      case Side::kRight:
        border.width_right_ = width;
        break;
      case Side::kBottom:
        border.width_bottom_ = width;
        break;
      case Side::kLeft:
        border.width_left_ = width;
        break;
      case Side::kAll:
        border.width_top_ = width;
        border.width_right_ = width;
        border.width_bottom_ = width;
        border.width_left_ = width;
        break;
      default:
        break;
    }
  }
  OnBorderChanged(border);
  OnLayoutChange();
}

void BaseView::SetBorderColor(std::vector<Side> sides,
                              std::vector<uint32_t> colors) {
  FML_DCHECK(sides.size() == colors.size());
  auto& border = render_object()->MutableBorder();
  for (size_t i = 0; i < sides.size(); i++) {
    Side side = sides[i];
    uint32_t color = colors[i];
    switch (side) {
      case Side::kTop:
        border.color_top_ = color;
        break;
      case Side::kRight:
        border.color_right_ = color;
        break;
      case Side::kBottom:
        border.color_bottom_ = color;
        break;
      case Side::kLeft:
        border.color_left_ = color;
        break;
      case Side::kAll:
        border.color_top_ = color;
        border.color_right_ = color;
        border.color_bottom_ = color;
        border.color_left_ = color;
        break;
      default:
        break;
    }
  }
  OnBorderChanged(border);
}

void BaseView::SetBorderRadius(const FloatSize& left_top,
                               const FloatSize& right_top,
                               const FloatSize& right_bottom,
                               const FloatSize& left_bottom) {
  auto& border = render_object()->MutableBorder();
  border.radius_x_top_left_length_.SetValue(left_top.width());
  border.radius_y_top_left_length_.SetValue(left_top.height());
  border.radius_x_top_right_length_.SetValue(right_top.width());
  border.radius_y_top_right_length_.SetValue(right_top.height());
  border.radius_x_bottom_right_length_.SetValue(right_bottom.width());
  border.radius_y_bottom_right_length_.SetValue(right_bottom.height());
  border.radius_x_bottom_left_length_.SetValue(left_bottom.width());
  border.radius_y_bottom_left_length_.SetValue(left_bottom.height());
  border.UpdateRadius(width_, height_);
  OnBorderChanged(border);
  OnLayoutChange();
}

void BaseView::SetBorderRadius(size_t index, const std::vector<Length>& array) {
  auto& border = render_object()->MutableBorder();
  if (array.size() == 0 && index == 4) {  // reset all
    for (size_t i = 0; i < index; i++) {
      border.SetRadius(i, Length(), Length());
    }

  } else if (array.size() == 0 && index < 4) {  // reset single
    border.SetRadius(index, Length(), Length());

  } else if (array.size() == 8 && index == 4) {  // update all
    for (size_t i = 0; i < index; i++) {
      Length length_x = array.at(i * 2);
      Length length_y = array.at(i * 2 + 1);
      border.SetRadius(i, length_x, length_y);
    }

  } else if (array.size() == 2 && index < 4) {  // update single
    Length length_x = array.at(0);
    Length length_y = array.at(1);
    border.SetRadius(index, length_x, length_y);

  } else {
    FML_DCHECK(false);
    return;
  }

  border.UpdateRadius(width_, height_);
  OnBorderChanged(border);
}

void BaseView::SetOutline(const OutlineData& outline) {
  render_object()->SetOutline(outline);
}

void BaseView::SetOutlineStyle(const BorderStyleType style) {
  auto& outline = render_object()->MutableOutline();
  outline.style_ = style;
  render_object()->SetOutline(outline);
}

void BaseView::SetOutlineWidth(int width) {
  auto& outline = render_object()->MutableOutline();
  outline.width_ = width;
  render_object()->SetOutline(outline);
}

void BaseView::SetOutlineOffset(int offset) {
  auto& outline = render_object()->MutableOutline();
  outline.offset_ = offset;
  render_object()->SetOutline(outline);
}

void BaseView::SetOutlineColor(unsigned int color) {
  auto& outline = render_object()->MutableOutline();
  outline.color_ = color;
  render_object()->SetOutline(outline);
}

void BaseView::SetBackground(const BackgroundData& background) {
  Color old_color;
  GetProperty(ClayAnimationPropertyType::kBackgroundColor, old_color);

  render_object()->SetBackgroundData(background);

  if (old_color != background.background_color) {
    // Restore old value and ensure that transition animation will be triggered.
    render_object()->SetBackgroundColor(old_color);
    SetBackgroundColor(background.background_color);
  }

  FML_DCHECK(page_view());

  bg_image_loader_token_++;
  for (size_t i = 0; i < background.background_images.size(); i++) {
    const BackgroundImageData& image = background.background_images[i];

    if (image.type == ClayBackgroundImageType::kUrl && !image.src_str.empty()) {
      LoadBackgroundOrMaskImage(image.src_str, i);
    } else if (image.type == ClayBackgroundImageType::kLinearGradient) {
      std::optional<Gradient> gradient;
      if (!image.gradient_str.empty()) {
        // Create Gradient by |gradient_str|:
        gradient = Gradient::Create(image.gradient_str);
      } else {
        // Create Gradient by |gradient_data|:
        gradient = Gradient::CreateLinear(image.gradient_data);
      }

      if (gradient.has_value()) {
        render_object()->SetBackgroundImage(i, *gradient);
      }
    }
  }
}

void BaseView::SetBackgroundColor(const Color& color) {
  if (OnBackgroundProperty(BaseView::BackgroundUpdate{
          BaseView::BackgroundPropType::kColor, color})) {
    return;
  }
  if (render_object()->HasBackground() &&
      render_object()->Background().background_color == color) {
    return;
  }
  TransitionTo(ClayAnimationPropertyType::kBackgroundColor, color);
}

void BaseView::SetCursor(const std::vector<std::string>& vec) {
  cursor_ = std::make_unique<MouseCursor>(vec);
  auto* mouse_region_manager = page_view_->mouse_region_manager();
  if (mouse_region_manager) {
    mouse_region_manager->AddCursorHolder(this);
    // Force cursor update if mouse is currently hovering over this view
    if (is_mouse_hover_) {
      mouse_region_manager->ForceUpdateCursor();
    }
  }
}

MouseCursor* BaseView::GetMouseCursor() { return cursor_.get(); }

void BaseView::SetCursor(const clay::Value::Array& array) {
  // array like:
  // 0(uint32_t)
  // array(clay::Value::Array,size=3): path/to/image(string), x(int),y(int)
  // 1(uint32_t)
  // keyword(string)

  cursor_ = std::make_unique<MouseCursor>();

  bool is_parse_error = false;

#define ADD_CHECK_OVERFLOW   \
  if (++i >= array.size()) { \
    is_parse_error = true;   \
    break;                   \
  }

  for (size_t i = 0; i < array.size();) {
    uint32_t int_type = utils::GetUint(array[i], -1);
    ClayCursorType type = static_cast<ClayCursorType>(int_type);
    switch (type) {
      case ClayCursorType::kUrl: {
        ADD_CHECK_OVERFLOW;
        const auto& url_array = utils::GetArray(array[i]);

        if (url_array.size() != 3) {
          is_parse_error = true;
          break;
        }

        // parse path
        auto path = utils::GetCString(url_array[0], "default");
        auto path_type = CursorTypeUtil::ParseCursorType(path);

        // parse x
        auto x = utils::GetInt(url_array[1], 0);

        // parse y
        auto y = utils::GetInt(url_array[2], 0);

        cursor_->AddCursor({path_type, path, x, y});

        ++i;
        break;
      }
      case ClayCursorType::kKeyword: {
        ADD_CHECK_OVERFLOW;
        auto str = utils::GetCString(array[i], "default");
        auto type = CursorTypeUtil::ParseCursorType(str);
        cursor_->AddCursor({type, str});

        ++i;
        break;
      }
      default: {
        is_parse_error = true;
        break;
      }
    }

    if (is_parse_error) {
      FML_DLOG(ERROR)
          << "error cursor format! Use default cursor or first cursor";
      cursor_->AddCursor(Cursor(CursorTypes::kBasic, "default"));
      break;
    }
  }
#undef ADD_CHECK_OVERFLOW

  // in case of `cursor:none`
  if (array.size() == 0) {
    cursor_->AddCursor(Cursor(CursorTypes::kNone, "none"));
  }

  auto* mouse_region_manager = page_view_->mouse_region_manager();
  if (mouse_region_manager) {
    mouse_region_manager->AddCursorHolder(this);
    // Force cursor update if mouse is currently hovering over this view
    if (is_mouse_hover_) {
      mouse_region_manager->ForceUpdateCursor();
    }
  }
}

// Add Lamé curve in the target quadrant to the target path.
static void AddLameCurveToPath(GrPath& path, float rx, float ry, float cx,
                               float cy, float ex, float ey, int quadrant) {
  double cosI, sinI, x, y;
  float fx = (quadrant == 1 || quadrant == 4) ? 1 : -1;
  float fy = (quadrant == 1 || quadrant == 2) ? 1 : -1;
  for (float i = (M_PI / 2 * (quadrant - 1)); i < M_PI / 2 * quadrant;
       i += 0.01) {
    // abs for cos and sin
    cosI = fx * cos(i);
    sinI = fy * sin(i);
    x = fx * rx * pow(cosI, 2 / ex) + cx;
    y = fy * ry * pow(sinI, 2 / ey) + cy;
    if (i == 0) {
      PATH_MOVE_TO(path, x, y);
    } else {
      PATH_LINE_TO(path, x, y);
    }
  }
}

void BaseView::SetClipOffsetPath(const clay::Value::Array& array,
                                 bool is_clip_path) {
  auto shape_type = static_cast<ClayBasicShapeType>(utils::GetInt(array[0]));
  auto& path_data = is_clip_path ? clip_path_data_ : offset_path_data_;
  switch (shape_type) {
    case ClayBasicShapeType::kCircle: {
      ClayPlatformLength radius_length, x_pos_length, y_pos_length;
      bool result = true;
      result &= utils::TryGetPlatformLength(array, 1, radius_length);
      result &= utils::TryGetPlatformLength(array, 3, x_pos_length);
      result &= utils::TryGetPlatformLength(array, 5, y_pos_length);
      if (!result) {
        FML_DLOG(ERROR) << "Setting clip-path: error Circle format!";
        return;
      }
      if (!path_data.has_value()) {
        path_data = ClipPathData{};
      }
      path_data->params.emplace_back(radius_length);
      path_data->params.emplace_back(radius_length);
      path_data->params.emplace_back(x_pos_length);
      path_data->params.emplace_back(y_pos_length);
      path_data->clip_type = ClipPathData::ClipType::kCircle;
      break;
    }
    case ClayBasicShapeType::kEllipse: {
      ClayPlatformLength x_length, y_length, x_pos_length, y_pos_length;
      bool result = true;
      result &= utils::TryGetPlatformLength(array, 1, x_length);
      result &= utils::TryGetPlatformLength(array, 3, y_length);
      result &= utils::TryGetPlatformLength(array, 5, x_pos_length);
      result &= utils::TryGetPlatformLength(array, 7, y_pos_length);
      if (!result) {
        FML_DLOG(ERROR) << "Setting clip-path: error Ellipse format!";
        return;
      }
      if (!path_data.has_value()) {
        path_data = ClipPathData{};
      }
      path_data->params.emplace_back(x_length);
      path_data->params.emplace_back(y_length);
      path_data->params.emplace_back(x_pos_length);
      path_data->params.emplace_back(y_pos_length);
      path_data->clip_type = ClipPathData::ClipType::kEllipse;
      break;
    }
    case ClayBasicShapeType::kPath: {
      std::string path_str = utils::GetCString(array[1]);
      GrPath path;
      if (!PathBuilder::ParsePathString(path_str.c_str(), &path)) {
        FML_DLOG(ERROR) << "Setting clip-path: error Path format!";
        return;
      }
      auto scaled_density =
          page_view()->GetPixelRatio<kPixelTypeLogical, kPixelTypeClay>();
      skity::Matrix density_scale =
          skity::Matrix::Scale(scaled_density, scaled_density);
      PATH_TRANSFORM(path, density_scale);
      path_data.reset();
      if (is_clip_path) {
        render_object()->SetClipPath(path);
      } else {
        render_object()->SetOffsetPath(path);
      }
      break;
    }
    case ClayBasicShapeType::kSuperEllipse: {
      ClayPlatformLength radius_x, radius_y, center_x, center_y;
      ClayPlatformLength exponent_x, exponent_y;
      bool result = true;
      result &= utils::TryGetPlatformLength(array, 1, radius_x);
      result &= utils::TryGetPlatformLength(array, 3, radius_y);
      result &= utils::TryGetPlatformLength(array, 7, center_x);
      result &= utils::TryGetPlatformLength(array, 9, center_y);
      if (!result) {
        FML_DLOG(ERROR) << "Setting clip-path: error inset format!";
        return;
      }
      if (!path_data.has_value()) {
        path_data = ClipPathData{};
      }
      path_data->params.emplace_back(radius_x);
      path_data->params.emplace_back(radius_y);
      path_data->params.emplace_back(center_x);
      path_data->params.emplace_back(center_y);
      path_data->exponents.emplace_back(array[5].GetDouble());
      path_data->exponents.emplace_back(array[6].GetDouble());
      path_data->clip_type = ClipPathData::ClipType::kSuperEllipse;
      break;
    }
    case ClayBasicShapeType::kInset: {
      // begin handling inset function params
      // clang-format off
      // params arrange as:
      // first 8 fields are inset:
      // | top | lengthUnit | right | lengthUnit | bottom | lengthUnit | left | lengthUnit |
      // clang-format on
      ClayPlatformLength top_length, right_length, bottom_length, left_length;
      bool result = true;
      result &= utils::TryGetPlatformLength(array, 1, top_length);
      result &= utils::TryGetPlatformLength(array, 3, right_length);
      result &= utils::TryGetPlatformLength(array, 5, bottom_length);
      result &= utils::TryGetPlatformLength(array, 7, left_length);

      if (!result) {
        FML_DLOG(ERROR) << "Setting clip-path: error inset format!";
        return;
      }
      if (!path_data.has_value()) {
        path_data = ClipPathData{};
      }
      path_data->params.emplace_back(top_length);
      path_data->params.emplace_back(right_length);
      path_data->params.emplace_back(bottom_length);
      path_data->params.emplace_back(left_length);
      // rounded corner's radius value start at index 9
      int radius_offset = 9;
      if (array.size() == 9) {
        // Rect only has first 8 params.
        path_data->corner_type = ClipPathData::CornerType::kCornerRect;
      }
      if (array.size() == 27) {
        // super-ellipse has two more fields for exponents before border-radius.
        // | exponent-x | exponent-y | border-radius ...|
        path_data->exponents.emplace_back(array[9].GetDouble());
        path_data->exponents.emplace_back(array[10].GetDouble());
        radius_offset = 11;
        path_data->corner_type =
            ClipPathData::CornerType::kCornerSuperElliptical;
      }
      if (array.size() == 27 || array.size() == 25) {
        // clang-format off
        // inset with rounded and lame corner has 16 radius fields:
        // |   top-left-x  | lengthUnit |   top-left-y   | lengthUnit |
        // top-right-x  | lengthUnit | |  top-right-y  | lengthUnit |
        // bottom-right-x | lengthUnit | bottom-right-y | lengthUnit | |
        // bottom-left-x | lengthUnit |  bottom-left-y | lengthUnit |
        // clang-format on
        ClipPathData::BorderRadius radius_top_left, radius_top_right,
            radius_bottom_right, radius_bottom_left;
        utils::TryGetPlatformLength(array, 0 + radius_offset,
                                    radius_top_left.x);
        utils::TryGetPlatformLength(array, 2 + radius_offset,
                                    radius_top_left.y);
        utils::TryGetPlatformLength(array, 4 + radius_offset,
                                    radius_top_right.x);
        utils::TryGetPlatformLength(array, 6 + radius_offset,
                                    radius_top_right.y);
        utils::TryGetPlatformLength(array, 8 + radius_offset,
                                    radius_bottom_right.x);
        utils::TryGetPlatformLength(array, 12 + radius_offset,
                                    radius_bottom_right.y);
        utils::TryGetPlatformLength(array, 12 + radius_offset,
                                    radius_bottom_left.x);
        utils::TryGetPlatformLength(array, 14 + radius_offset,
                                    radius_bottom_left.y);

        path_data->radius.emplace_back(radius_top_left);
        path_data->radius.emplace_back(radius_top_right);
        path_data->radius.emplace_back(radius_bottom_right);
        path_data->radius.emplace_back(radius_bottom_left);
        if (array.size() == 25) {
          path_data->corner_type = ClipPathData::CornerType::kCornerRounded;
        }
      } else {
        // invalid params array
        return;
      }
      path_data->clip_type = ClipPathData::ClipType::kInset;
      break;
    }
    case ClayBasicShapeType::kUnknown: {
      FML_DLOG(ERROR) << "Setting clip-path: Unknown type.";
      break;
    }
  }
}

void BaseView::DrawClipPath(bool is_clip_path) {
#define SET_PATH(path)                    \
  if (is_clip_path) {                     \
    render_object()->SetClipPath(path);   \
  } else {                                \
    render_object()->SetOffsetPath(path); \
  }

  auto& path_data = is_clip_path ? clip_path_data_ : offset_path_data_;
  switch (path_data->clip_type) {
    case ClipPathData::ClipType::kCircle: {
      if (path_data->params.size() != 4) {
        break;
      }
      float x_len = utils::ResolvePlatformLength(path_data->params[0], width_);
      float y_len = utils::ResolvePlatformLength(path_data->params[1], height_);
      float radius = std::min(x_len, y_len);
      float x_pos = utils::ResolvePlatformLength(path_data->params[2], width_);
      float y_pos = utils::ResolvePlatformLength(path_data->params[3], height_);
      FloatRoundedRect rrect;
      rrect.SetOval({x_pos - radius, y_pos - radius, 2 * radius, 2 * radius});
      SET_PATH(rrect);
      break;
    }
    case ClipPathData::ClipType::kEllipse: {
      if (path_data->params.size() != 4) {
        break;
      }
      float x_len = utils::ResolvePlatformLength(path_data->params[0], width_);
      float y_len = utils::ResolvePlatformLength(path_data->params[1], height_);
      float x_pos = utils::ResolvePlatformLength(path_data->params[2], width_);
      float y_pos = utils::ResolvePlatformLength(path_data->params[3], height_);
      FloatRoundedRect rrect;
      rrect.SetOval({x_pos - x_len, y_pos - y_len, 2 * x_len, 2 * y_len});
      SET_PATH(rrect);
      break;
    }
    case ClipPathData::ClipType::kPath:
      break;
    case ClipPathData::ClipType::kSuperEllipse: {
      if (path_data->params.size() != 4 || path_data->exponents.size() != 2) {
        break;
      }
      GrPath path;
      float rx = utils::ResolvePlatformLength(path_data->params[0], width_);
      float ry = utils::ResolvePlatformLength(path_data->params[1], height_);
      float cx = utils::ResolvePlatformLength(path_data->params[2], width_);
      float cy = utils::ResolvePlatformLength(path_data->params[3], height_);
      float ex = path_data->exponents[0];
      float ey = path_data->exponents[1];
      if (rx == 0 && ry == 0) {
        // Nothing to do, keep the path empty.
        break;
      }
      // Add super-ellipse to the target position cx, cy
      for (int i = 1; i <= 4; i++) {
        AddLameCurveToPath(path, rx, ry, cx, cy, ex, ey, i);
      }
      SET_PATH(path);
      break;
    }
    case ClipPathData::ClipType::kInset: {
      if (path_data->params.size() != 4) {
        break;
      }
      double top = utils::ResolvePlatformLength(path_data->params[0], width_);
      double right = utils::ResolvePlatformLength(path_data->params[1], width_);
      double bottom =
          utils::ResolvePlatformLength(path_data->params[2], width_);
      double left = utils::ResolvePlatformLength(path_data->params[3], width_);

      // Adjust inset value, values are proportionally reduce the inset effect
      // to 100% if the pair of insets in either dimension add up to more than
      // the side length.
      double v_inset = top + bottom;
      double h_inset = left + right;
      if (v_inset != 0 && (v_inset > height_)) {
        double s = height_ / v_inset;
        top *= s;
        bottom *= s;
      }
      if (h_inset != 0 && (h_inset > width_)) {
        double s = width_ / h_inset;
        left *= s;
        right *= s;
      }

      float radius_top_left_x =
          utils::ResolvePlatformLength(path_data->radius[0].x, width_);
      float radius_top_left_y =
          utils::ResolvePlatformLength(path_data->radius[0].y, width_);
      float radius_top_right_x =
          utils::ResolvePlatformLength(path_data->radius[1].x, width_);
      float radius_top_right_y =
          utils::ResolvePlatformLength(path_data->radius[1].y, width_);
      float radius_bottom_right_x =
          utils::ResolvePlatformLength(path_data->radius[2].x, width_);
      float radius_bottom_right_y =
          utils::ResolvePlatformLength(path_data->radius[2].y, width_);
      float radius_bottom_left_x =
          utils::ResolvePlatformLength(path_data->radius[3].x, width_);
      float radius_bottom_left_y =
          utils::ResolvePlatformLength(path_data->radius[3].y, width_);
      switch (path_data->corner_type) {
        case ClipPathData::CornerType::kCornerRect: {
          GrPath path;
          skity::Rect rect = skity::Rect::MakeLTRB(left, top, width_ - right,
                                                   height_ - bottom);
          PATH_ADD_RECT(path, rect);
          SET_PATH(path);
          break;
        }
        case ClipPathData::CornerType::kCornerRounded: {
          GrPath path;
          skity::Vec2 radii[4] = {
              {radius_top_left_x, radius_top_left_y},
              {radius_top_right_x, radius_top_right_y},
              {radius_bottom_right_x, radius_bottom_right_y},
              {radius_bottom_left_x, radius_bottom_left_y}};
          skity::RRect rrect;
          rrect.SetRectRadii(
              skity::Rect::MakeXYWH(left, top, width_ - left - right,
                                    height_ - top - bottom),
              radii);
          PATH_ADD_RRECT(path, rrect);
          SET_PATH(path);
          break;
        }
        case ClipPathData::CornerType::kCornerSuperElliptical: {
          if (path_data->exponents.size() != 2) {
            break;
          }
          GrPath path;
          std::vector<float> radius = {
              radius_top_left_x,     radius_top_left_y,
              radius_top_right_x,    radius_top_right_y,
              radius_bottom_right_x, radius_bottom_right_y,
              radius_bottom_left_x,  radius_bottom_left_y};
          // Add ellipse to the target rect at cx cy
          // bottom-right corner
          float rx = radius[4];
          float ry = radius[5];
          float cx = width_ - right - rx;
          float cy = width_ - bottom - ry;
          float ex = path_data->exponents[0];
          float ey = path_data->exponents[1];
          AddLameCurveToPath(path, rx, ry, cx, cy, ex, ey, 1);

          // bottom-left corner
          rx = radius[6];
          rx = radius[7];
          cx = left + rx;
          cy = width_ - bottom - ry;
          AddLameCurveToPath(path, rx, ry, cx, cy, ex, ey, 2);

          // top-left corner
          rx = radius[0];
          ry = radius[1];
          cx = left + rx;
          cy = top + ry;
          AddLameCurveToPath(path, rx, ry, cx, cy, ex, ey, 3);

          // top-right corner
          rx = radius[2];
          ry = radius[3];
          cx = width_ - right - rx;
          cy = top + ry;
          AddLameCurveToPath(path, rx, ry, cx, cy, ex, ey, 4);
          SET_PATH(path);
          break;
        }
        case ClipPathData::CornerType::kUnknown:
          break;
      }
      break;
    }
    case ClipPathData::ClipType::kUnknown: {
      FML_DLOG(ERROR) << "Setting clip-path: Unknown type.";
      break;
    }
  }
#undef SET_PATH
}

void BaseView::ClearClipPath() {
  clip_path_data_.reset();
  render_object()->ClearClipPath();
}

void BaseView::ClearOffsetPath() {
  offset_path_data_.reset();
  render_object()->ClearOffsetPath();
}

void BaseView::SetFilter(const clay::Value::Array& array) {
  // Clear the old filter state.
  ClearFilter();
  auto size = array.size();
  if (size == 0) {
    return;
  }
  bool decoded_by_custom =
      page_view() && page_view()->GetCustomFilterDecoder() &&
      page_view()->GetCustomFilterDecoder()->Decode(array, this);
  if (!decoded_by_custom && size >= 2) {
    // array like: [type, amount, unit]
    int int_type = utils::GetInt(array[0]);
    ClayFilterType type = static_cast<ClayFilterType>(int_type);
    float amount = utils::GetDouble(array[1]);
    switch (type) {
      case ClayFilterType::kGrayScale: {
        // grayscale is a value between 0 and 1 (100%)
        amount = std::clamp(amount, 0.f, 1.f);
        if (amount > 0.f) {
          render_object()->AppendGrayScale(amount);
        }
        break;
      }
      case ClayFilterType::kBlur: {
        amount = std::max(amount, 0.f);
        if (amount > 0.f) {
          render_object()->SetBlurRadius(amount);
        }
        break;
      }
      case ClayFilterType::kNone: {
        break;
      }
      default: {
        FML_DLOG(ERROR) << "Unsupported filter type";
        break;
      }
    }
  }
}

void BaseView::ClearFilter() {
  FML_DCHECK(render_object());
  render_object()->ClearFilter();
}

void BaseView::SetImageRendering(ClayImageRendering image_rendering) {
  // ClayImageRendering::kCrispEdges is not supported, behave as kAuto.
  render_object()->SetImageFilterMode(
      image_rendering == ClayImageRendering::kPixelated ? FilterMode::kNearest
                                                        : FilterMode::kLinear);
}

void BaseView::SetID(int id) {
  id_ = id;
  if (render_object()) {
    render_object()->SetID(id);
  }
}

void BaseView::LoadBackgroundOrMaskImage(const std::string& uri, size_t index,
                                         bool background) {
  FML_DCHECK(page_view());

#ifndef ENABLE_SKITY
  page_view_->GetImageResourceFetcher()->FetchImageAsync(
      uri,
      [self = weak_factory_.GetWeakPtr(), uri, index, background,
       bg_image_loader_token = bg_image_loader_token_,
       mask_image_loader_token = mask_image_loader_token_](
          std::unique_ptr<ImageResource> resource, bool hit_cache) {
        if (!resource || !self) {
          if (!resource && background && self) {
            self->NotifyBgImageLoadStatus(
                false, {"errMsg", "url", "lynx_categorized_code", "error_code"},
                "resource load fail", uri, 0, 0);
          }
          return;
        }
        if (self->GetCurrentImageLoaderToken() != bg_image_loader_token &&
            self->GetCurrentMaskImageLoaderToken() != mask_image_loader_token) {
          return;
        }

        if (background) {
          self->NotifyBgImageLoadStatus(true, {"height", "width", "url"},
                                        resource->GetHeight(),
                                        resource->GetWidth(), uri);
          self->render_object()->SetBackgroundImage(index, std::move(resource));
        } else {
          self->render_object()->SetMaskImage(index, std::move(resource));
        }
      },
      page_view_->UseTextureBackend(),
      background && page_view_->DeferredImageDecode(),
      page_view_->ImageDecodeWithPriority());
#else
  page_view_->GetImageResourceFetcher()->FetchImage(
      uri, false,
      [self = weak_factory_.GetWeakPtr(), uri, index, background](
          std::unique_ptr<BaseImageInstance> image_instance, bool hit_cache) {
        if (!image_instance || !self) {
          if (!image_instance && background && self) {
            self->NotifyBgImageLoadStatus(
                false, {"errMsg", "url", "lynx_categorized_code", "error_code"},
                "resource load fail", uri, 0, 0);
          }
          return;
        }
        auto image = image_instance->GetImage();
        if (!image) {
          if (background && self) {
            self->NotifyBgImageLoadStatus(
                false, {"errMsg", "url", "lynx_categorized_code", "error_code"},
                "resource load fail", uri, 0, 0);
          }
          return;
        }
        image_instance->SetAnimationFrameCallback([self]() {
          if (!self) {
            return;
          }
          self->render_object()->MarkNeedsPaint();
        });
        if (background) {
          self->NotifyBgImageLoadStatus(true, {"height", "width", "url"},
                                        image_instance->GetHeight(),
                                        image_instance->GetWidth(), uri);
          self->render_object()->SetBackgroundImage(index,
                                                    std::move(image_instance));
        } else {
          self->render_object()->SetMaskImage(index, std::move(image_instance));
        }
      });
#endif  // ENABLE_SKITY
}

void BaseView::SetBackgroundImage(const clay::Value::Array& array) {
  if (OnBackgroundProperty(BaseView::BackgroundUpdate{
          BaseView::BackgroundPropType::kImage, &array})) {
    return;
  }
  // The image count
  render_object()->ResizeBackground(array.size() / 2);
  if (array.size() == 0) {
    return;
  }

  bg_image_loader_token_++;
  for (size_t i = 0; i < array.size(); i = i + 2) {
    const auto& type =
        static_cast<ClayBackgroundImageType>(utils::GetUint(array[i]));

    if (type == ClayBackgroundImageType::kUrl) {  // uri
      const auto& uri = utils::GetCString(array[i + 1]);
      LoadBackgroundOrMaskImage(uri, i / 2);
    } else {
      std::optional<Gradient> (*f)(const clay::Value::Array&) = nullptr;
      if (type == ClayBackgroundImageType::kLinearGradient) {
        f = Gradient::CreateLinear;
      } else if (type == ClayBackgroundImageType::kRadialGradient) {
        f = Gradient::CreateRadial;
      } else if (type == ClayBackgroundImageType::kConicGradient) {
        f = Gradient::CreateConic;
      }
      if (!f) {
        FML_DLOG(WARNING) << "Unknow ClayBackgroundImageType:"
                          << static_cast<int>(type);
        continue;
      }
      const auto& array_param = utils::GetArray(array[i + 1]);
      FML_DCHECK(array_param.size() > 0);
      auto g = f(array_param);
      if (g.has_value()) {
        render_object()->SetBackgroundImage(i / 2, *g);
      } else {
        FML_DLOG(WARNING) << "Create gradient failed";
      }
    }
  }
}

void BaseView::SetBackgroundClip(const clay::Value::Array& array) {
  if (OnBackgroundProperty(BaseView::BackgroundUpdate{
          BaseView::BackgroundPropType::kClip, &array})) {
    return;
  }
  std::vector<ClayBackgroundClipType> clips(array.size());
  for (size_t i = 0; i < array.size(); i++) {
    clips[i] = static_cast<ClayBackgroundClipType>(utils::GetInt(array[i]));
  }
  render_object()->SetBackgroundClip(clips);
}

void BaseView::SetBackgroundOrigin(const clay::Value::Array& array) {
  if (OnBackgroundProperty(BaseView::BackgroundUpdate{
          BaseView::BackgroundPropType::kOrigin, &array})) {
    return;
  }
  std::vector<ClayBackgroundOriginType> origins(array.size());
  for (size_t i = 0; i < array.size(); i++) {
    origins[i] = static_cast<ClayBackgroundOriginType>(utils::GetInt(array[i]));
  }
  render_object()->SetBackgroundOrigin(origins);
}

void BaseView::SetBackgroundPosition(
    const std::vector<BackgroundPosition>& positions) {
  if (OnBackgroundProperty(BaseView::BackgroundUpdate{
          BaseView::BackgroundPropType::kPosition, &positions})) {
    return;
  }
  render_object()->SetBackgroundPosition(positions);
}

void BaseView::SetBackgroundRepeat(const clay::Value::Array& array) {
  if (OnBackgroundProperty(BaseView::BackgroundUpdate{
          BaseView::BackgroundPropType::kRepeat, &array})) {
    return;
  }
  std::vector<ClayBackgroundRepeatType> repeats(array.size());
  for (size_t i = 0; i < array.size(); i++) {
    repeats[i] = static_cast<ClayBackgroundRepeatType>(utils::GetInt(array[i]));
  }
  render_object()->SetBackgroundRepeat(repeats);
}

void BaseView::SetBackgroundSize(const std::vector<BackgroundSize>& sizes) {
  if (OnBackgroundProperty(BaseView::BackgroundUpdate{
          BaseView::BackgroundPropType::kSize, &sizes})) {
    return;
  }
  render_object()->SetBackgroundSize(sizes);
}

void BaseView::SetMaskImage(const clay::Value::Array& array) {
  if (array.size() == 0) {
    return;
  }

  render_object()->ResizeMask(array.size() / 2);
  for (size_t i = 0; i < array.size(); i = i + 2) {
    const auto& type = static_cast<ClayMaskImageType>(utils::GetUint(array[i]));

    if (type == ClayMaskImageType::kUrl) {  // uri
      const auto& uri = utils::GetCString(array[i + 1]);
      LoadBackgroundOrMaskImage(uri, i / 2, false);
    } else {
      std::optional<Gradient> (*f)(const clay::Value::Array&) = nullptr;
      if (type == ClayMaskImageType::kLinearGradient) {
        f = Gradient::CreateLinear;
      } else if (type == ClayMaskImageType::kRadialGradient) {
        f = Gradient::CreateRadial;
      } else if (type == ClayMaskImageType::kConicGradient) {
        f = Gradient::CreateConic;
      }
      if (!f) {
        FML_DLOG(WARNING) << "Unknow ClayMaskImageType:"
                          << static_cast<int>(type);
        continue;
      }
      const auto& array_param = utils::GetArray(array[i + 1]);
      FML_DCHECK(array_param.size() > 0);
      auto g = f(array_param);
      if (g.has_value()) {
        render_object()->SetMaskImage(i / 2, *g);
      } else {
        FML_DLOG(WARNING) << "Create gradient failed";
      }
    }
  }
}

void BaseView::SetMaskPosition(const std::vector<MaskPosition>& positions) {
  render_object()->SetMaskPosition(positions);
}

void BaseView::SetMaskRepeat(const clay::Value::Array& array) {
  std::vector<ClayMaskRepeatType> repeats(array.size());
  for (size_t i = 0; i < array.size(); i++) {
    repeats[i] = static_cast<ClayMaskRepeatType>(utils::GetInt(array[i]));
  }
  render_object()->SetMaskRepeat(repeats);
}

void BaseView::SetMaskOrigin(const clay::Value::Array& array) {
  std::vector<ClayMaskOriginType> origins(array.size());
  for (size_t i = 0; i < array.size(); i++) {
    origins[i] = static_cast<ClayMaskOriginType>(utils::GetInt(array[i]));
  }
  render_object()->SetMaskOrigin(origins);
}

void BaseView::SetMaskSize(const std::vector<MaskSize>& mask_sizes) {
  render_object()->SetMaskSize(mask_sizes);
}

void BaseView::SetMaskClip(const clay::Value::Array& array) {
  std::vector<ClayMaskClipType> clips(array.size());
  for (size_t i = 0; i < array.size(); i++) {
    clips[i] = static_cast<ClayMaskClipType>(utils::GetInt(array[i]));
  }
  render_object()->SetMaskClip(clips);
}

TransitionManager* BaseView::TransitionMgr() {
  if (!transition_mgr_) {
    transition_mgr_ =
        std::make_unique<TransitionManager>(GetAnimationMutator());
    transition_mgr_->SetEventHandler(GetAnimationMutator());
  }
  return transition_mgr_.get();
}

void BaseView::TransitionTo(ClayAnimationPropertyType type, float value) {
  switch (type) {
    case ClayAnimationPropertyType::kLeft:
    case ClayAnimationPropertyType::kTop:
    case ClayAnimationPropertyType::kWidth:
    case ClayAnimationPropertyType::kHeight:
      // Start property transition animation if it is enabled
      if (IsTransitionAnimationReady() && TransitionMgr()->Enabled(type) &&
          TransitionMgr()->TransitionTo(type, value)) {
        Invalidate();
        return;
      }
      SetProperty(type, value, false);
      break;
    case ClayAnimationPropertyType::kOpacity:
      if (IsTransitionAnimationReady() && TransitionMgr()->Enabled(type) &&
          TransitionMgr()->TransitionTo(type, value)) {
        UpdateTransitionRasterAnimation(type);
        return;
      }
      SetProperty(type, value, false);
      break;
    default:
      FML_DLOG(ERROR) << "BaseView::TransitionTo with unsupported type: "
                      << static_cast<int>(type);
      break;
  }
}

void BaseView::TransitionTo(ClayAnimationPropertyType type,
                            const Color& value) {
  switch (type) {
    case ClayAnimationPropertyType::kBackgroundColor:
      if (IsTransitionAnimationReady() && TransitionMgr()->Enabled(type) &&
          TransitionMgr()->TransitionTo(type, value)) {
        UpdateTransitionRasterAnimation(type);
        return;
      }
      SetProperty(type, value, false);
      break;
    default:
      FML_DLOG(ERROR) << "BaseView::TransitionTo with unsupported type: "
                      << static_cast<int>(type);
      break;
  }
}

void BaseView::TransitionTo(ClayAnimationPropertyType type,
                            const TransformOperations& value) {
  switch (type) {
    case ClayAnimationPropertyType::kTransform:
      if (IsTransitionAnimationReady() && TransitionMgr()->Enabled(type) &&
          TransitionMgr()->TransitionTo(type, value)) {
        UpdateTransitionRasterAnimation(type);
        return;
      }
      SetProperty(type, value, false);
      break;
    default:
      FML_DLOG(ERROR) << "BaseView::TransitionTo with unsupported type: "
                      << static_cast<int>(type);
      break;
  }
}

void BaseView::SetProperty(ClayAnimationPropertyType type, float value,
                           bool skip_update_for_raster_animation) {
  skip_update_for_raster_animation =
      skip_update_for_raster_animation && IsRasterAnimationEnabled();
  FloatRect old_bounds = GetBounds();
  switch (type) {
    case ClayAnimationPropertyType::kLeft:
      left_ = value;
      render_object()->SetLeft(value);
      break;
    case ClayAnimationPropertyType::kTop:
      top_ = value;
      render_object()->SetTop(value);
      break;
    case ClayAnimationPropertyType::kWidth:
      width_ = value;
      render_object()->SetWidth(value);
      UpdateRenderObjectTransformOrigin();
      break;
    case ClayAnimationPropertyType::kHeight:
      height_ = value;
      render_object()->SetHeight(value);
      UpdateRenderObjectTransformOrigin();
      break;
    case ClayAnimationPropertyType::kOpacity:
      render_object()->SetOpacity(value, skip_update_for_raster_animation);
      break;
    default:
      FML_DLOG(ERROR) << "BaseView::SetProperty with unsupported type: "
                      << static_cast<int>(type);
      break;
  }
  NotifyBoundChangeIfNeeded(old_bounds);
}

void BaseView::NotifyBoundChangeIfNeeded(const FloatRect& old_bounds) {
  if (ignore_size_change_checks_) {
    return;
  }
  FloatRect bounds = GetBounds();
  if (old_bounds.size() != bounds.size()) {
    OnContentSizeChanged(old_bounds, bounds);
  }
  if (old_bounds != bounds) {
    OnBoundsChanged(old_bounds, bounds);
  }
}

void BaseView::UpdateChildrenBounds() {
  if (needs_layout_updated_) {
    return;
  }
  needs_layout_updated_ = true;
  for (BaseView* child : children_) {
    child->UpdateChildrenBounds();
  }
}

void BaseView::SetProperty(ClayAnimationPropertyType type, const Color& value,
                           bool skip_update_for_raster_animation) {
  skip_update_for_raster_animation =
      skip_update_for_raster_animation && IsRasterAnimationEnabled();
  switch (type) {
    case ClayAnimationPropertyType::kBackgroundColor:
      render_object()->SetBackgroundColor(value,
                                          skip_update_for_raster_animation);
      break;
    case ClayAnimationPropertyType::kColor:
      // it's used for compositor animation.
      // just run through here.
      break;
    default:
      FML_DLOG(WARNING) << "BaseView::SetProperty with unsupported type: "
                        << static_cast<int>(type);
      break;
  }
}

void BaseView::SetOffsetDistance(float distance) {
  render_object()->SetOffsetDistance(distance);
}

void BaseView::SetOffsetRotate(float rotate) {
  render_object()->SetOffsetRotate(rotate);
}

void BaseView::SetTransformOperations(const TransformOperations& value,
                                      bool is_from_animation) {
  transform_ops_ = value;
#ifdef ENABLE_ACCESSIBILITY
  Transform old_transform = GetTransform();
#endif
  float old_translate_z = render_object()->GetTranslateZ();
  if (post_translation_.has_value()) {
    TransformOperations transform;
    transform.AppendTranslate(post_translation_->x(), post_translation_->y(),
                              0);
    transform.Append(value);
    render_object()->SetTransformOperations(transform, is_from_animation);
  } else {
    render_object()->SetTransformOperations(transform_ops_, is_from_animation);
  }
#ifdef ENABLE_ACCESSIBILITY
  Transform new_transform = GetTransform();
  // Changes of transform will affect current semantics_node's and all
  // descendants' accumulated transform to its real parent_node.
  if (!old_transform.ApproximatelyEqual(new_transform)) {
    if (auto* semantics_owner = GetSemanticsOwner()) {
      semantics_owner->AddDirtySemanticsForDescendants(this);
    }
  }
#endif
  if (Parent() &&
      std::abs(render_object()->GetTranslateZ() - old_translate_z) > 1e-6) {
    Parent()->DirtyChildrenPaintingOrder();
  }
}

void BaseView::SetProperty(ClayAnimationPropertyType type,
                           const FilterOperations& value) {
  switch (type) {
    case ClayAnimationPropertyType::kFilter:
      SetFilterOperations(value);
      break;
    default: {
      FML_LOG(ERROR) << "BaseView::SetProperty with unsupported type: "
                     << static_cast<int>(type);
      break;
    }
  }
}

void BaseView::SetFilterOperations(const FilterOperations& value) {
  color_matrix_ops_ = value;
  render_object_->ClearFilter();
  // apply will calculate the blur radius.
  std::array<float, 20> m = value.Apply();
  std::vector<float> v(m.data(), m.data() + 20);
  render_object_->CombineColorFilter(v);
  render_object_->SetBlurRadius(value.GetBlurRadius());
}

void BaseView::SetProperty(ClayAnimationPropertyType type,
                           const TransformOperations& value,
                           bool skip_update_for_raster_animation) {
  skip_update_for_raster_animation =
      skip_update_for_raster_animation && IsRasterAnimationEnabled();
  switch (type)
  case ClayAnimationPropertyType::kTransform: {
    SetTransformOperations(value, skip_update_for_raster_animation);
    break;
    default: {
      FML_DLOG(ERROR) << "BaseView::SetProperty with unsupported type: "
                      << static_cast<int>(type);
      break;
    }
  }
}

void BaseView::SetBoxShadowOperations(const BoxShadowOperations& value) {
  box_shadow_ops_ = value;
  render_object_->SetShadows(value.apply());
}

void BaseView::GetProperty(ClayAnimationPropertyType type, float& value) {
  switch (type) {
    case ClayAnimationPropertyType::kLeft:
      value = left_;
      break;
    case ClayAnimationPropertyType::kTop:
      value = top_;
      break;
    case ClayAnimationPropertyType::kWidth:
      value = width_;
      break;
    case ClayAnimationPropertyType::kHeight:
      value = height_;
      break;
    case ClayAnimationPropertyType::kOpacity:
      value = render_object()->HasOpacity() ? render_object()->Opacity() : 1.0f;
      break;
    default:
      FML_DLOG(ERROR) << "BaseView::GetProperty with unsupported type: "
                      << static_cast<int>(type);
      break;
  }
}

void BaseView::GetProperty(ClayAnimationPropertyType type, Color& value) {
  switch (type) {
    case ClayAnimationPropertyType::kBackgroundColor:
      if (render_object()->HasBackground()) {
        value = render_object()->Background().background_color;
      } else {
        value = Color();
      }
      break;
    default:
      FML_DLOG(ERROR) << "BaseView::GetProperty with unsupported type: "
                      << static_cast<int>(type);
      break;
  }
}

void BaseView::GetProperty(ClayAnimationPropertyType type,
                           TransformOperations& value) {
  switch (type) {
    case ClayAnimationPropertyType::kTransform:
      value = transform_ops_;
      break;
    default:
      FML_DLOG(ERROR) << "BaseView::GetProperty with unsupported type: "
                      << static_cast<int>(type);
      break;
  }
}

void BaseView::GetProperty(ClayAnimationPropertyType type,
                           FilterOperations& value) {
  switch (type) {
    case ClayAnimationPropertyType::kFilter:
      value = color_matrix_ops_;
      break;
    default:
      FML_LOG(ERROR) << "BaseView::GetProperty with unsupported type: "
                     << static_cast<int>(type);
      break;
  }
}

AnimationHandler* BaseView::GetAnimationHandler() {
  return page_view_->GetAnimationHandler();
}

const KeyframesMap* BaseView::GetKeyframesMap(
    const std::string& animation_name) {
  return page_view_->GetKeyframesMap(animation_name);
}

KeyframesManager* BaseView::KeyframesMgr() {
  if (!keyframes_mgr_) {
    keyframes_mgr_ = std::make_unique<KeyframesManager>(GetAnimationMutator());
    keyframes_mgr_->SetEventHandler(GetAnimationMutator());
  }
  return keyframes_mgr_.get();
}

void BaseView::SetAnimation(const std::vector<AnimationData>& data) {
  if (!data.empty()) {
    SetRepaintBoundary(true);
  }
  animation_ = std::make_optional<std::vector<AnimationData>>(data);
}

void BaseView::OnAnimationNodeReady() {
  if (!animation_.has_value()) {
    return;
  }

  auto result = KeyframesMgr()->UpdateData(*animation_);
  if (result.data_has_changed) {
    // Animations may not play without this
    page_view()->RequestPaint();

    UpdateKeyframesRasterAnimation();
  }
  if (result.state_has_changed) {
    Invalidate();
  }
}

void BaseView::OnTransitionAnimationReady() {
  transition_animation_ready_ = true;
}

void BaseView::SetAnimation(const clay::Value::Array& array) {
  std::vector<AnimationData> animations(array.size());
  for (size_t i = 0; i < array.size(); i++) {
    const auto& arr = utils::GetArray(array[i]);
    if (arr.size() == 0) {
      continue;
    }
    FML_DCHECK(arr.size() == 13);
    int idx = 0;
    animations[i].name = utils::GetCString(arr[idx++]);
    animations[i].duration = utils::GetDouble(arr[idx++]);
    animations[i].timing_func.timing_func =
        static_cast<ClayTimingFunctionType>(utils::GetInt(arr[idx++]));
    animations[i].timing_func.steps_type =
        static_cast<ClayStepsType>(utils::GetInt(arr[idx++]));
    animations[i].timing_func.x1 = utils::GetDouble(arr[idx++]);
    animations[i].timing_func.y1 = utils::GetDouble(arr[idx++]);
    animations[i].timing_func.x2 = utils::GetDouble(arr[idx++]);
    animations[i].timing_func.y2 = utils::GetDouble(arr[idx++]);
    animations[i].delay = utils::GetDouble(arr[idx++]);
    animations[i].iteration_count = utils::GetInt(arr[idx++]) - 1;
    animations[i].direction =
        static_cast<ClayAnimationDirectionType>(utils::GetInt(arr[idx++]));
    animations[i].fill_mode =
        static_cast<ClayAnimationFillModeType>(utils::GetInt(arr[idx++]));
    animations[i].play_state =
        static_cast<ClayAnimationPlayStateType>(utils::GetInt(arr[idx++]));
  }
  SetAnimation(animations);
}

void BaseView::AppendTransition(const TransitionData& data) {
  SetRepaintBoundary(true);
  TransitionMgr()->AppendData(data);
}

void BaseView::SetTransition(const std::vector<TransitionData>& data) {
  if (!data.empty()) {
    SetRepaintBoundary(true);
  }
  TransitionMgr()->UpdateData(data);
}

void BaseView::SetTransition(const clay::Value::Array& array) {
  if (array.size() != 0) {
    SetRepaintBoundary(true);
  }
  std::vector<TransitionData> transitions(array.size());
  for (size_t i = 0; i < array.size(); i++) {
    const auto& arr = utils::GetArray(array[i]);
    if (arr.size() == 0) {
      continue;
    }
    int idx = 0;
    transitions[i].property =
        static_cast<ClayAnimationPropertyType>(utils::GetInt(arr[idx++]));
    transitions[i].duration = utils::GetDouble(arr[idx++]);
    transitions[i].timing_func.timing_func =
        static_cast<ClayTimingFunctionType>(utils::GetInt(arr[idx++]));
    transitions[i].timing_func.steps_type =
        static_cast<ClayStepsType>(utils::GetInt(arr[idx++]));
    transitions[i].timing_func.x1 = utils::GetDouble(arr[idx++]);
    transitions[i].timing_func.y1 = utils::GetDouble(arr[idx++]);
    transitions[i].timing_func.x2 = utils::GetDouble(arr[idx++]);
    transitions[i].timing_func.y2 = utils::GetDouble(arr[idx++]);
    transitions[i].delay = utils::GetDouble(arr[idx++]);
  }
  TransitionMgr()->UpdateData(transitions);
}

void BaseView::SetTransform(const TransformOperations& ops,
                            const FloatPoint& origin) {
  SetTransformOrigin(
      std::make_optional<TransformOrigin>(origin.x(), origin.y()));

  TransformOperations old_ops;
  GetProperty(ClayAnimationPropertyType::kTransform, old_ops);
  constexpr float tolerance = 0.001;
  if (old_ops.ApproximatelyEqual(ops, tolerance)) {
    if (TransitionMgr()->IsAnimationRunning(
            ClayAnimationPropertyType::kTransform)) {
      TransitionMgr()->CancelAnimator(ClayAnimationPropertyType::kTransform);
    }
    SetProperty(ClayAnimationPropertyType::kTransform, ops, false);
    return;
  }
  TransitionTo(ClayAnimationPropertyType::kTransform, ops);
}

void BaseView::SetTransform(const std::vector<TransformRaw>& transform_raw) {
  transform_raw_ = transform_raw;
  auto ops = clay::TransformOperations(*transform_raw_, width_, height_);
  TransformOperations old_ops;
  GetProperty(ClayAnimationPropertyType::kTransform, old_ops);
  constexpr float tolerance = 0.001;
  if (old_ops.ApproximatelyEqual(ops, tolerance)) {
    if (TransitionMgr()->IsAnimationRunning(
            ClayAnimationPropertyType::kTransform)) {
      TransitionMgr()->CancelAnimator(ClayAnimationPropertyType::kTransform);
    }
    SetProperty(ClayAnimationPropertyType::kTransform, ops, false);
    return;
  }
  TransitionTo(ClayAnimationPropertyType::kTransform, ops);
}

void BaseView::SetTransformOrigin(std::optional<TransformOrigin> origin) {
  transform_origin_ = std::move(origin);
  UpdateRenderObjectTransformOrigin();
}

void BaseView::SetTransformOrigin(const std::vector<Length>& array) {
  auto transform_origin = std::make_optional<TransformOrigin>();
  if (array.size() == 0) {
    transform_origin->Reset();
  } else if (array.size() >= 1) {
    transform_origin->SetX(array[0]);
    if (array.size() == 2) {
      transform_origin->SetY(array[1]);
    }
  }
  SetTransformOrigin(std::move(transform_origin));
}

void BaseView::SetPerspective(const clay::Value::Array& array) {
  float perspective = 0;
  // CAUTION: scale and CAMERA_DISTANCE_NORMALIZATION_MULTIPLIER will affect
  // final result seriously.
  float scale = 1;
  double value = utils::GetInt(array[0]);
  int type = utils::GetInt(array[1]);
  if (array.size() > 1 && type != PLATFORM_PERSPECTIVE_UNIT_DEFAULT) {
    if (type == PLATFORM_PERSPECTIVE_UNIT_NUMBER) {
      perspective = (float)(value * scale * scale *
                            CAMERA_DISTANCE_NORMALIZATION_MULTIPLIER);
    } else {
      if (type == PLATFORM_PERSPECTIVE_UNIT_VW ||
          type == PLATFORM_PERSPECTIVE_UNIT_VH) {
        perspective = type == PLATFORM_PERSPECTIVE_UNIT_VW
                          ? (float)(value / 100.f * width_)
                          : (float)(value / 100.f * height_);
      } else {
        perspective = (float)value;
      }
      perspective =
          perspective * scale * CAMERA_DISTANCE_NORMALIZATION_MULTIPLIER;
    }
  } else {
    int max_length = std::max(width_, height_);
    perspective = max_length * scale *
                  CAMERA_DISTANCE_NORMALIZATION_MULTIPLIER *
                  DEFAULT_PERSPECTIVE_FACTOR;
  }
  if (perspective != perspective_value_) {
    perspective_value_ = perspective;
    render_object()->SetPerspective(perspective_value_.value());
  }
}

void BaseView::SetConsumeSlideEventDirection(const clay::Value::Array& array) {
  size_t array_size = array.size();
  consume_slide_event_ranges_.resize(array_size, {0, 0});
  for (size_t i = 0; i < array_size; i++) {
    if (!array[i].IsArray()) {
      continue;
    }
    const auto& angles = utils::GetArray(array[i]);
    if (angles.size() != 2) {
      continue;
    }
    consume_slide_event_ranges_[i] = {utils::GetNum(angles[0]),
                                      utils::GetNum(angles[1])};
  }
}

bool BaseView::ConsumeSlideEvent(float angle) {
  for (const auto& range : consume_slide_event_ranges_) {
    if (angle >= range.first && angle <= range.second) {
      return true;
    }
  }
  return false;
}

Transform BaseView::GetTransform() const {
  if (render_object()->HasTransform()) {
    return render_object()->GetTransform();
  } else {
    return Transform();
  }
}

FloatPoint BaseView::GetTransformOrigin() const {
  if (transform_origin_.has_value()) {
    return transform_origin_->GetValue(width_, height_);
  }
  return FloatPoint(width_ / 2.0f, height_ / 2.0f);
}

void BaseView::SetPaddings(float padding_left, float padding_top,
                           float padding_right, float padding_bottom) {
  FloatRect old_rect(PaddingLeft() + BorderLeft(), PaddingTop() + BorderTop(),
                     Width() - PaddingRight() - BorderRight(),
                     Height() - PaddingBottom() - BorderBottom());
  if (padding_left_ != padding_left) {
    padding_left_ = padding_left;
    render_object()->SetPaddingLeft(padding_left_);
  }

  if (padding_top_ != padding_top) {
    padding_top_ = padding_top;
    render_object()->SetPaddingTop(padding_top_);
  }

  if (padding_right_ != padding_right) {
    padding_right_ = padding_right;
    render_object()->SetPaddingRight(padding_right_);
  }

  if (padding_bottom_ != padding_bottom) {
    padding_bottom_ = padding_bottom;
    render_object()->SetPaddingBottom(padding_bottom_);
  }
  FloatRect new_rect(PaddingLeft() + BorderLeft(), PaddingTop() + BorderTop(),
                     Width() - PaddingRight() - BorderRight(),
                     Height() - PaddingBottom() - BorderBottom());
  if (new_rect.width() != old_rect.width() ||
      new_rect.height() != old_rect.height()) {
    OnContentSizeChanged(old_rect, new_rect);
  }
  OnLayoutChange();
}

void BaseView::SetMargins(float margin_left, float margin_top,
                          float margin_right, float margin_bottom) {
  margin_left_ = margin_left;
  render_object()->SetMarginLeft(margin_left);
  margin_top_ = margin_top;
  render_object()->SetMarginTop(margin_top);
  margin_right_ = margin_right;
  render_object()->SetMarginRight(margin_right);
  margin_bottom_ = margin_bottom;
  render_object()->SetMarginBottom(margin_bottom);
}

float BaseView::LeftWithScroll() const {
  if (render_object()) {
    return left_ - render_object()->ScrollLeft();
  }
  return 0.f;
}

float BaseView::TopWithScroll() const {
  if (render_object()) {
    return top_ - render_object()->ScrollTop();
  }
  return 0.f;
}

float BaseView::ContentInsetLeft() const {
  return BorderLeft() + PaddingLeft();
}

float BaseView::ContentInsetTop() const { return BorderTop() + PaddingTop(); }

FloatPoint BaseView::AbsoluteLocationWithScroll() const {
  BaseView* parent = Parent();
  float left = Left();
  float top = Top();
  auto self = this;
  while (parent) {
    if (self->IsIndependentSubViewTree()) {
      break;
    } else {
      self = parent;
      left += parent->LeftWithScroll();
      top += parent->TopWithScroll();
      parent = parent->Parent();
    }
  }

  return FloatPoint(left, top);
}

void BaseView::OnViewPostionUpdate(FloatPoint scroll_offset) {
  scroll_offset += FloatPoint(LeftWithScroll(), TopWithScroll());
  for (auto child : GetChildren()) {
    child->OnViewPostionUpdate(scroll_offset);
  }
}

float BaseView::BorderLeft() const {
  if (render_object()) {
    return render_object()->BorderLeft();
  }
  return 0.f;
}

float BaseView::BorderTop() const {
  if (render_object()) {
    return render_object()->BorderTop();
  }
  return 0.f;
}

float BaseView::BorderRight() const {
  if (render_object()) {
    return render_object()->BorderRight();
  }
  return 0.f;
}

float BaseView::BorderBottom() const {
  if (render_object()) {
    return render_object()->BorderBottom();
  }
  return 0.f;
}

FocusManager* BaseView::GetParentFocusManager() {
  if (Parent()) {
    return Parent()->GetFocusManager();
  }
  return nullptr;
}

void BaseView::FocusHasChanged(bool focused, bool is_leaf) {
  if (is_leaf) {
    OnFocusChanged(focused);
    page_view()->SendEvent(
        GetCallbackId(),
        focused ? event_attr::kEventFocus : event_attr::kEventBlur,
        std::vector<std::string>{});
    if (!IsFocusFence()) {
      // This is only used to update pseudo styles. Skip `focus-isolate` nodes
      // for performance
      if (page_view()->GetEventDelegate()) {
        page_view()->GetEventDelegate()->OnFocusChanged(GetCallbackId(),
                                                        focused);
      }
    }

    if (focused) {
      ScrollToVisible();
    }
  }
}

void BaseView::SetHasDefaultFocusRing(bool has_focus_ring) {
  if (page_view()->DefaultFocusRingEnabled()) {
    render_object()->SetHasDefaultFocusRing(has_focus_ring);
  }
}

FloatRect BaseView::CalcFocusRect() const {
  FloatPoint point(left_, top_);
  BaseView* parent = Parent();
  while (parent && !parent->IsFocusScope()) {
    FloatPoint offset = parent->GetScrollOffset();
    point.MoveBy(-offset);

    point.Move(parent->Left(), parent->Top());
    parent = parent->Parent();
  }

  return FloatRect(point.x(), point.y(), width_, height_);
}

bool BaseView::DispatchKeyEventOnFocusNode(const KeyEvent* event) {
  return OnKeyEvent(event);
}

FloatRect BaseView::GetContentVisibleRect() {
  FloatPoint offset = GetScrollOffset();
  return FloatRect(offset.x(), offset.y(), ContentWidth(), ContentHeight());
}

FloatSize BaseView::GetThicknessOffset() {
  return FloatSize(PaddingLeft() + BorderLeft(), PaddingTop() + BorderTop());
}

void BaseView::SetInLayoutTree(bool in_layout_tree) {
  if (IsLayoutRootCandidate()) {
    // If the current node is a layout root candidate, it is already in a
    // layout tree.
    in_layout_tree = true;
  }
  if (in_layout_tree == in_layout_tree_) {
    return;
  }
  in_layout_tree_ = in_layout_tree;
  for (BaseView* child : children_) {
    child->SetInLayoutTree(in_layout_tree);
  }
}

void BaseView::MarkNeedsLayout(BaseView* view_to_layout) {
  if (ShouldIgnoreLayoutRequest()) {
    return;
  }

  if (!InLayoutTree() || !attach_to_tree()) {
    return;
  }

  if (needs_layout_) {
    return;
  }
  needs_layout_ = true;

  // Only the actual layout root will handle the layout request.
  // So a subview will go through its ancestors until a Layout Root is found.
  if (IsLayoutRootCandidate()) {
    page_view()->GetLayoutController()->AddNeedLayout(this->GetWeakPtr());
    page_view()->RequestPaint();
  } else if (Parent()) {
    Parent()->MarkNeedsLayout(view_to_layout == nullptr ? this
                                                        : view_to_layout);
  }
}

void BaseView::Layout(LayoutContext* context) {
  needs_layout_ = false;
  OnLayout(context);
}

void BaseView::OnLayout(LayoutContext* context) {
  // During the layout process of a page containing a list component,
  // child elements may be inserted, causing a crash when iterating
  // over std::vector.
  for (size_t i = 0; i < children_.size(); i++) {
    auto child = children_[i];
    if (child->NeedsLayout()) {
      child->Layout(context);
    }
  }
}

void BaseView::OnContentSizeChanged(const FloatRect& old_rect,
                                    const FloatRect& new_rect) {
  if (render_object()->HasBorder()) {
    auto& border = render_object()->MutableBorder();
    border.UpdateRadius(width_, height_);
    render_object()->MarkNeedsPaint();
  }
  if (transform_raw_.has_value()) {  // update transform percent values
    if (transition_mgr_ && transition_mgr_->IsAnimationRunning(
                               ClayAnimationPropertyType::kTransform)) {
      transition_mgr_->UpdateAnimationValue(
          ClayAnimationPropertyType::kTransform,
          clay::TransformOperations(*transform_raw_, width_, height_));
    } else {
      SetProperty(ClayAnimationPropertyType::kTransform,
                  clay::TransformOperations(*transform_raw_, width_, height_),
                  false);
    }
  }

  if (clip_path_data_.has_value()) {
    DrawClipPath(true);
  }
  if (offset_path_data_.has_value()) {
    DrawClipPath(false);
  }

  UpdateChildrenBounds();
}

void BaseView::OnBoundsChanged(const FloatRect& old_bounds,
                               const FloatRect& new_bounds) {
  if (keyframes_mgr_ && old_bounds.size() != new_bounds.size()) {
    keyframes_mgr_->UpdateLayoutSize();
  }
  // TODO(wanchen): There may be a better way to deal with incomplete scrolling
  // of scroll-view
  if (parent_ != nullptr) {
    parent_->OnChildSizeChanged(this);
  }

  UpdateChildrenBounds();
}

bool BaseView::CanAcceptEvent() const {
  if (!Visible() || !IsInteractable()) {
    return false;
  }
  auto origin = GetTransformOrigin();
  Transform transform(GetTransformOps()
                          .Apply()
                          .matrix()
                          .PreTranslate(-origin.x(), -origin.y())
                          .PostTranslate(origin.x(), origin.y()));
  // If transform is not invertible, it means the view is not visible with
  // 2D transform.
  return transform.IsInvertible();
}

bool BaseView::HitTest(const PointerEvent& event, HitTestResult& result) {
  if (!CanAcceptEvent()) {
    return false;
  }

  FloatPoint point_by_self = GetPointBySelf(event.position);
  // Transform translate_transform = LocalToGlobalTransform();
  // SkVector4 sv(point_by_self.x(), point_by_self.y(), 0, 1);
  // SkVector4 sv_result = translate_transform.matrix() * sv;
  // FloatPoint point(sv_result.fData[0], sv_result.fData[1]);
  //  FML_CHECK(event.position == point);

  bool beyond_self = point_by_self.x() <= -hit_slop_left_ ||
                     point_by_self.x() > width_ + hit_slop_right_ ||
                     point_by_self.y() <= -hit_slop_top_ ||
                     point_by_self.y() > height_ + hit_slop_bottom_;
  if (beyond_self && !CanChildrenEscape()) {
    return false;
  }

  // we should consider the case of overflow visible
  bool clip_x = (GetOverflow() == CSSProperty::OVERFLOW_Y);
  bool clip_y = (GetOverflow() == CSSProperty::OVERFLOW_X);
  if (clip_x && (point_by_self.x() < -hit_slop_left_ ||
                 point_by_self.x() > width_ + hit_slop_right_)) {
    return false;
  }

  if (clip_y && (point_by_self.y() < -hit_slop_top_ ||
                 point_by_self.y() > height_ + hit_slop_bottom_)) {
    return false;
  }

  bool founded = HitTestChildren(event, result);

  if (beyond_self) {
    return founded;
  }
  should_pass_event_for_hittest_ = ShouldPassEventToNativeInherited(this);
  result.emplace_back(GetHitTestTargetWeakPtr());
  return true;
}

bool BaseView::HitTestChildren(const PointerEvent& event,
                               HitTestResult& result) {
  RebuildSortedChildrenIfNeeded();
  for (auto it = sorted_children_.rbegin(); it != sorted_children_.rend();
       ++it) {
    if ((*it)->HitTest(event, result)) {
      return true;  // if a view is hit, skip hitting its siblings
    }
  }
  return false;
}

void BaseView::HandleEvent(const PointerEvent& event) {
  if (event.type == PointerEvent::EventType::kDownEvent ||
      event.type == PointerEvent::EventType::kPanZoomStartEvent) {
    if (app_region_.compare(attr_value::kAppRegionDrag) == 0) {
      FloatPoint relative_position;
      auto top_view =
          GetTopViewToAcceptEvent(event.position, &relative_position);
      if (!top_view || (top_view && top_view->IsAppRegionDraggable())) {
        event_draggable_ = event;
        can_draggable_ = true;
      }
    }

    for (auto& recognizer : gesture_recognizers_) {
      recognizer->AddPointer(event);
    }
  } else if (event.type == PointerEvent::EventType::kMoveEvent ||
             event.type == PointerEvent::EventType::kPanZoomUpdateEvent) {
    if (can_draggable_) {
      float pixel_tolerance = FromLogical(8);
      float pixel_distance =
          (event.position - event_draggable_.position).distance();
      if (pixel_distance > pixel_tolerance) {
        page_view_->MoveWindow();
      }
    }
  } else {
    can_draggable_ = false;
  }
}

bool BaseView::HasDragGestureRecognizer(ScrollDirection direction) const {
  GestureRecognizerType type = (direction == ScrollDirection::kVertical)
                                   ? GestureRecognizerType::kVerticalDrag
                                   : GestureRecognizerType::kHorizontalDrag;
  for (auto& recognizer : gesture_recognizers_) {
    if (recognizer) {
      if (recognizer->getType() == type ||
          recognizer->getType() == GestureRecognizerType::kDragGesture) {
        return true;
      }
    }
  }
  return false;
}

bool BaseView::HasTapGestureRecognizer() const {
  for (auto& recognizer : gesture_recognizers_) {
    if (recognizer && recognizer->getType() == GestureRecognizerType::kTap) {
      return true;
    }
  }
  return false;
}

bool BaseView::HasLongPressGestureRecognizer() const {
  for (auto& recognizer : gesture_recognizers_) {
    if (recognizer &&
        recognizer->getType() == GestureRecognizerType::kLongPress) {
      return true;
    }
  }
  return false;
}

bool BaseView::HasTapEvent() const { return HasEvent(event_attr::kEventTap); }

bool BaseView::HasLongPressEvent() const {
  return HasEvent(event_attr::kEventLongPress) ||
         HasEvent(event_attr::kEventMouseLongPress);
}

void BaseView::DestroyChildrenRecursively(BaseView* view) {
  if (view == nullptr) {
    return;
  }
  if (view != this) {
    view->Destroy();
  }
  auto& children = view->GetChildren();
  auto iter = children.begin();
  while (iter != children.end()) {
    DestroyChildrenRecursively(*iter);
    // Remove child dangling pointer in parent.
    iter = children.erase(iter);
  }
  if (view != this) {
    delete view;
  }
}

void BaseView::OnDetachFromTree() {
  FML_DCHECK(attach_to_tree_) << "Node has already detached!";
  attach_to_tree_ = false;
  transition_animation_ready_ = false;
  for (auto& i : children_) {
    i->OnDetachFromTree();
  }
  OnFocusDetach();

  if (IsLayoutRootCandidate()) {
    page_view_->GetLayoutController()->RemoveDirtyNode(this->GetWeakPtr());
  }

  if (has_intersection_observer_) {
    page_view_->intersection_observer_manager()->NotifyTargetDetached(this);
  }
}

void BaseView::OnAttachToTree() {
  attach_to_tree_ = true;
  // The need_layout_ has been set before. Re-trigger it so that we can dirty
  // the ancestors and re-layout eventually.
  if (InLayoutTree() && needs_layout_) {
    needs_layout_ = false;
  }
  // TODO(wangchen): mark leaf node needs layout, could do better
  MarkNeedsLayout();

  for (auto& i : children_) {
    i->OnAttachToTree();
  }
  OnFocusAttach();

  if (has_intersection_observer_) {
    page_view_->intersection_observer_manager()->NotifyTargetAttached(this);
  }
}

void BaseView::Invalidate() {
  render_object()->MarkNeedsPaint();
#ifdef ENABLE_ACCESSIBILITY
  if (SemanticsOwner* owner = GetSemanticsOwner()) {
    owner->AddDirtySemanticsForDescendants(this);
  }
#endif
}

FloatRect BaseView::ContentBoundsInViewport() const {
  auto location = AbsoluteLocationWithScroll();
  return FloatRect(location.x() + BorderLeft() + PaddingLeft(),
                   location.y() + BorderTop() + PaddingTop(), ContentWidth(),
                   ContentHeight());
}

FloatRect BaseView::BoundsRelativeTo(BaseView* view) const {
  auto location = AbsoluteLocationWithScroll();
  if (view) {
    location = location - view->AbsoluteLocationWithScroll();
  }
  return FloatRect(location.x(), location.y(), Width(), Height());
}

BaseView* BaseView::GetTopViewToAcceptEvent(const FloatPoint& position,
                                            FloatPoint* relative_position) {
  FML_DCHECK(relative_position);
  if (!BaseView::CanAcceptEvent()) {
    return nullptr;
  }

  FloatPoint point_by_self = GetPointBySelf(position);
  bool is_point_inside = point_by_self.x() >= -hit_slop_left_ &&
                         point_by_self.x() <= width_ + hit_slop_right_ &&
                         point_by_self.y() >= -hit_slop_top_ &&
                         point_by_self.y() <= height_ + hit_slop_bottom_;
  if (!CanChildrenEscape() && !is_point_inside) {
    return nullptr;
  }

  BaseView* view = nullptr;

  RebuildSortedChildrenIfNeeded();
  for (auto it = sorted_children_.rbegin(); it != sorted_children_.rend();
       it++) {
    if ((*it)->IsIndependentSubViewTree()) {
      continue;
    }
    view = (*it)->GetTopViewToAcceptEvent(position, relative_position);
    if (view) {
      return view;
    }
  }

  // An internally created view (with an id < 0) cannot be the event target.
  if (is_point_inside && !IsAnonymousView()) {
    if (IsIndependentSubViewTree() && CanEventsPassThroughToViewsBehind()) {
      return nullptr;
    }
    *relative_position = point_by_self;
    return CanEventThrough().has_value() && CanEventThrough().value() ? nullptr
                                                                      : this;
  }
  return nullptr;
}

FloatPoint BaseView::GetPointBySelf(const FloatPoint& point_by_page) const {
  FloatPoint point = point_by_page;
  BaseView* parent = Parent();
  if (IsIndependentSubViewTree()) {
    parent = page_view();
  }
  if (parent) {
    point = parent->GetPointBySelf(point);
    FloatPoint offset = parent->GetScrollOffsetForPaint();
    point.MoveBy(offset);
  }
  point.Move(-Left(), -Top());
  if (sticky_) {
    point.Move(-sticky_.value().offset_x, -sticky_.value().offset_y);
  }

  auto origin = GetTransformOrigin();
  Transform transform(GetTransformOps()
                          .Apply()
                          .matrix()
                          .PreTranslate(-origin.x(), -origin.y())
                          .PostTranslate(origin.x(), origin.y()));
  if (!transform.IsIdentity()) {
    /*
     * transform direction:
     * forward: convert coordinates stored in BaseView to coordinates
     * displayed reverse: convert coordinates displayed to coordinates stored
     * in BaseView
     */
    transform.TransformPointReverse(&point);
  }
  return point;
}

Transform BaseView::LocalToGlobalTransform() const {
  FloatPoint point(left_, top_);
  BaseView* parent = Parent();
  if (IsIndependentSubViewTree()) {
    parent = page_view();
  }
  Transform transform = Transform(skity::Matrix());
  while (parent) {
    FloatPoint offset = parent->GetScrollOffset();
    point.MoveBy(-offset);

    point.Move(parent->Left(), parent->Top());
    parent = parent->Parent();
  }
  transform.Translate(point.x(), point.y());
  // TODO(wangchen): Consider the transform case
  return transform;
}

void BaseView::SetVisible(bool visible) {
  render_object()->SetVisible(visible);
}

bool BaseView::Visible() const { return render_object()->Visible(); }

void BaseView::SetAttribute(const char* attr, const clay::Value& value) {
  // NOTE: new attributes should be added to `HandleCommonAttribute`
  HandleCommonAttribute(attr, value);
}

bool BaseView::HandleCommonAttribute(const char* attr,
                                     const clay::Value& value) {
  auto kw = GetKeywordID(attr);
  switch (kw) {
    case KeywordID::kIdselector:
      id_selector_ = utils::GetCString(value);
      break;
    case KeywordID::kReactRef:
      ref_id_selector_ = utils::GetCString(value);
      break;
    case KeywordID::kItemKey:
      item_key_ = utils::GetCString(value);
      break;
    case KeywordID::kXAppRegion: {
      int app_region_type = utils::GetInt(value);
      if (app_region_type == app_region_value::kAppRegionDrag) {
        app_region_ = attr_value::kAppRegionDrag;
      } else if (app_region_type == app_region_value::kAppRegionNoDrag) {
        app_region_ = attr_value::kAppRegionNoDrag;
      }
    } break;
    case KeywordID::kFocusable:
      SetFocusable(attribute_utils::GetBool(value));
      break;
    case KeywordID::kIgnoreFocus: {
      auto ignore_focus = attribute_utils::GetBool(value);
      ignore_focus_ = ignore_focus;
    } break;
    case KeywordID::kIntersectionObservers: {
      auto* manager = page_view_->intersection_observer_manager();
      if (manager) {
        // first remove old observer
        manager->RemoveIntersectionObserver(this);
        const auto& observers = utils::GetArray(value);
        for (size_t i = 0; i < observers.size(); ++i) {
          const auto& map = utils::GetMap(observers[i]);
          auto observer =
              std::make_unique<IntersectionObserver>(manager, map, this);
          manager->AddObserver(std::move(observer));
          has_intersection_observer_ = true;
        }
      }
    } break;
    case KeywordID::kFocusIndex:
      SetFocusIndex(attribute_utils::GetPoint(value));
      break;
    case KeywordID::kNextFocusUp:
      SetNextFocusUp(utils::GetCString(value));
      break;
    case KeywordID::kNextFocusDown:
      SetNextFocusDown(utils::GetCString(value));
      break;
    case KeywordID::kNextFocusLeft:
      SetNextFocusLeft(utils::GetCString(value));
      break;
    case KeywordID::kNextFocusRight:
      SetNextFocusRight(utils::GetCString(value));
      break;
    case KeywordID::kNextFocusFallback:
      SetNextFocusFallback(utils::GetBool(value));
    case KeywordID::kUserInteractionEnabled:
      SetInteractable(utils::GetBool(value, true));
      break;
    case KeywordID::kBlockNativeEvent:
      SetBlockNativeEvent(utils::GetBool(value));
      break;
    case KeywordID::kConsumeSlideEvent:
      SetConsumeSlideEventDirection(utils::GetArray(value));
      break;
    case KeywordID::kEnableNewAnimator:
      SetEnableNewAnimator(utils::GetBool(value));
      break;
    case KeywordID::kName:
      name_ = utils::GetCString(value);
      break;
    case KeywordID::kDataset:
      data_set_ = CloneClayValue(value);
      break;
    case KeywordID::kEventThrough:
      event_through_ = utils::GetBool(value);
      if (event_through_) {
        auto task_runners = page_view_->GetTaskRunners();
        if (task_runners.GetPlatformTaskRunner() !=
            task_runners.GetUITaskRunner()) {
          FML_LOG(ERROR)
              << "event through is only supported in MOST_ON_UI thread mode";
        }
      }
      break;
    case KeywordID::kHitSlop:
      if (value.IsMap()) {
        const auto& map = value.GetMap();
        if (map.empty()) {
          return true;
        }
        auto& top = attribute_utils::GetMapItem(map, "top");
        if (top.IsString()) {
          hit_slop_top_ = attribute_utils::ToPxWithDisplayMetrics(
              utils::GetCString(top, ""), page_view());
        }
        auto& left = attribute_utils::GetMapItem(map, "left");
        if (left.IsString()) {
          hit_slop_left_ = attribute_utils::ToPxWithDisplayMetrics(
              utils::GetCString(left, ""), page_view());
        }
        auto& right = attribute_utils::GetMapItem(map, "right");
        if (right.IsString()) {
          hit_slop_right_ = attribute_utils::ToPxWithDisplayMetrics(
              utils::GetCString(right, ""), page_view());
        }
        auto& bottom = attribute_utils::GetMapItem(map, "bottom");
        if (bottom.IsString()) {
          hit_slop_bottom_ = attribute_utils::ToPxWithDisplayMetrics(
              utils::GetCString(bottom, ""), page_view());
        }
      } else if (value.IsString()) {
        std::string value_with_unit = attribute_utils::GetCString(value, "");
        float slop_value = attribute_utils::ToPxWithDisplayMetrics(
            value_with_unit, page_view());
        hit_slop_top_ = slop_value;
        hit_slop_left_ = slop_value;
        hit_slop_right_ = slop_value;
        hit_slop_bottom_ = slop_value;
      }
      break;
    default:
      // Handle css properties
      if (CSSProperty::SetAttribute(this, kw, value)) {
        return true;
      }
      // Handle exposure properties
      if (UpdateExposeAttrs(attr, value)) {
        return true;
      }
      FML_DLOG(ERROR) << "Setting attribute " << attr
                      << " not supported by component " << GetName();
      return false;
  }
  return true;
}

template <typename... Args>
void BaseView::NotifyBgImageLoadStatus(bool success,
                                       const std::vector<std::string>& keys,
                                       Args&&... args) {
  if (success && HasEvent(event_attr::kEventBgImageLoadSuccess)) {
    page_view()->SendEvent(id(), event_attr::kEventBgImageLoadSuccess, keys,
                           args...);
  } else if (!success && HasEvent(event_attr::kEventBgImageLoadError)) {
    page_view()->SendEvent(id(), event_attr::kEventBgImageLoadError, keys,
                           args...);
  }
}

bool BaseView::UpdateExposeAttrs(const char* attr, const clay::Value& value) {
  auto kw = GetKeywordID(attr);
  if (kExposureAttributes.find(kw) == kExposureAttributes.end()) {
    return false;
  }
  auto* manager = page_view_->intersection_observer_manager();
  if (manager) {
    ExposeObserver* newAddedObserver = nullptr;
    if (!manager->HasExposeObserver(this)) {
      auto observer =
          std::make_unique<ExposeObserver>(manager, clay::Value::Map(), this);
      newAddedObserver = observer.get();
      manager->AddObserver(std::move(observer));
    }
    auto result = manager->UpdateExposeData(attr, value, this);
    if (result) {
      has_intersection_observer_ = true;
    } else if (newAddedObserver) {
      // if is new added observer and operate failed, revert the added
      // observer
      manager->RemoveObserver(newAddedObserver);
    }
    return result;
  }
  return false;
}

void BaseView::AddEventCallback(const char* event_c) {
  std::string event(event_c);
  if (!events_) {
    events_ = std::make_optional<std::vector<std::string>>();
  }
  if (event.compare(event_attr::kEventLayoutChange) == 0) {
    enable_layout_change_event_ = true;
  } else {
    // Handle exposure properties
    if (UpdateExposeAttrs(event_c, {})) {
      return;
    }
  }
  events_->emplace_back(event);
}

void BaseView::SetGestureDetectorMap(const GestureMap& gesture_detector_map) {
  if (gesture_detector_map.size() == 0) {
    return;
  }
  gesture_detector_map_ = gesture_detector_map;
  GestureDetectorDidSet();
}

void BaseView::GestureDetectorDidSet() {
  auto* gesture_arena_manager =
      page_view_->GetGestureHandlerDispatcher()->gesture_arena_manager();
  if (!gesture_arena_manager) return;
  if (!gesture_arena_manager->IsMemberExist(gesture_arena_member_id_)) {
    gesture_arena_member_id_ = gesture_arena_manager->AddMember(GetWeakPtr());
  }
  if (gesture_handler_map_.empty() && gesture_arena_member_id_ > 0) {
    gesture_handler_map_ = BaseGestureHandler::ConvertToGestureHandler(
        id_, page_view_, GetWeakPtr(), gesture_detector_map_);
  }
}

std::vector<float> BaseView::GestureScrollBy(float delta_x, float delta_y) {
  return std::vector<float>{0, 0, delta_x, delta_y};
}

void BaseView::SetGestureDetectorState(int gesture_id, int state) {
  auto* gesture_arena_manager =
      page_view_->GetGestureHandlerDispatcher()->gesture_arena_manager();
  if (!gesture_arena_manager ||
      !gesture_arena_manager->IsMemberExist(gesture_arena_member_id_))
    return;
  gesture_arena_manager->SetGestureDetectorState(gesture_arena_member_id_,
                                                 gesture_id, state);
}

void BaseView::ConsumeGesture(int gesture_id, const Value& params) {
  if (!params.IsMap()) return;
  const Value::Map& map = params.GetMap();
  bool inner = false;
  bool consume = false;
  if (map.find("inner") != map.end()) {
    inner = map.at("inner").GetBool();
  }
  if (map.find("consume") != map.end()) {
    consume = map.at("consume").GetBool();
  }
  if (inner) {
    enable_builtin_gesture_recognizer_ = consume;
  } else {
    intercept_gesture_status_ =
        consume ? InterceptGestureStatus::True : InterceptGestureStatus::False;
  }
}

std::vector<float> BaseView::ScrollBy(float delta_x, float delta_y) {
  return GestureScrollBy(delta_x, delta_y);
}

#ifndef NDEBUG
void BaseView::DumpViewTree(int depth) const {
  std::string intent;
  for (int i = 0; i < depth; ++i) {
    intent.append((i == depth - 1) ? "|-" : "| ");
  }
  FML_LOG(ERROR) << intent << "[" << GetName() << "] " << this << ToString();
  for (BaseView* child : children_) {
    child->DumpViewTree(depth + 1);
  }
}

std::string BaseView::ToString() const {
  std::stringstream ss;
  ss << " name=" << GetName();
  ss << " id=" << id();
  ss << " frame=(" << Left() << "," << Top() << "," << Width() << ","
     << Height() << ")";
  if (!GetScrollOffset().IsOrigin()) {
    ss << " scroll_offset=" << GetScrollOffset().ToString();
  }
  if (PaddingLeft() != 0 || PaddingTop() != 0 || PaddingRight() != 0 ||
      PaddingBottom() != 0) {
    ss << " padding=(" << PaddingLeft() << "," << PaddingTop() << ","
       << PaddingRight() << "," << PaddingBottom() << ")";
  }
  if (!id_selector_.empty()) {
    ss << " id_selector=" << id_selector_;
  }
  if (!Visible()) {
    ss << " hidden";
  }
  return ss.str();
}
#endif

void BaseView::setFocus(const LynxModuleValues& args,
                        const LynxUIMethodCallback& callback) {
  bool focus = IsFocused();
  bool scroll = true;
  CastNamedLynxModuleArgs({"focus", "scroll"}, args, focus, scroll);

  if (focus) {
    RequestFocus();
    if (scroll) {
      ScrollToFocus();
    }
  } else {
    ClearFocus();
  }
  callback(LynxUIMethodResult::kSuccess, clay::Value());
}

void BaseView::interceptBackKeyOnce(const LynxModuleValues& args,
                                    const LynxUIMethodCallback& callback) {
  page_view()->SetInterceptBackKeyOnce(true);
  callback(LynxUIMethodResult::kSuccess, clay::Value());
}

void BaseView::cancelInterceptBackKey(const LynxModuleValues& args,
                                      const LynxUIMethodCallback& callback) {
  page_view()->SetInterceptBackKeyOnce(false);
  callback(LynxUIMethodResult::kSuccess, clay::Value());
}

void BaseView::boundingClientRect(const LynxModuleValues& args,
                                  const LynxUIMethodCallback& callback) {
  std::string target_view = utils::GetCString(args.Get("relativeTo"), "");
  BaseView* relative_to = nullptr;
  if (target_view != "") {
    if (!target_view.empty() && target_view[0] == '#') {
      target_view.erase(0, 1);
    }
    relative_to = page_view()->FindViewByIdSelector(target_view);
  }
  auto rect = BoundsRelativeTo(relative_to);

  clay::Value::Map map;
  map["id"] = clay::Value(GetIdSelector());
  map["left"] = clay::Value(ToLogical(rect.x()));
  map["right"] = clay::Value(ToLogical(rect.MaxX()));
  map["top"] = clay::Value(ToLogical(rect.y()));
  map["bottom"] = clay::Value(ToLogical(rect.MaxY()));
  map["width"] = clay::Value(ToLogical(rect.width()));
  map["height"] = clay::Value(ToLogical(rect.height()));
  map["dataset"] = CloneClayValue(data_set_);
  callback(LynxUIMethodResult::kSuccess, clay::Value(std::move(map)));
}

void BaseView::scrollIntoView(const LynxModuleValues& args,
                              const LynxUIMethodCallback& callback) {
  if (!args.HasKey("scrollIntoViewOptions")) {
    callback(LynxUIMethodResult::kParamInvalid, clay::Value());
  }
  const auto& scroll_into_view_options = args.Get("scrollIntoViewOptions");
  const auto& map = utils::GetMap(scroll_into_view_options);
  auto block = utils::GetCString(utils::GetMapItem(map, "block"));
  auto inline_mode = utils::GetCString(utils::GetMapItem(map, "inline"));
  auto behavior = utils::GetCString(utils::GetMapItem(map, "behavior"));

  BaseView* node = this;
  while (node->Parent()) {
    if (node->Is<ScrollView>()) {
      static_cast<ScrollView*>(node)->StartScrollInto(this, block, inline_mode,
                                                      behavior);
      break;
    }
    node = node->Parent();
  }
  callback(LynxUIMethodResult::kSuccess, clay::Value());
}

void BaseView::takeScreenshot(const LynxModuleValues& args,
                              const LynxUIMethodCallback& callback) {
  auto format = utils::GetCString(args.Get("format"), "png");
  auto scale = utils::GetNum(args.Get("scale"), 1.0);
  if (scale <= 0 || scale > 1.0) {
    callback(LynxUIMethodResult::kParamInvalid, clay::Value());
    return;
  }

  // 1. Make sure this View is repaint boundary;
  // 2. Call PageView::MakeRasterSnapshot;
  // 3. Gen sub LayerTree with FrameBuilder from the current View, apply the
  //    scale using a TransformLayer;
  // 4. Call Rasterizer::MakeRasterSnapshot with subtree, and get the result
  //    dlimage;
  // 5. Encode the dlimage to png or jpeg;
  // 6. Convert to base64 and return it to the Frontend.
  BaseView* node = this;
  node->SetRepaintBoundary(true);
  bool compress_jpeg = format == "jpeg";
  page_view_->MakeRasterSnapshot(
      node, compress_jpeg, scale,
      [compress_jpeg, scale, callback](GrDataPtr data, int32_t w, int32_t h) {
        if (!data) {
          callback(LynxUIMethodResult::kOperationError, clay::Value());
          return;
        }
#ifndef ENABLE_SKITY
        size_t length =
            fml::Base64::Encode(data->data(), data->size(), nullptr);
        std::vector<char> base64_data(length);
        fml::Base64::Encode(data->data(), data->size(), base64_data.data());
#else
        size_t length =
            fml::Base64::Encode(data->RawData(), data->Size(), nullptr);
        std::vector<char> base64_data(length);
        fml::Base64::Encode(data->RawData(), data->Size(), base64_data.data());
#endif  // ENABLE_SKITY
        std::string result_data(compress_jpeg ? "data:image/jpeg;base64,"
                                              : "data:image/png;base64,");
        result_data.append(base64_data.begin(), base64_data.end());
        clay::Value::Map map;
        map["width"] = clay::Value(w);
        map["height"] = clay::Value(h);
        map["data"] = clay::Value(result_data);
        callback(LynxUIMethodResult::kSuccess, clay::Value(std::move(map)));
      });
}

void BaseView::ScrollToFocus() {
  FloatRect focused_view_rect = GetFocusManager()->GetFocusedNodeRect();
  if (!focused_view_rect.IsEmpty()) {
    ScrollChildViewToVisible(focused_view_rect);
    if (Parent()) {
      Parent()->ScrollToFocus();
    }
  }
}

void BaseView::OnAnimationEvent(const AnimationParams& animation_params) {
  bool has_event = false;
  auto event_type = animation_params.event_type;
  switch (event_type) {
    case kClayEventTypeAnimationStart:
      has_event = HasEvent(event_attr::kEventAnimationStart);
      break;
    case kClayEventTypeAnimationRepeat:
      has_event = HasEvent(event_attr::kEventAnimationIteration);
      break;
    case kClayEventTypeAnimationEnd:
      has_event = HasEvent(event_attr::kEventAnimationEnd);
      break;
    case kClayEventTypeAnimationCancel:
      has_event = HasEvent(event_attr::kEventAnimationCancel);
      break;
    default:
      FML_DCHECK(false);
      return;
  }

  if (has_event && !(page_view_->IsRasterAnimationEnabled() &&
                     page_view_->GetKeyframesMap(
                         animation_params.animation_name) != nullptr)) {
    page_view()->DispatchAnimationEvent(animation_params, GetCallbackId());
  }
}

bool BaseView::HasAnimationEvent(const ClayEventType& event_type) const {
  std::string event;
  switch (event_type) {
    case kClayEventTypeAnimationStart:
      event = event_attr::kEventAnimationStart;
      break;
    case kClayEventTypeAnimationRepeat:
      event = event_attr::kEventAnimationIteration;
      break;
    case kClayEventTypeAnimationEnd:
      event = event_attr::kEventAnimationEnd;
      break;
    case kClayEventTypeAnimationCancel:
      event = event_attr::kEventAnimationCancel;
      break;
    case kClayEventTypeTransitionStart:
      event = event_attr::kEventTransitionStart;
      break;
    case kClayEventTypeTransitionEnd:
      event = event_attr::kEventTransitionEnd;
      break;
    default:
      return false;
  }
  return !event.empty() && HasEvent(event);
}

void BaseView::OnTransitionEvent(const AnimationParams& animation_params,
                                 ClayAnimationPropertyType property_type) {
  bool has_event = false;
  auto event_type = animation_params.event_type;
  switch (event_type) {
    case kClayEventTypeTransitionStart:
      has_event = HasEvent(event_attr::kEventTransitionStart);
      break;
    case kClayEventTypeTransitionEnd:
      has_event = HasEvent(event_attr::kEventTransitionEnd);
      break;
    default:
      FML_DCHECK(false);
      return;
  }
#ifdef ENABLE_RASTER_CACHE_SCALE
  UpdateCacheStrategy();
#endif
  if (has_event && !(page_view_->IsRasterAnimationEnabled() &&
                     IsRasterAnimationProperty(property_type))) {
    page_view()->DispatchTransitionEvent(animation_params, GetCallbackId(),
                                         property_type);
  }
}

void BaseView::OnMouseEnter(const PointerEvent& event) {
  is_mouse_hover_ = true;
  OnMouseHoverChange();
  OnMouseEvent(kClayEventTypeMouseEnter, event);
}

void BaseView::OnMouseLeave(const PointerEvent& event) {
  is_mouse_hover_ = false;
  OnMouseHoverChange();
  OnMouseEvent(kClayEventTypeMouseLeave, event);
}

void BaseView::OnMouseHover(const PointerEvent& event) {
  OnMouseEvent(kClayEventTypeMouseOver, event);
}

void BaseView::MarkDirty() { Invalidate(); }

void BaseView::OnMouseHoverChange() {
  if (page_view_->GetEventDelegate()) {
    page_view_->GetEventDelegate()->OnHoverChanged(GetCallbackId(),
                                                   is_mouse_hover_);
  }
}

void BaseView::OnMouseEvent(const ClayEventType type,
                            const PointerEvent& event) {
  if (page_view_->GetEventDelegate()) {
    auto position = event.position;
    FloatPoint view_point = GetPointBySelf(position);
    // In flutter, event has no `button` property.
    // In web, only `buttons` has possible value for mouseenter/mouseleave.
    int buttons = type == kClayEventTypeMouseOver ? 0 : event.buttons;
    page_view_->GetEventDelegate()->OnMouseEvent(
        EventTypeToString(type), GetCallbackId(), 0, buttons, 1, view_point.x(),
        view_point.y(), position.x(), position.y());
  }
}

void BaseView::OnLayoutChange() {
  if (enable_layout_change_event_ && page_view_->GetEventDelegate()) {
    const auto& id_selector = GetIdSelector();
    FloatRect rect = page_view_->ConvertTo<kPixelTypeLogical>(GetBounds());
    auto map = CreateClayMap(
        {"id", "left", "right", "top", "bottom", "width", "height"},
        id_selector.c_str(), rect.x(), rect.MaxX(), rect.y(), rect.MaxY(),
        rect.width(), rect.height());
    page_view_->GetEventDelegate()->OnLayoutChanged(GetCallbackId(),
                                                    std::move(map));
  }
}

void BaseView::LayoutUpdated() {
  if (NeedsLayoutUpdated()) {
    needs_layout_updated_ = false;
    OnLayoutUpdated();
  }
  for (auto& i : children_) {
    i->LayoutUpdated();
  }
}

void BaseView::UpdateCacheStrategy() {
  auto running_animators = TransitionMgr()->GetRunningAnimators();
  bool force_cache = false;
  for (auto animator : running_animators) {
    if (animator->GetDuration() <= FORCE_CACHE_ANIMATION_DURATION) {
      force_cache = true;
    } else {
      force_cache = false;
      break;
    }
  }
  // Update self and descendant render object's cache strategy.
  if (force_cache) {
    render_object()->SetCacheStrategyRecursively(CacheStrategy::ForceCache);
  } else if (render_object()->GetCacheStrategy() == CacheStrategy::ForceCache) {
    render_object()->SetCacheStrategyRecursively(CacheStrategy::NotCache);
  }
}

void BaseView::EndAllTransitionsRecursively() {
  if (transition_mgr_ && transition_mgr_->HasAnimationRunning()) {
    transition_mgr_->EndAllAnimators();
  }
  for (auto* child : children_) {
    child->EndAllTransitionsRecursively();
  }
}

void BaseView::GetTransformValue(float left, float right, float top,
                                 float bottom, TransOffset& res) {
  GetLocationOnScreen(left, top, res.left_top);
  GetLocationOnScreen(width_ + right, top, res.right_top);
  GetLocationOnScreen(width_ + right, height_ + bottom, res.right_bottom);
  GetLocationOnScreen(left, height_ + bottom, res.left_bottom);
}

void BaseView::GetLocationOnScreen(float in_out_location_x,
                                   float in_out_location_y,
                                   std::vector<float>& res) {
  // TODO(wangyanyi): not considerate node which may be detached and overlay
  res = TransformFromViewToRootView(this, in_out_location_x, in_out_location_y);
}

std::vector<float> BaseView::TransformFromViewToRootView(
    BaseView* view, float& in_out_location_x, float& in_out_location_y) {
  auto float_point = FloatPoint(in_out_location_x, in_out_location_y);
  Transform transform = Transform();
  if (view->render_object()->HasTransform()) {
    transform = view->render_object()->GetTransform();
  }
  if (!transform.IsIdentity()) {
    transform.TransformPoint(&float_point);
  }

  while (view->Parent() && view != page_view_) {
    auto parent_view = view->Parent();

    float_point.SetX(float_point.x() + view->Left() -
                     parent_view->render_object()->ScrollLeft());
    float_point.SetY(float_point.y() + view->Top() -
                     parent_view->render_object()->ScrollTop());

    auto parent_transform = parent_view->GetTransform();
    if (!parent_transform.IsIdentity()) {
      parent_transform.TransformPoint(&float_point);
    }

    view = parent_view;
  }
  return std::vector<float>{float_point.x(), float_point.y()};
}

void BaseView::UpdateTransitionRasterAnimation(ClayAnimationPropertyType type) {
  if (!IsRasterAnimationEnabled()) {
    Invalidate();
    return;
  }
  if (IsRasterAnimationProperty(type)) {
    bool has_animation = TransitionMgr()->IsAnimationRunning(type);
    render_object_->MarkHasTransition(type, has_animation);
    if (has_animation) {
      render_object_->SetTransitionManager(transition_mgr_.get());
      SetRepaintBoundary(true);
    }
    Invalidate();
  }
}

void BaseView::UpdateKeyframesRasterAnimation() {
  if (!IsRasterAnimationEnabled()) {
    Invalidate();
    return;
  }
  bool keyframes_manager_set = false;
  bool raster_animation_changed = false;
  ForEachRasterAnimationProperty(
      [this, &keyframes_manager_set,
       &raster_animation_changed](ClayAnimationPropertyType type) {
        bool has_animation = KeyframesMgr()->HasAnimationForType(type);
        raster_animation_changed =
            (render_object_->HasAnimation(type) == has_animation) ||
            raster_animation_changed;
        render_object_->MarkHasAnimation(type, has_animation);
        if (has_animation && !keyframes_manager_set) {
          render_object_->SetKeyframesManager(keyframes_mgr_.get());
          SetRepaintBoundary(true);
          keyframes_manager_set = true;
        }
      });
  if (raster_animation_changed) {
    Invalidate();
  }
}

void BaseView::UpdateRenderObjectTransformOrigin() {
  if (transform_origin_.has_value()) {
    render_object()->SetTransformOrigin(
        (*transform_origin_).GetValue(width_, height_));
  } else {
    render_object()->SetTransformOrigin({width_ / 2.0f, height_ / 2.0f});
  }
}

bool BaseView::IsRasterAnimationEnabled() const {
  return page_view_->IsRasterAnimationEnabled();
}

void BaseView::UpdateSticky(std::optional<StickyInfo> sticky) {
  if (!sticky.has_value()) {
    sticky_ = std::nullopt;
    return;
  }
  sticky_ = sticky;

  if (Parent() && Parent()->Is<ScrollView>()) {
    static_cast<ScrollView*>(Parent())->EnableSticky();
  }
}

void BaseView::CheckStickyOnParentScrollAndReset(int left, int top) {
  if (!sticky_.has_value()) {
    return;
  }

  float current_left = Left() - left;
  float current_top = Top() - top;
  if (current_left < sticky_->left) {
    sticky_->offset_x = sticky_->left - current_left;
  } else {
    int parent_width = Parent()->Width();
    float current_right = current_left + Width();
    if (current_right + sticky_->right > parent_width) {
      sticky_->offset_x =
          std::max(parent_width - current_right - sticky_->right,
                   sticky_->left - current_left);
    } else {
      sticky_->offset_x = 0;
    }
  }

  if (current_top < sticky_->top) {
    sticky_->offset_y = sticky_->top - current_top;
  } else {
    int parent_height = Parent()->Height();
    float current_bottom = current_top + Height();
    if (current_bottom + sticky_->bottom > parent_height) {
      sticky_->offset_y =
          std::max(parent_height - current_bottom - sticky_->bottom,
                   sticky_->top - current_top);
    } else {
      sticky_->offset_y = 0;
    }
  }
  post_translation_->SetX(sticky_->offset_x);
  post_translation_->SetY(sticky_->offset_y);
  TransformOperations transform_ops;
  transform_ops.AppendTranslate(sticky_->offset_x, sticky_->offset_y, 0);
  transform_ops.Append(transform_ops_);
  float old_translate_z = render_object()->GetTranslateZ();
  render_object()->SetTransformOperations(transform_ops, false);
  if (std::abs(render_object()->GetTranslateZ() - old_translate_z) > 1e-6) {
    Parent()->DirtyChildrenPaintingOrder();
  }
}
#ifdef ENABLE_ACCESSIBILITY
void BaseView::MarkRebuildSemanticsTree() {
  if (auto* semantics_owner = GetSemanticsOwner()) {
    if (!semantics_owner->NeedRebuildSemanticsTree()) {
      semantics_owner->SetRebuildSemanticsTree(true);
    }
  }
}

void BaseView::UpdateSemantics(
    const std::vector<fml::RefPtr<SemanticsNode>>& new_children,
    BaseView* parent_node_view, bool need_check_children, bool force_update) {
  FML_DCHECK(semantics_);
  UpdateSemanticsData(parent_node_view);

  semantics_->UpdateWith(new_children, need_check_children, force_update);

  // If label is "", this semantics_node will use its children's labels instead.
  if (page_view() != this && semantics_->GetSemanticsData().label.empty()) {
    semantics_->GetSemanticsData().label =
        semantics_->GetAccessibilityLabelWithChildren();
    if (!semantics_->GetSemanticsData().label.empty()) {
      semantics_->MarkDirty();
    }
  }
}

FloatRect BaseView::GetSemanticsBounds() const {
  auto semantics_bounds = GetBounds();
  float left = left_;
  float top = top_;
  auto* parent = Parent();
  while (parent) {
    left += parent->Left();
    top += parent->Top();
    if (parent->Is<ScrollView>()) {
      auto delta = static_cast<ScrollView*>(parent)->GetScrollOffset();
      left -= delta.x();
      top -= delta.y();
    }
    parent = parent->Parent();
  }
  semantics_bounds.SetX(left);
  semantics_bounds.SetY(top);
  return semantics_bounds;
}

void BaseView::UpdateSemanticsData(BaseView* parent_node_view) {
  FML_DCHECK(semantics_);
  // Keep current data to old data, so that we can easily compare if
  // SemanticsData has changed.
  semantics_->TransferCurrentData();

  semantics_->GetSemanticsData().actions = GetSemanticsActions();
  semantics_->GetSemanticsData().flags = GetSemanticsFlags();
  semantics_->GetSemanticsData().scroll_children = GetA11yScrollChildren();
  // CAUTION: SemanticsBounds need to be the global bounds to PageView.
  semantics_->GetSemanticsData().semantics_bounds = GetSemanticsBounds();
  semantics_->GetSemanticsData().transform =
      AccumulateTransformFromView(parent_node_view);

  semantics_->GetSemanticsData().accessibility_elements =
      accessibility_elements_.value_or(std::vector<std::string>());

  semantics_->GetSemanticsData().id_selector = GetIdSelector();

  semantics_->GetSemanticsData().label = GetAccessibilityLabel();
}

int32_t BaseView::GetA11yScrollChildren() const { return 0; }

int32_t BaseView::GetSemanticsActions() const {
  int32_t actions = 0;
  if (HasTapEvent() || HasTapGestureRecognizer()) {
    actions |= static_cast<int32_t>(SemanticsNode::SemanticsAction::kTap);
  }
  if (HasLongPressEvent() || HasLongPressGestureRecognizer()) {
    actions |= static_cast<int32_t>(SemanticsNode::SemanticsAction::kLongPress);
  }
  if (Parent() && Parent()->Is<ScrollView>()) {
    if (static_cast<ScrollView*>(Parent())->CanScrollX()) {
      actions |=
          static_cast<int32_t>(SemanticsNode::SemanticsAction::kScrollLeft) |
          static_cast<int32_t>(SemanticsNode::SemanticsAction::kScrollRight);
    } else if (static_cast<ScrollView*>(Parent())->CanScrollY()) {
      actions |=
          static_cast<int32_t>(SemanticsNode::SemanticsAction::kScrollUp) |
          static_cast<int32_t>(SemanticsNode::SemanticsAction::kScrollDown);
    }
  }
  return actions;
}

int32_t BaseView::GetSemanticsFlags() const {
  int32_t flags = 0;
  if (Parent() && Parent()->Is<ScrollView>()) {
    flags |= static_cast<int32_t>(
        SemanticsNode::SemanticsFlag::kHasImplicitScrolling);
  }
  return flags;
}

std::u16string BaseView::GetAccessibilityLabel() const {
  return accessibility_label_.value_or(u"");
}

Transform BaseView::AccumulateTransformFromView(BaseView* view) const {
  Transform total_transform = render_object()->GetTransform();
  BaseView* parent = Parent();
  while (view != parent && parent) {
    total_transform.ConcatTransform(parent->GetTransform());
    parent = parent->Parent();
  }
  return total_transform;
}

void BaseView::PrepareSemantics(
    fml::RefPtr<SemanticsNode> parent_node,
    std::vector<fml::RefPtr<SemanticsNode>>& result,
    const std::vector<std::string>& ancestor_a11y_elements) {
  FML_DCHECK(GetSemanticsOwner() &&
             GetSemanticsOwner()->NeedRebuildSemanticsTree());
  FML_DCHECK(Parent());
  // This view is not a11y visible when its ancestors have specified
  // accessibility-elements and its id_selector is not inside it.
  if (!IsAccessibilityElement() ||
      (ancestor_a11y_elements.size() > 0 &&
       (id_selector_.empty() ||
        std::find(ancestor_a11y_elements.begin(), ancestor_a11y_elements.end(),
                  id_selector_) == ancestor_a11y_elements.end()))) {
    for (auto* view : children_) {
      view->PrepareSemantics(parent_node, result, ancestor_a11y_elements);
    }
    return;
  }
  FML_DCHECK(parent_node);
  if (!semantics_) {
    FML_DCHECK(id() > 0);
    semantics_ = fml::MakeRefCounted<SemanticsNode>(this, id());
  }
  if (!semantics_->Attached()) {
    semantics_->Attach(parent_node->Owner());
  }
  std::vector<fml::RefPtr<SemanticsNode>> new_children;
  std::vector<std::string> a11y_elements =
      accessibility_elements_.value_or(ancestor_a11y_elements);
  for (auto* view : children_) {
    view->PrepareSemantics(semantics_, new_children, a11y_elements);
  }
  UpdateSemantics(new_children, parent_node->OwnerView(), true, true);
  result.push_back(semantics_);
}

bool BaseView::IsAccessibilityElement() const {
  FML_DCHECK(page_view());
  // If talkback or voiceover is not enabled, there is no need to regard any
  // nodes to be accessible.
  if (!page_view()->IsSemanticsEnabled()) {
    return false;
  }
  // Text and Image are not affected by lynx page config
  // `enableAccessibilityElement`.
  if (Is<BaseImageView>() || Is<BaseTextView>() || Is<EditableView>() ||
      Is<TextAreaView>() || Is<TextAreaNGView>()) {
    return accessibility_element_.value_or(true);
  }
  // Only views created from lynx should be visible to a11y system.
  // RawTextView is also internal view but id is not -1.
  if (IsInternalView() || Is<RawTextView>()) {
    return false;
  }
  return accessibility_element_.value_or(
      page_view() && page_view()->IsPageEnableAccessibilityElement());
  return false;
}

void BaseView::SetAccessibilityElement(bool value) {
  if (accessibility_element_ != value) {
    accessibility_element_ = value;
    // Since a11y visibility has changed, we need to rebuild the semantics
    // tree.
    MarkRebuildSemanticsTree();
    page_view()->RequestNewFrameForSemantics();
  }
}

void BaseView::SetAccessibilityLabel(const std::string& value) {
  std::u16string value_u16 = UnicodeUtil::Utf8ToUtf16(value);
  if (accessibility_label_ != value_u16) {
    accessibility_label_ = value_u16;
    if (auto* semantics_owner = GetSemanticsOwner()) {
      semantics_owner->AddDirtySemanticsForDescendants(this);
    }
    page_view()->RequestNewFrameForSemantics();
  }
}

void BaseView::SetAccessibilityElements(const std::string& value) {
  accessibility_elements_ =
      lynx::base::SplitString<std::string, std::string>(value, ",");
  MarkRebuildSemanticsTree();
  page_view()->RequestNewFrameForSemantics();
}

SemanticsOwner* BaseView::GetSemanticsOwner() const {
  return page_view()->GetSemanticsOwner();
}
#endif

void BaseView::VisitChildren(const std::function<void(BaseView*)>& visitor) {
  std::vector<BaseView*> children_copy(children_.begin(), children_.end());
  for (auto* child : children_copy) {
    visitor(child);
    child->VisitChildren(visitor);
  }
}

void BaseView::RebuildSortedChildrenIfNeeded() {
  if (!sorted_children_.empty()) {
    return;
  }

  sorted_children_ = GetChildren();
  bool needs_reorder = false;
  for (auto* child : sorted_children_) {
    if (child->GetTranslateZ() || child->GetPaintingOrder()) {
      needs_reorder = true;
      break;
    }
  }

  if (!needs_reorder) {
    return;
  }

  std::stable_sort(
      sorted_children_.begin(), sorted_children_.end(),
      [](BaseView* node1, BaseView* node2) {
        if (node1->GetTranslateZ() < node2->GetTranslateZ()) {
          return true;
        } else if (node1->GetTranslateZ() == node2->GetTranslateZ()) {
          return node1->GetPaintingOrder() < node2->GetPaintingOrder();
        }
        return false;
      });
}

float BaseView::FromLogical(float value) const {
  return page_view()->ConvertFrom<kPixelTypeLogical>(value);
}

float BaseView::ToLogical(float value) const {
  return page_view()->ConvertTo<kPixelTypeLogical>(value);
}

void BaseView::OnNodeReady() {
  OnTransitionAnimationReady();
  OnAnimationNodeReady();
}

void BaseView::UpdateInlineImageInfo() {
  for (auto child : GetChildren()) {
    if (child->Is<InlineImageView>()) {
      auto image = static_cast<InlineImageView*>(child);
      child->SetX(image->GetLocation().x() + ContentInsetLeft());
      child->SetY(image->GetLocation().y() + ContentInsetLeft());
    } else {
      child->UpdateInlineImageInfo();
    }
  }
}

void BaseView::DecodeImagesRecursively() {
  VisitChildren([](BaseView* view) { view->render_object()->DecodeImages(); });
}

}  // namespace clay
