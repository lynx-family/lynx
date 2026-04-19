// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/native_view.h"

#include <cstdint>
#include <tuple>
#include <utility>

#include "base/include/fml/macros.h"
#include "clay/common/graphics/drawable_image.h"
#include "clay/common/graphics/shared_image_external_texture.h"
#include "clay/gfx/shared_image/shared_image_sink.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/lynx_module/lynx_ui_method_registrar.h"
#include "clay/ui/rendering/render_external_content.h"
#include "clay/ui/shadow/shadow_node.h"

namespace clay {

NativeView::NativeView(int id, std::string tag, PageView* page_view)
    : WithTypeInfo(id, std::move(tag),
                   std::make_unique<RenderExternalContent>(), page_view) {
  if (page_view != nullptr && page_view->GetViewContext() != nullptr) {
    composition_preference_ =
        page_view->GetViewContext()->GetNativeViewCompositionPreference(
            GetName());
  }
  Puppet<Owner::kUI, NativeViewService> native_view_service =
      page_view->GetServiceManager()->GetService<NativeViewService>();
  native_view_plugin_ = native_view_service.CreateObjectInActorThread(
      [id, self_ptr = this](auto& service) {
        return service.CreateNativeViewPlugin(id, self_ptr);
      });
  native_view_plugin_.Act(
      [tag = GetName()](auto& plugin)
          -> std::tuple<fml::RefPtr<SharedImageSink>, bool, bool, bool> {
        if (!plugin.OnCreate(tag)) {
          return std::make_tuple(nullptr, false, false, false);
        }
        return std::make_tuple(plugin.GetSharedImageSink(),
                               plugin.SupportHybridComposition(),
                               plugin.SupportScrolling(), true);
      },
      [weak_self = fml::WeakPtr<NativeView>(GetWeakPtr())](
          std::tuple<fml::RefPtr<SharedImageSink>, bool, bool, bool> ret) {
        if (weak_self) {
          auto& [image_sink, support_hybrid_composition, support_scrolling,
                 available] = ret;
          auto content =
              static_cast<RenderExternalContent*>(weak_self->render_object());
          if (image_sink) {
            auto texture =
                std::make_shared<SharedImageExternalTexture>(image_sink);
            weak_self->page_view()->RegisterDrawableImage(texture);
            weak_self->tex_id_ = texture->Id();
            content->SetDrawableImageId(*weak_self->tex_id_);
            content->SetFitMode(DrawableImage::FitMode::kClipToBounds);
            content->SetRenderMode(
                RenderExternalContent::RenderMode::kExternalTexture);
          } else if (support_hybrid_composition) {
            content->SetViewId(weak_self->id());
          }
          weak_self->is_scroll_enabled_ = support_scrolling;
          weak_self->is_available_ = available;
        }
      });
  SetFocusable(true);
}

bool NativeView::ShouldIgnoreForTouchHitTest() const {
  return ignore_for_touch_hit_test_;
}

bool NativeView::HitTest(const PointerEvent& event, HitTestResult& result) {
  if (event.device == PointerEvent::DeviceType::kTouch &&
      ShouldIgnoreForTouchHitTest()) {
    return false;
  }
  return BaseView::HitTest(event, result);
}

BaseView* NativeView::GetTopViewToAcceptEvent(const FloatPoint& position,
                                              FloatPoint* relative_position) {
  if (ShouldIgnoreForTouchHitTest()) {
    return nullptr;
  }
  return BaseView::GetTopViewToAcceptEvent(position, relative_position);
}

void NativeView::FocusHasChanged(bool focused, bool is_leaf) {
  native_view_plugin_.Act([focused, is_leaf](auto& plugin) {
    plugin.OnFocusChanged(focused, is_leaf);
  });
  BaseView::FocusHasChanged(focused, is_leaf);
}

void NativeView::SendMotionEvent(const PointerEvent& point_event,
                                 const FloatPoint& transformed_postion) {
  native_view_plugin_.Act(
      [point_event,
       transformed_postion = page_view()->ConvertTo<kPixelTypePlatform>(
           transformed_postion)](auto& plugin) {
        return plugin.OnTouchEvent(point_event, transformed_postion);
      });
}

// This function is easily confused with the destructor.
// Although 'Destroy' will be called first then the destructor  second
// currently. Maybe we can reactor this and make the destruction process more
// unified.
void NativeView::OnDestroy() {
  UpdateTouchDispatchState(true, /* action= */ 3);
  native_view_plugin_.Act([](auto& plugin) { return plugin.OnDestroy(); });
  if (tex_id_.has_value()) {
    page_view_->UnregisterDrawableImage(*tex_id_);
  }
}

void NativeView::UpdateTouchDispatchState(bool handled, int action) {
  constexpr int kActionDown = 0;
  constexpr int kActionUp = 1;
  constexpr int kActionCancel = 3;
  constexpr int kActionPointerDown = 5;
  constexpr int kActionPointerUp = 6;

  switch (action) {
    case kActionDown:
    case kActionPointerDown:
      ignore_for_touch_hit_test_ = !handled;
      break;
    case kActionUp:
    case kActionPointerUp:
    case kActionCancel:
      ignore_for_touch_hit_test_ = false;
      break;
    default:
      break;
  }
}

void NativeView::SetPaddings(float padding_left, float padding_top,
                             float padding_right, float padding_bottom) {
  // we set padding to platform view to avoid hit zone too small when we add
  // paddings. platform unit , we just pass it to platform
  native_view_plugin_.Act(
      [padding_left, padding_top, padding_right, padding_bottom](auto& plugin) {
        plugin.UpdatePaddings(padding_left, padding_top, padding_right,
                              padding_bottom);
      });
}

void NativeView::DidUpdateAttributes() {
  // TODO(liuguoliang): Should strip render properties like border, padding to
  // prevent Lynx renders border again.
  BaseView::DidUpdateAttributes();
  // Apply all attributes which in the cache once.
  clay::Value::Array events;
  if (events_) {
    for (const auto& event : *events_) {
      events.emplace_back(event);
    }
  }
  native_view_plugin_.Act(
      [staging_attrs = std::move(staging_attrs_), events = std::move(events),
       weak_self = fml::WeakPtr<NativeView>(GetWeakPtr())](auto& plugin) {
        plugin.UpdatePlatformAttributes(staging_attrs, events);

        // Try to create SharedImageSink if necessary.
        if (weak_self && !weak_self->tex_id_.has_value()) {
          fml::RefPtr<SharedImageSink> image_sink = plugin.GetSharedImageSink();
          if (image_sink) {
            auto texture =
                std::make_shared<SharedImageExternalTexture>(image_sink);
            weak_self->page_view()->RegisterDrawableImage(texture);
            weak_self->tex_id_ = texture->Id();

            auto content =
                static_cast<RenderExternalContent*>(weak_self->render_object());
            content->SetDrawableImageId(*weak_self->tex_id_);
            content->SetFitMode(DrawableImage::FitMode::kClipToBounds);
            content->SetRenderMode(
                RenderExternalContent::RenderMode::kExternalTexture);
          }
        }
      });
}

void NativeView::HandleEvent(const PointerEvent& event) {
  if (!IsScrollEnabled()) {
    return;
  }
  BaseView::HandleEvent(event);
  if (event.type == PointerEvent::EventType::kSignalEvent) {
    page_view()->gesture_manager()->RegisterSignalRoute(
        [weak = this->GetWeakPtr()](const PointerEvent& event) {
          // eat signal event
        });
  }
}

void NativeView::InvokePlatformMethod(const std::string& method_name,
                                      clay::Value::Map args,
                                      const LynxUIMethodCallback& callback) {
  native_view_plugin_.Act(
      [method_name, args = std::move(args), callback](auto& plugin) {
        plugin.InvokePlatformMethod(method_name, args, callback);
      });
}

void NativeView::SetAttribute(const char* attr, const clay::Value& value) {
  if (!HandleCommonAttribute(attr, value)) {
    staging_attrs_.emplace(attr, CloneClayValue(value));
  } else if (GetKeywordID(attr) == KeywordID::kName) {
    staging_attrs_.emplace(attr, CloneClayValue(value));
  }
}

void NativeView::ApplyUpdateChanged() {
  if (attach_to_tree()) {
    FloatRect bounds = ContentBoundsInViewport();
    float ratio = page_view()->DevicePixelRatio();
    // Need to update the size & position of platform view.
    // It's necessary for some cases of platform view, we should apply the
    // right layout information. For example, the platform input may show the
    // overlay depends on the layout information.
    if (bounds == bounds_ && ratio == device_pixel_ratio_) {
      return;
    }
    bounds_ = bounds;
    device_pixel_ratio_ = ratio;
    native_view_plugin_.Act(
        [bounds =
             page_view()->ConvertTo<kPixelTypePlatform>(bounds)](auto& plugin) {
          plugin.LayoutChanged(bounds.x(), bounds.y(), bounds.width(),
                               bounds.height());
        });
  }
}

void NativeView::OnPainting() {
  // We have no idea to know whether the content bounds has changed or not.
  // Refer to Android SurfaceView, it use the ViewTreeObserver.onPreDraw to
  // update surface.
  // We check it before global Painting event.
  ApplyUpdateChanged();
}

void NativeView::OnAttachToTree() {
  BaseView::OnAttachToTree();

  native_view_plugin_.Act([](auto& plugin) { return plugin.OnAttach(); });

  page_view_->GetViewTreeObserver()->AddOnPaintingListener(this);
}

void NativeView::OnDetachFromTree() {
  if (is_editing_) {
    is_editing_ = false;
    page_view_->SetEditingPlatformView(nullptr);
  }
  BaseView::OnDetachFromTree();
  native_view_plugin_.Act([](auto& plugin) { return plugin.OnDetach(); });
  page_view_->GetViewTreeObserver()->RemoveOnPaintingListener(this);
}

void NativeView::OnNodeReady() {
  // Ensure layout info is pushed before the layout-finish signal, so Java side
  // can receive onLayout (size/position) earlier than node-ready.
  ApplyUpdateChanged();
  native_view_plugin_.Act([](auto& plugin) { plugin.OnNodeReady(); });
}

MeasureResult NativeView::Measure(const MeasureConstraint& constraint) {
  MeasureConstraint platform_constraint = constraint;
  if (constraint.width.has_value()) {
    platform_constraint.width =
        page_view()->ConvertTo<kPixelTypePlatform>(*constraint.width);
  }
  if (constraint.height.has_value()) {
    platform_constraint.height =
        page_view()->ConvertTo<kPixelTypePlatform>(*constraint.height);
  }
  auto result = native_view_plugin_
                    .ActWithPromise([platform_constraint](auto& plugin) {
                      return plugin.Measure(platform_constraint);
                    })
                    .get()
                    .value_or(MeasureResult{});
  result.width = page_view()->ConvertFrom<kPixelTypePlatform>(result.width);
  result.height = page_view()->ConvertFrom<kPixelTypePlatform>(result.height);
  result.baseline =
      page_view()->ConvertFrom<kPixelTypePlatform>(result.baseline);
  return result;
}

void NativeView::MarkLayoutDirty() {
  auto shadow_node = page_view()->GetShadowNodeById(id());
  if (shadow_node) {
    shadow_node->MarkDirty();
  } else {
    FML_DCHECK(false) << "NativeViewShadowNode not found: " << id();
  }
}

void NativeView::MarkAsEditing() {
  if (is_editing_) return;
  is_editing_ = true;
  page_view_->SetEditingPlatformView(this);
}

void NativeView::OnInsert(int parent_id, int index) {
  native_view_plugin_.Act(
      [parent_id, index](auto& plugin) { plugin.OnInsert(parent_id, index); });
}

void NativeView::ResignFirstResponder() {
  is_editing_ = false;
  native_view_plugin_.Act([](auto& plugin) { plugin.ResignFirstResponder(); });
}

}  // namespace clay
