// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/flutter_windows_view.h"

#include <chrono>

#include "clay/ui/platform/cursor_types.h"
namespace clay {

namespace {
// The maximum duration to block the platform thread for while waiting
// for a window resize operation to complete.
constexpr std::chrono::milliseconds kWindowResizeTimeout{100};

/// Returns true if the surface will be updated as part of the resize process.
///
/// This is called on window resize to determine if the platform thread needs
/// to be blocked until the frame with the right size has been rendered. It
/// should be kept in-sync with how the engine deals with a new surface request
/// as seen in `CreateOrUpdateSurface` in `GPUSurfaceGL`.
bool SurfaceWillUpdate(size_t cur_width, size_t cur_height, size_t target_width,
                       size_t target_height) {
  // TODO (https://github.com/flutter/flutter/issues/65061) : Avoid special
  // handling for zero dimensions.
  bool non_zero_target_dims = target_height > 0 && target_width > 0;
  bool not_same_size =
      (cur_height != target_height) || (cur_width != target_width);
  return non_zero_target_dims && not_same_size;
}

/// Update the surface's swap interval to block until the v-blank iff
/// the system compositor is disabled.
void UpdateVsync(FlutterWindowsEngine* engine, egl::WindowSurface* surface,
                 bool needs_vsync) {
  egl::Manager* egl_manager = engine->egl_manager();
  if (!egl_manager) {
    return;
  }

  auto update_vsync = [egl_manager, surface, needs_vsync]() {
    if (!surface || !surface->IsValid()) {
      return;
    }

    if (!surface->MakeCurrent()) {
      FML_LOG(ERROR) << "Unable to make the render surface current to update "
                        "the swap interval";
      return;
    }

    if (!surface->SetVSyncEnabled(needs_vsync)) {
      FML_LOG(ERROR) << "Unable to update the render surface's swap interval";
    }

    if (!egl_manager->render_context()->ClearCurrent()) {
      FML_LOG(ERROR) << "Unable to clear current surface after updating "
                        "the swap interval";
    }
  };

  // Updating the vsync makes the EGL context and render surface current.
  // If the engine is running, the render surface should only be made current on
  // the raster thread. If the engine is initializing, the raster thread doesn't
  // exist yet and the render surface can be made current on the platform
  // thread.
  if (engine->running()) {
    engine->PostRasterThreadTask(update_vsync);
  } else {
    update_vsync();
  }
}

/// Destroys a rendering surface that backs a Flutter view.
void DestroyWindowSurface(const FlutterWindowsEngine& engine,
                          std::unique_ptr<egl::WindowSurface> surface) {
  // EGL surfaces are used on the raster thread if the engine is running.
  // There may be pending raster tasks that use this surface. Destroy the
  // surface on the raster thread to avoid concurrent uses.
  if (engine.running()) {
    engine.PostRasterThreadTask(fml::MakeCopyable(
        [surface = std::move(surface)] { surface->Destroy(); }));
  } else {
    // There's no raster thread if engine isn't running. The surface can be
    // destroyed on the platform thread.
    surface->Destroy();
  }
}

}  // namespace

FlutterWindowsView::FlutterWindowsView(
    std::unique_ptr<WindowBindingHandler> window_binding)
    : FlutterWindowsView(kImplicitViewId, std::move(window_binding)) {}

FlutterWindowsView::FlutterWindowsView(
    FlutterViewId view_id, std::unique_ptr<WindowBindingHandler> window_binding)
    : view_id_(view_id) {
  // Take the binding handler, and give it a pointer back to self.
  binding_handler_ = std::move(window_binding);
  binding_handler_->SetView(this);

  render_target_ = std::make_unique<WindowsRenderTarget>(
      binding_handler_->GetRenderTarget());
}

FlutterWindowsView::~FlutterWindowsView() {
  binding_handler_.reset();
  if (view_id_ == kImplicitViewId) {
    // stop draw texture in raster thread before destroy window view avoid crash
    engine_->Stop();
    engine_->SetView(nullptr);
  }
  if (surface_) {
    DestroyWindowSurface(*engine_, std::move(surface_));
  }
}

FlutterViewId FlutterWindowsView::view_id() const { return view_id_; }

void FlutterWindowsView::ReleasePointer(int32_t pointer_id) {
  auto it = pointer_states_.find(pointer_id);
  if (it == pointer_states_.end()) {
    return;
  }

  ClayPointerEvent event = {};
  event.x = it->second->physical_x;
  event.y = it->second->physical_y;
  event.phase = ClayPointerPhase::kClayPointerPhaseRemove;
  SendPointerEventWithData(event, it->second.get());
}

void FlutterWindowsView::ReleaseAllPointers() {
  std::vector<int32_t> pointer_ids;
  pointer_ids.reserve(pointer_states_.size());
  for (const auto& entry : pointer_states_) {
    pointer_ids.push_back(entry.first);
  }
  for (int32_t pointer_id : pointer_ids) {
    ReleasePointer(pointer_id);
  }
}

void FlutterWindowsView::SetEngine(FlutterWindowsEngine* engine) {
  engine_ = engine;

  move_handle_ =
      std::make_unique<clay::WindowMoveHandle>(binding_handler_.get());

  mouse_drop_handle_ = std::make_unique<clay::WindowMouseDropHandle>(
      binding_handler_.get(), engine_);

  PhysicalWindowBounds bounds = binding_handler_->GetPhysicalWindowBounds();

  SendWindowMetrics(bounds.width, bounds.height,
                    binding_handler_->GetDpiScale());
}

void FlutterWindowsView::MoveWindow() { move_handle_->MoveWindow(); }

uint32_t FlutterWindowsView::GetFrameBufferId(size_t width, size_t height) {
  // Called on an engine-controlled (non-platform) thread.
  std::unique_lock<std::mutex> lock(resize_mutex_);

  if (resize_status_ != ResizeState::kResizeStarted) {
    return kWindowFrameBufferID;
  }

  if (resize_target_width_ == width && resize_target_height_ == height) {
    if (!ResizeRenderSurface(resize_target_width_, resize_target_height_)) {
      return kWindowFrameBufferID;
    }
    resize_status_ = ResizeState::kFrameGenerated;
  }

  return kWindowFrameBufferID;
}

void FlutterWindowsView::SetDamageRegion(const clay::Rect& region) {
  surface_->SetDamageRegion(region);
}

std::optional<clay::Rect> FlutterWindowsView::GetDamageRegion() {
  return surface_->GetDamageRegion();
}

void FlutterWindowsView::UpdateFlutterCursor(clay::CursorTypes cursor_type) {
  binding_handler_->UpdateFlutterCursor(cursor_type);
}

void FlutterWindowsView::SetFlutterCursor(HCURSOR cursor) {
  binding_handler_->SetFlutterCursor(cursor);
}

void FlutterWindowsView::ForceRedraw() {
  if (engine_ && resize_status_ == ResizeState::kDone) {
    // Request new frame.
    engine_->ScheduleFrame();
  }
}

void FlutterWindowsView::OnWindowSizeChanged(size_t width, size_t height) {
  // Called on the platform thread.
  std::unique_lock<std::mutex> lock(resize_mutex_);

  if (!engine_) {
    return;
  }

  if (!engine_->egl_manager()) {
    SendWindowMetrics(width, height, binding_handler_->GetDpiScale());
    return;
  }

  if (!surface_ || !surface_->IsValid()) {
    SendWindowMetrics(width, height, binding_handler_->GetDpiScale());
    return;
  }

  bool surface_will_update =
      SurfaceWillUpdate(surface_->width(), surface_->height(), width, height) ||
      SurfaceWillUpdate(resize_target_width_, resize_target_height_, width,
                        height);
  if (surface_will_update) {
    resize_status_ = ResizeState::kResizeStarted;
    resize_target_width_ = width;
    resize_target_height_ = height;
  }

  SendWindowMetrics(width, height, binding_handler_->GetDpiScale());

  if (surface_will_update) {
    // Block the platform thread until:
    //   1. GetFrameBufferId is called with the right frame size.
    //   2. Any pending SwapBuffers calls have been invoked.
    resize_cv_.wait_for(lock, kWindowResizeTimeout,
                        [&resize_status = resize_status_] {
                          return resize_status == ResizeState::kDone;
                        });
  }
}

void FlutterWindowsView::OnWindowRepaint() { ForceRedraw(); }

void FlutterWindowsView::OnPointerMove(double x, double y,
                                       ClayPointerDeviceKind device_kind,
                                       int32_t device_id, int modifiers_state) {
  engine_->keyboard_key_handler()->SyncModifiersIfNeeded(modifiers_state);
  SendPointerMove(x, y, GetOrCreatePointerState(device_kind, device_id));
}

void FlutterWindowsView::OnPointerDown(double x, double y,
                                       ClayPointerDeviceKind device_kind,
                                       int32_t device_id,
                                       ClayPointerMouseButtons flutter_button) {
  if (flutter_button != 0) {
    auto state = GetOrCreatePointerState(device_kind, device_id);
    int32_t pointer_id = state->pointer_id;
    uint64_t previous_buttons = state->buttons;
    state->buttons |= flutter_button;
    if (!SendPointerDown(x, y, state)) {
      auto it = pointer_states_.find(pointer_id);
      if (it != pointer_states_.end()) {
        it->second->buttons = previous_buttons;
      }
    }
  }
}

void FlutterWindowsView::OnPointerUp(double x, double y,
                                     ClayPointerDeviceKind device_kind,
                                     int32_t device_id,
                                     ClayPointerMouseButtons flutter_button) {
  if (flutter_button != 0) {
    auto state = GetPointerState(device_kind, device_id);
    if (!state) {
      return;
    }
    int32_t pointer_id = state->pointer_id;
    uint64_t previous_buttons = state->buttons;
    state->buttons &= ~flutter_button;
    if (!SendPointerUp(x, y, state)) {
      auto it = pointer_states_.find(pointer_id);
      if (it != pointer_states_.end()) {
        it->second->buttons = previous_buttons;
      }
    }
  }
}

void FlutterWindowsView::OnPointerLeave(double x, double y,
                                        ClayPointerDeviceKind device_kind,
                                        int32_t device_id) {
  PointerState* state = GetPointerState(device_kind, device_id);
  if (state) {
    SendPointerLeave(x, y, state);
  }
}

void FlutterWindowsView::OnPointerPanZoomStart(int32_t device_id) {
  PointerLocation point = binding_handler_->GetPrimaryPointerLocation();
  SendPointerPanZoomStart(device_id, point.x, point.y);
}

void FlutterWindowsView::OnPointerPanZoomUpdate(int32_t device_id, double pan_x,
                                                double pan_y, double scale,
                                                double rotation) {
  SendPointerPanZoomUpdate(device_id, pan_x, pan_y, scale, rotation);
}

void FlutterWindowsView::OnPointerPanZoomEnd(int32_t device_id) {
  SendPointerPanZoomEnd(device_id);
}

void FlutterWindowsView::OnText(const std::u16string& text) { SendText(text); }

void FlutterWindowsView::OnKey(int key, int scancode, int action,
                               char32_t character, bool extended, bool was_down,
                               KeyEventCallback callback) {
  SendKey(key, scancode, action, character, extended, was_down, callback);
}

void FlutterWindowsView::OnComposeBegin() { SendComposeBegin(); }

void FlutterWindowsView::OnComposeCommit() { SendComposeCommit(); }

void FlutterWindowsView::OnComposeEnd() { SendComposeEnd(); }

void FlutterWindowsView::OnComposeChange(const std::u16string& text,
                                         int cursor_pos) {
  SendComposeChange(text, cursor_pos);
}

void FlutterWindowsView::OnScroll(double x, double y, double delta_x,
                                  double delta_y, int scroll_offset_multiplier,
                                  ClayPointerDeviceKind device_kind,
                                  int32_t device_id) {
  SendScroll(x, y, delta_x, delta_y, scroll_offset_multiplier, device_kind,
             device_id);
}

void FlutterWindowsView::OnScrollInertiaCancel(int32_t device_id) {
  PointerLocation point = binding_handler_->GetPrimaryPointerLocation();
  SendScrollInertiaCancel(device_id, point.x, point.y);
}

void FlutterWindowsView::OnCursorRectUpdated(const FloatRect& rect) {
  binding_handler_->OnCursorRectUpdated(rect);
}

void FlutterWindowsView::OnResetImeComposing() {
  binding_handler_->OnResetImeComposing();
}

void FlutterWindowsView::OnTextInputClientChange(int client_id) {
  binding_handler_->OnTextInputClientChange(client_id);
}

// Sends new size  information to FlutterEngine.
void FlutterWindowsView::SendWindowMetrics(size_t width, size_t height,
                                           double dpi_scale) const {
  clay::ViewportMetrics metrics;
  metrics.physical_width = width;
  metrics.physical_height = height;
  metrics.device_pixel_ratio = dpi_scale;
  // Default logical pixel is 96
  metrics.device_density_dpi = dpi_scale * 96.f;
  metrics.physical_view_inset_top = 0.0;
  metrics.physical_view_inset_right = 0.0;
  metrics.physical_view_inset_bottom = 0.0;
  metrics.physical_view_inset_left = 0.0;
  engine_->SendWindowMetricsEvent(metrics);
}

void FlutterWindowsView::SendInitialBounds() {
  PhysicalWindowBounds bounds = binding_handler_->GetPhysicalWindowBounds();

  SendWindowMetrics(bounds.width, bounds.height,
                    binding_handler_->GetDpiScale());
}

FlutterWindowsView::PointerState* FlutterWindowsView::GetOrCreatePointerState(
    ClayPointerDeviceKind device_kind, int32_t device_id) {
  // Create a virtual pointer ID that is unique across all device types
  // to prevent pointers from clashing in the engine's converter
  // (ui/window/pointer_data_packet_converter.cc)
  int32_t pointer_id = (static_cast<int32_t>(device_kind) << 28) | device_id;

  auto [it, added] = pointer_states_.try_emplace(pointer_id, nullptr);
  if (added) {
    auto state = std::make_unique<PointerState>();
    state->device_kind = device_kind;
    state->pointer_id = pointer_id;
    it->second = std::move(state);
  }

  return it->second.get();
}

FlutterWindowsView::PointerState* FlutterWindowsView::GetPointerState(
    ClayPointerDeviceKind device_kind, int32_t device_id) {
  int32_t pointer_id = (static_cast<int32_t>(device_kind) << 28) | device_id;
  auto it = pointer_states_.find(pointer_id);
  if (it == pointer_states_.end()) {
    return nullptr;
  }
  return it->second.get();
}

// Set's |event_data|'s phase to either kMove or kHover depending on the current
// primary mouse button state.
void FlutterWindowsView::SetEventPhaseFromCursorButtonState(
    ClayPointerEvent* event_data, const PointerState* state) const {
  // For details about this logic, see ClayPointerPhase in the clay.h
  // file.
  if (state->buttons == 0) {
    event_data->phase = state->clay_state_is_down
                            ? ClayPointerPhase::kClayPointerPhaseUp
                            : ClayPointerPhase::kClayPointerPhaseHover;
  } else {
    event_data->phase = state->clay_state_is_down
                            ? ClayPointerPhase::kClayPointerPhaseMove
                            : ClayPointerPhase::kClayPointerPhaseDown;
  }
}

bool FlutterWindowsView::SendPointerMove(double x, double y,
                                         PointerState* state) {
  ClayPointerEvent event = {};
  event.x = x;
  event.y = y;

  SetEventPhaseFromCursorButtonState(&event, state);
  return SendPointerEventWithData(event, state);
}

bool FlutterWindowsView::SendPointerDown(double x, double y,
                                         PointerState* state) {
  ClayPointerEvent event = {};
  event.x = x;
  event.y = y;

  SetEventPhaseFromCursorButtonState(&event, state);
  if (!SendPointerEventWithData(event, state)) {
    return false;
  }

  state->clay_state_is_down = true;
  return true;
}

bool FlutterWindowsView::SendPointerUp(double x, double y,
                                       PointerState* state) {
  ClayPointerEvent event = {};
  event.x = x;
  event.y = y;

  SetEventPhaseFromCursorButtonState(&event, state);
  if (!SendPointerEventWithData(event, state)) {
    return false;
  }
  if (event.phase == ClayPointerPhase::kClayPointerPhaseUp) {
    state->clay_state_is_down = false;
  }
  return true;
}

bool FlutterWindowsView::SendPointerLeave(double x, double y,
                                          PointerState* state) {
  ClayPointerEvent event = {};
  event.x = x;
  event.y = y;
  event.phase = ClayPointerPhase::kClayPointerPhaseRemove;
  return SendPointerEventWithData(event, state);
}

bool FlutterWindowsView::SendPointerPanZoomStart(int32_t device_id, double x,
                                                 double y) {
  auto state =
      GetOrCreatePointerState(kClayPointerDeviceKindTrackpad, device_id);
  state->pan_zoom_start_x = x;
  state->pan_zoom_start_y = y;
  ClayPointerEvent event = {};
  event.x = x;
  event.y = y;
  event.phase = ClayPointerPhase::kClayPointerPhasePanZoomStart;
  return SendPointerEventWithData(event, state);
}

bool FlutterWindowsView::SendPointerPanZoomUpdate(int32_t device_id,
                                                  double pan_x, double pan_y,
                                                  double scale,
                                                  double rotation) {
  auto state =
      GetOrCreatePointerState(kClayPointerDeviceKindTrackpad, device_id);
  ClayPointerEvent event = {};
  event.x = state->pan_zoom_start_x;
  event.y = state->pan_zoom_start_y;
  event.pan_x = pan_x;
  event.pan_y = pan_y;
  event.scale = scale;
  event.rotation = rotation;
  event.phase = ClayPointerPhase::kClayPointerPhasePanZoomUpdate;
  return SendPointerEventWithData(event, state);
}

bool FlutterWindowsView::SendPointerPanZoomEnd(int32_t device_id) {
  auto state =
      GetOrCreatePointerState(kClayPointerDeviceKindTrackpad, device_id);
  ClayPointerEvent event = {};
  event.x = state->pan_zoom_start_x;
  event.y = state->pan_zoom_start_y;
  event.phase = ClayPointerPhase::kClayPointerPhasePanZoomEnd;
  return SendPointerEventWithData(event, state);
}

void FlutterWindowsView::SendText(const std::u16string& text) {
  engine_->text_input_plugin()->TextHook(text);
}

void FlutterWindowsView::SendKey(int key, int scancode, int action,
                                 char32_t character, bool extended,
                                 bool was_down, KeyEventCallback callback) {
  engine_->keyboard_key_handler()->KeyboardHook(
      key, scancode, action, character, extended, was_down,
      [=, callback = std::move(callback)](bool handled) {
        if (!handled) {
          engine_->text_input_plugin()->KeyboardHook(
              key, scancode, action, character, extended, was_down);
        }
        callback(handled);
      });
}

void FlutterWindowsView::SendComposeBegin() {
  engine_->text_input_plugin()->ComposeBeginHook();
}

void FlutterWindowsView::SendComposeCommit() {
  engine_->text_input_plugin()->ComposeCommitHook();
}

void FlutterWindowsView::SendComposeEnd() {
  engine_->text_input_plugin()->ComposeEndHook();
}

void FlutterWindowsView::SendComposeChange(const std::u16string& text,
                                           int cursor_pos) {
  engine_->text_input_plugin()->ComposeChangeHook(text, cursor_pos);
}

bool FlutterWindowsView::SendScroll(double x, double y, double delta_x,
                                    double delta_y,
                                    int scroll_offset_multiplier,
                                    ClayPointerDeviceKind device_kind,
                                    int32_t device_id) {
  auto state = GetOrCreatePointerState(device_kind, device_id);

  ClayPointerEvent event = {};
  event.x = x;
  event.y = y;
  event.signal_kind = ClayPointerSignalKind::kClayPointerSignalKindScroll;
  event.scroll_delta_x = delta_x * scroll_offset_multiplier;
  event.scroll_delta_y = delta_y * scroll_offset_multiplier;
  event.is_precise_scroll = 0;
  SetEventPhaseFromCursorButtonState(&event, state);
  return SendPointerEventWithData(event, state);
}

bool FlutterWindowsView::SendScrollInertiaCancel(int32_t device_id, double x,
                                                 double y) {
  auto state =
      GetOrCreatePointerState(kClayPointerDeviceKindTrackpad, device_id);

  ClayPointerEvent event = {};
  event.x = x;
  event.y = y;
  event.signal_kind =
      ClayPointerSignalKind::kClayPointerSignalKindScrollInertiaCancel;
  SetEventPhaseFromCursorButtonState(&event, state);
  return SendPointerEventWithData(event, state);
}

bool FlutterWindowsView::SendPointerEventWithData(
    const ClayPointerEvent& event_data, PointerState* state) {
  // If sending anything other than an add, and the pointer isn't already added,
  // synthesize an add to satisfy Flutter's expectations about events.
  if (!state->clay_state_is_added &&
      event_data.phase != ClayPointerPhase::kClayPointerPhaseAdd) {
    ClayPointerEvent event = {};
    event.phase = ClayPointerPhase::kClayPointerPhaseAdd;
    event.x = event_data.x;
    event.y = event_data.y;
    event.buttons = 0;
    if (!SendPointerEventWithData(event, state)) {
      return false;
    }
  }
  // Don't double-add (e.g., if events are delivered out of order, so an add has
  // already been synthesized).
  if (state->clay_state_is_added &&
      event_data.phase == ClayPointerPhase::kClayPointerPhaseAdd) {
    return true;
  }

  ClayPointerEvent event = event_data;
  event.device_kind = state->device_kind;
  event.device = state->pointer_id;
  event.buttons = state->buttons;

  // Set metadata that's always the same regardless of the event.
  event.struct_size = sizeof(event);
  event.timestamp =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();
  event.is_precise_scroll = event_data.is_precise_scroll;

  if (!engine_->SendPointerEvent(this, event)) {
    if (event.phase != ClayPointerPhase::kClayPointerPhaseAdd) {
      pointer_states_.erase(state->pointer_id);
    }
    return false;
  }

  state->physical_x = event.x;
  state->physical_y = event.y;

  if (event.phase == ClayPointerPhase::kClayPointerPhaseAdd) {
    state->clay_state_is_added = true;
  } else if (event.phase == ClayPointerPhase::kClayPointerPhaseRemove) {
    auto it = pointer_states_.find(state->pointer_id);
    if (it != pointer_states_.end()) {
      pointer_states_.erase(it);
    }
  }
  return true;
}

bool FlutterWindowsView::MakeCurrent() {
  if (!surface_ || !surface_->IsValid()) {
    return false;
  }
  return surface_->MakeCurrent();
}

bool FlutterWindowsView::SwapBuffers() {
  if (!surface_ || !surface_->IsValid()) {
    return false;
  }
  // Called on an engine-controlled (non-platform) thread.
  std::unique_lock<std::mutex> lock(resize_mutex_);

  switch (resize_status_) {
    // SwapBuffer requests during resize are ignored until the frame with the
    // right dimensions has been generated. This is marked with
    // kFrameGenerated resize status.
    case ResizeState::kResizeStarted:
      return false;
    case ResizeState::kFrameGenerated: {
      bool visible = binding_handler_->IsVisible();
      bool swap_buffers_result;
      // For visible windows swap the buffers while resize handler is waiting.
      // For invisible windows unblock the handler first and then swap buffers.
      // SwapBuffers waits for vsync and there's no point doing that for
      // invisible windows.
      if (visible) {
        swap_buffers_result = surface_->SwapBuffers();
      }
      resize_status_ = ResizeState::kDone;
      lock.unlock();
      resize_cv_.notify_all();
      binding_handler_->OnWindowResized();
      if (!visible) {
        swap_buffers_result = surface_->SwapBuffers();
      }
      return swap_buffers_result;
    }
    case ResizeState::kDone:
    default:
      return surface_->SwapBuffers();
  }
}

bool FlutterWindowsView::PresentSoftwareBitmap(const void* allocation,
                                               size_t row_bytes,
                                               size_t height) {
  return binding_handler_->OnBitmapSurfaceUpdated(allocation, row_bytes,
                                                  height);
}

void FlutterWindowsView::CreateRenderSurface() {
  FML_DCHECK(surface_ == nullptr);

  if (engine_ && engine_->egl_manager()) {
    PhysicalWindowBounds bounds = binding_handler_->GetPhysicalWindowBounds();
    bool enable_vsync = binding_handler_->NeedsVSync();
    surface_ = engine_->egl_manager()->CreateWindowSurface(
        egl::GLImplementationType::kAngleEGL, GetWindowHandle(), bounds.width,
        bounds.height);
    UpdateVsync(engine_, surface_.get(), enable_vsync);
    resize_target_width_ = bounds.width;
    resize_target_height_ = bounds.height;
  }
}

bool FlutterWindowsView::ResizeRenderSurface(size_t width, size_t height) {
  FML_DCHECK(surface_ != nullptr);

  // No-op if the surface is already the desired size.
  if (width == surface_->width() && height == surface_->height()) {
    return true;
  }

  return surface_->Resize(width, height);
}

void FlutterWindowsView::UpdateHighContrastEnabled(bool enabled) {
  engine_->UpdateHighContrastEnabled(enabled);
}

WindowsRenderTarget* FlutterWindowsView::GetRenderTarget() const {
  return render_target_.get();
}

HWND FlutterWindowsView::GetWindowHandle() const {
  return binding_handler_->GetWindowHandle();
}

FlutterWindowsEngine* FlutterWindowsView::GetEngine() { return engine_; }

void FlutterWindowsView::NotifyWinEventWrapper(DWORD event, HWND hwnd,
                                               LONG idObject, LONG idChild) {
  // if (hwnd) {
  //   NotifyWinEvent(EVENT_SYSTEM_ALERT, hwnd, OBJID_CLIENT,
  //                  AccessibilityRootNode::kAlertChildId);
  // }
}

void FlutterWindowsView::OnDwmCompositionChanged() {
  if (!surface_ || !surface_->IsValid()) {
    return;
  }

  if (!surface_->MakeCurrent()) {
    FML_LOG(ERROR) << "Unable to make the render surface current to update "
                      "the swap interval";
    return;
  }
  surface_->SetVSyncEnabled(binding_handler_->NeedsVSync());
}

}  // namespace clay
