// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/image_drawable.h"

#include <native_drawing/drawing_brush.h>
#include <native_drawing/drawing_color_filter.h>
#include <native_drawing/drawing_filter.h>
#include <native_drawing/drawing_matrix.h>
#include <native_drawing/drawing_rect.h>
#include <native_drawing/drawing_sampling_options.h>

#include <algorithm>
#include <utility>

#include "base/include/float_comparison.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/public/image_service.h"

namespace lynx {
namespace tasm {
namespace harmony {

static constexpr const int32_t kFrameIndexDone = -1;
static constexpr const uint64_t kfFrameSchedulingDelay = 8;  // ms

ImageDrawable::~ImageDrawable() {
  if (sample_) {
    OH_Drawing_SamplingOptionsDestroy(sample_);
  }
  if (src_rect_) {
    OH_Drawing_RectDestroy(src_rect_);
  }
  if (dst_rect_) {
    OH_Drawing_RectDestroy(dst_rect_);
  }
  if (filter_) {
    OH_Drawing_FilterDestroy(filter_);
  }
  if (color_filter_) {
    OH_Drawing_ColorFilterDestroy(color_filter_);
  }
  if (brush_) {
    OH_Drawing_BrushDestroy(brush_);
  }
  DestroyImageDataDrawBitmaps();
  DestroyMatrix();
}

void ImageDrawable::UpdateBounds(float left, float top, float width,
                                 float height, float padding_left,
                                 float padding_top, float padding_right,
                                 float padding_bottom, float scale_density) {
  if (dst_rect_) {
    OH_Drawing_RectDestroy(dst_rect_);
    dst_rect_ = nullptr;
  }
  width_ = width * scale_density;
  height_ = height * scale_density;
  left_ = padding_left * scale_density;
  top_ = padding_top * scale_density;
  right_ = width_ - padding_right * scale_density;
  bottom_ = height_ - padding_bottom * scale_density;
  dst_rect_ = OH_Drawing_RectCreate(left_, top_, right_, bottom_);
  UpdateDrawMatrix();
}

void ImageDrawable::UpdateDrawCurrent(std::unique_ptr<LynxBaseImage> pixelmap) {
  DestroyImageDataDrawBitmaps();
  image_data_.reset();
  pixel_maps_ = std::move(pixelmap);
  image_width_ = pixel_maps_->Width();
  image_height_ = pixel_maps_->Height();
  loop_duration_ = static_cast<uint64_t>(pixel_maps_->LoopDuration());
  if (src_rect_) {
    OH_Drawing_RectDestroy(src_rect_);
    src_rect_ = nullptr;
  }
  src_rect_ = OH_Drawing_RectCreate(0, 0, image_width_, image_height_);
  if (!sample_) {
    sample_ = OH_Drawing_SamplingOptionsCreate(FILTER_MODE_LINEAR,
                                               MIPMAP_MODE_LINEAR);
  }
  UpdateDrawMatrix();
  auto ui_base = ui_base_.lock();
  if (ui_base) {
    ui_base->Invalidate();
  }
}

void ImageDrawable::UpdateDrawCurrent(std::shared_ptr<ImageData> image_data) {
  pixel_maps_.reset();
  DestroyImageDataDrawBitmaps();
  loop_duration_ = 0;
  if (!image_data || !image_data->Pixelmap()) {
    image_data_.reset();
    return;
  }
  image_data_ = std::move(image_data);

  auto frame_count = image_data_->FrameCount();
  auto* pixelmap_list = image_data_->PixelmapList();
  auto* delay_time_list = image_data_->DelayTimeList();
  if (frame_count > 1 && pixelmap_list && delay_time_list) {
    image_data_draw_bitmaps_.reserve(frame_count);
    for (uint32_t i = 0; i < frame_count; ++i) {
      image_data_draw_bitmaps_.push_back(
          pixelmap_list[i]
              ? OH_Drawing_PixelMapGetFromOhPixelMapNative(pixelmap_list[i])
              : nullptr);
      loop_duration_ += std::max(delay_time_list[i], 0);
    }
  } else {
    image_data_draw_bitmaps_.push_back(
        OH_Drawing_PixelMapGetFromOhPixelMapNative(image_data_->Pixelmap()));
  }

  auto ui_base = ui_base_.lock();
  if (ui_base) {
    ui_base->Invalidate();
  }
}

void ImageDrawable::UpdateMode(ImageDrawable::ImageMode mode) { mode_ = mode; }

void ImageDrawable::Render(OH_Drawing_Canvas* canvas) {
  if (!pixel_maps_) {
    return;
  }
  if (!pixel_maps_->isAnimImage()) {
    const auto draw_bitmap = pixel_maps_->FirstFrame()->DrawBitmap();
    DrawPixelMap(canvas, draw_bitmap);
  } else {
    // For animation image
    const auto actual_render_time_start_ms = base::CurrentTimeMilliseconds();
    const auto animation_time_ms =
        is_running_
            ? actual_render_time_start_ms - start_time_ms_ +
                  frame_scheduling_offset_ms_
            : std::max(last_frame_anim_time_ms_, static_cast<uint64_t>(0));
    auto frame_index_to_draw =
        GetFrameIndexToRender(animation_time_ms, last_frame_anim_time_ms_);

    if (frame_index_to_draw == kFrameIndexDone) {
      frame_index_to_draw = pixel_maps_->FrameCount() - 1;
      DispatchAnimationStop();
      is_running_ = false;
    } else if (frame_index_to_draw == 0) {
      if (last_drawn_frame_index_ != kFrameIndexDone &&
          last_drawn_frame_index_ != 0 &&
          actual_render_time_start_ms >= expected_render_time_ms_) {
        DispatchAnimationRepeat();
      }
    }
    const auto draw_bitmap =
        pixel_maps_->FramePixelMap(frame_index_to_draw)->DrawBitmap();
    DrawPixelMap(canvas, draw_bitmap);
    last_drawn_frame_index_ = frame_index_to_draw;

    int64_t target_render_time_for_next_frame_ms = 0;
    int64_t scheduled_render_time_for_next_frame_ms = 0;
    const auto actual_render_time_end = base::CurrentTimeMilliseconds();
    if (is_running_) {
      target_render_time_for_next_frame_ms = GetTargetRenderTimeForNextFrameMS(
          actual_render_time_end - start_time_ms_);
      if (target_render_time_for_next_frame_ms != 0) {
        scheduled_render_time_for_next_frame_ms =
            target_render_time_for_next_frame_ms + kfFrameSchedulingDelay;
        ScheduleNextFrame(scheduled_render_time_for_next_frame_ms);
      } else {
        DispatchAnimationStop();
        is_running_ = false;
      }
    }
    last_frame_anim_time_ms_ = animation_time_ms;
  }
}

void ImageDrawable::UpdateDrawMatrix() {
  if (image_width_ <= 0 || image_height_ <= 0) {
    DestroyMatrix();
    return;
  }
  if (width_ <= 0 || height_ <= 0) {
    DestroyMatrix();
    return;
  }
  if (image_width_ == width_ && image_height_ == height_) {
    DestroyMatrix();
    return;
  }
  if (mode_ == ImageMode::kScaleToFill) {
    DestroyMatrix();
    return;
  }
  if (!matrix_) {
    matrix_ = OH_Drawing_MatrixCreate();
  } else {
    OH_Drawing_MatrixReset(matrix_);
  }
  float view_width = right_ - left_;
  float view_height = bottom_ - top_;
  if (mode_ == ImageMode::kAspectFit) {
    float w_rate = view_width / image_width_;
    float h_rate = view_height / image_height_;
    if (base::FloatsLarger(w_rate, h_rate)) {
      float final_width = image_width_ * h_rate;
      float start_w = (view_width - final_width) / 2 + left_;
      OH_Drawing_MatrixSetMatrix(matrix_, h_rate, 0.0f, start_w, 0.0f, h_rate,
                                 top_, 0.0f, 0.0f, 1.0f);
    } else {
      float final_height_ = image_height_ * w_rate;
      float start_h = (view_height - final_height_) / 2 + top_;
      OH_Drawing_MatrixSetMatrix(matrix_, w_rate, 0.0f, left_, 0.0f, w_rate,
                                 start_h, 0.0f, 0.0f, 1.0f);
    }
  } else if (mode_ == ImageMode::kAspectFill) {
    float w_rate = view_width / image_width_;
    float h_rate = view_height / image_height_;
    if (base::FloatsLarger(w_rate, h_rate)) {
      float final_height = image_height_ * w_rate;
      float translate_x = left_;
      float translate_y = (view_height - final_height) / 2 + top_;
      OH_Drawing_MatrixSetMatrix(matrix_, w_rate, 0.0f, translate_x, 0.0f,
                                 w_rate, translate_y, 0.0f, 0.0f, 1.0f);
    } else {
      float final_width = image_width_ * h_rate;
      float translate_x = (view_width - final_width) / 2 + left_;
      float translate_y = top_;
      OH_Drawing_MatrixSetMatrix(matrix_, h_rate, 0.0f, translate_x, 0.0f,
                                 h_rate, translate_y, 0.0f, 0.0f, 1.0f);
    }
  }
}

void ImageDrawable::DestroyMatrix() {
  if (matrix_) {
    OH_Drawing_MatrixDestroy(matrix_);
    matrix_ = nullptr;
  }
}

void ImageDrawable::UpdateImageRendering(starlight::ImageRenderingType) {}

void ImageDrawable::UpdateTintColor(uint32_t tint_color) {
  if (color_filter_) {
    OH_Drawing_ColorFilterDestroy(color_filter_);
    color_filter_ = nullptr;
  }
  color_filter_ = OH_Drawing_ColorFilterCreateBlendMode(
      tint_color, OH_Drawing_BlendMode::BLEND_MODE_SRC_IN);
  if (!filter_) {
    filter_ = OH_Drawing_FilterCreate();
  }
  if (!brush_) {
    brush_ = OH_Drawing_BrushCreate();
    OH_Drawing_BrushSetAntiAlias(brush_, true);
  }
  OH_Drawing_FilterSetColorFilter(filter_, color_filter_);
  OH_Drawing_BrushSetFilter(brush_, filter_);
}

ImageDrawable::ImageDrawable(const std::weak_ptr<UIBase>& ui_base)
    : ui_base_(ui_base) {
  timer_task_manager_ = std::make_unique<base::TimedTaskManager>();
}

void ImageDrawable::ResetContent() {
  pixel_maps_ = nullptr;
  image_data_.reset();
  DestroyImageDataDrawBitmaps();
}

void ImageDrawable::UpdateLoopCount(int32_t loop_count) {
  loop_count_ = static_cast<int>(loop_count);
}

void ImageDrawable::StartAnimation() {
  if (!IsAnimImage()) {
    return;
  }

  if (is_running_ && !is_paused_) {
    return;
  }
  is_running_ = true;
  start_time_ms_ = base::CurrentTimeMilliseconds();
  last_frame_anim_time_ms_ = 0;
  last_drawn_frame_index_ = kFrameIndexDone;
  if (is_paused_) {
    const auto now = base::CurrentTimeMilliseconds();
    start_time_ms_ = now - paused_start_time_ms_diff_;
    expected_render_time_ms_ = start_time_ms_;
    last_frame_anim_time_ms_ = now - paused_last_frame_anim_time_ms_diff_;
    last_drawn_frame_index_ = paused_last_drawn_frame_index_;
    is_paused_ = false;
  }
  InvalidateSelf();
  DispatchAnimationStart();
}

void ImageDrawable::StopAnimation() {
  if (!IsAnimImage()) {
    return;
  }
  if (!is_running_) {
    return;
  }
  is_running_ = false;
  start_time_ms_ = 0;
  expected_render_time_ms_ = start_time_ms_;
  last_frame_anim_time_ms_ = 0;
  last_drawn_frame_index_ = kFrameIndexDone;
  is_paused_ = false;
  UnScheduleSelf();
}

void ImageDrawable::PauseAnimation() {
  if (!IsAnimImage()) {
    return;
  }
  if (is_paused_ || !is_running_) {
    return;
  }
  is_running_ = false;
  const auto now = base::CurrentTimeMilliseconds();
  paused_start_time_ms_diff_ = now - start_time_ms_;
  paused_last_frame_anim_time_ms_diff_ = now - last_frame_anim_time_ms_;
  paused_last_drawn_frame_index_ = last_drawn_frame_index_;
  is_paused_ = true;
  UnScheduleSelf();
}

OH_Drawing_PixelMap* ImageDrawable::GetCurrentDrawBitmap() {
  if (!HasContent()) {
    return nullptr;
  }
  if (!IsAnimImage() || loop_duration_ == 0) {
    return FrameDrawBitmap(0);
  }

  // For animation image.
  const auto actual_render_time_start_ms = base::CurrentTimeMilliseconds();
  const auto animation_time_ms =
      is_running_
          ? actual_render_time_start_ms - start_time_ms_ +
                frame_scheduling_offset_ms_
          : std::max(last_frame_anim_time_ms_, static_cast<uint64_t>(0));
  auto frame_index_to_draw =
      GetFrameIndexToRender(animation_time_ms, last_frame_anim_time_ms_);

  if (frame_index_to_draw == kFrameIndexDone) {
    frame_index_to_draw = FrameCount() - 1;
    DispatchAnimationStop();
    is_running_ = false;
  } else if (frame_index_to_draw == 0) {
    if (last_drawn_frame_index_ != kFrameIndexDone &&
        last_drawn_frame_index_ != 0 &&
        actual_render_time_start_ms >= expected_render_time_ms_) {
      DispatchAnimationRepeat();
    }
  }
  const auto draw_bitmap = FrameDrawBitmap(frame_index_to_draw);
  last_drawn_frame_index_ = frame_index_to_draw;

  int64_t target_render_time_for_next_frame_ms = 0;
  int64_t scheduled_render_time_for_next_frame_ms = 0;
  const auto actual_render_time_end = base::CurrentTimeMilliseconds();
  if (is_running_) {
    target_render_time_for_next_frame_ms = GetTargetRenderTimeForNextFrameMS(
        actual_render_time_end - start_time_ms_);
    if (target_render_time_for_next_frame_ms != 0) {
      scheduled_render_time_for_next_frame_ms =
          target_render_time_for_next_frame_ms + kfFrameSchedulingDelay;
      ScheduleNextFrame(scheduled_render_time_for_next_frame_ms);
    } else {
      DispatchAnimationStop();
      is_running_ = false;
    }
  }
  last_frame_anim_time_ms_ = animation_time_ms;
  return draw_bitmap;
}

void ImageDrawable::InvalidateSelf() {
  auto ui_base = ui_base_.lock();
  if (ui_base) {
    ui_base->Invalidate();
  }
}

void ImageDrawable::ScheduleNextFrame(uint64_t target_animation_time_ms) {
  expected_render_time_ms_ = start_time_ms_ + target_animation_time_ms;
  const auto target_delay =
      expected_render_time_ms_ - base::CurrentTimeMilliseconds();
  timer_task_manager_->SetTimeout(
      [weak_ui = ui_base_] {
        auto ui = weak_ui.lock();
        if (ui) {
          ui->Invalidate();
        }
      },
      target_delay);
}

void ImageDrawable::UnScheduleSelf() { timer_task_manager_->StopAllTasks(); }

int32_t ImageDrawable::GetFrameIndexToRender(uint64_t animation_time_ms,
                                             uint64_t last_frame_time_ms) {
  if (!IsInfiniteAnimation()) {
    const auto loop_count = animation_time_ms / loop_duration_;
    if (loop_count >= loop_count_) {
      return kFrameIndexDone;
    }
  }
  return GetFrameNumberWithinLoop(animation_time_ms % loop_duration_);
}

int32_t ImageDrawable::GetFrameNumberWithinLoop(
    uint64_t time_in_current_loop_ms) {
  const auto frame_count = FrameCount();
  if (frame_count == 0) {
    return 0;
  }
  int32_t frame = 0;
  uint64_t current_duration = 0;
  do {
    current_duration += FrameDuration(frame);
    frame++;
  } while (frame < static_cast<int32_t>(frame_count) &&
           time_in_current_loop_ms >= current_duration);
  return frame - 1;
}

uint64_t ImageDrawable::GetTargetRenderTimeForNextFrameMS(
    uint64_t animation_time_ms) {
  if (loop_duration_ == 0) {
    return 0;
  }
  if (!IsInfiniteAnimation()) {
    auto loop_count = animation_time_ms / loop_duration_;
    if (loop_count >= loop_count_) {
      return 0;
    }
  }
  auto time_passed_in_current_loop_ms = animation_time_ms % loop_duration_;
  auto time_of_next_frame_in_loop_ms = 0;
  uint32_t frame_count = FrameCount();
  for (uint32_t i = 0; i < frame_count && time_of_next_frame_in_loop_ms <=
                                              time_passed_in_current_loop_ms;
       ++i) {
    time_of_next_frame_in_loop_ms += FrameDuration(i);
  }
  auto time_until_next_frame_in_loop_ms =
      time_of_next_frame_in_loop_ms - time_passed_in_current_loop_ms;
  return animation_time_ms + time_until_next_frame_in_loop_ms;
}

void ImageDrawable::DrawPixelMap(OH_Drawing_Canvas* canvas,
                                 OH_Drawing_PixelMap* draw_bitmap) {
  if (draw_bitmap) {
    OH_Drawing_CanvasAttachBrush(canvas, brush_);
    if (matrix_) {
      OH_Drawing_CanvasSave(canvas);
      OH_Drawing_CanvasConcatMatrix(canvas, matrix_);
      OH_Drawing_CanvasDrawPixelMapRect(canvas, draw_bitmap, src_rect_,
                                        src_rect_, sample_);
      OH_Drawing_CanvasRestore(canvas);
    } else {
      OH_Drawing_CanvasDrawPixelMapRect(canvas, draw_bitmap, src_rect_,
                                        dst_rect_, sample_);
    }
    OH_Drawing_CanvasDetachBrush(canvas);
  }
}

bool ImageDrawable::IsAnimate() { return IsAnimImage(); }

uint32_t ImageDrawable::FrameCount() const {
  if (pixel_maps_) {
    return pixel_maps_->FrameCount();
  }
  if (image_data_) {
    return image_data_->FrameCount();
  }
  return 0;
}

int32_t ImageDrawable::FrameDuration(int32_t frame_number) const {
  if (pixel_maps_) {
    return pixel_maps_->FrameDuration(frame_number);
  }
  if (image_data_ && image_data_->DelayTimeList() && frame_number >= 0 &&
      static_cast<uint32_t>(frame_number) < image_data_->FrameCount()) {
    return std::max(image_data_->DelayTimeList()[frame_number], 0);
  }
  return 0;
}

OH_Drawing_PixelMap* ImageDrawable::FrameDrawBitmap(
    int32_t frame_number) const {
  if (pixel_maps_) {
    auto* pixel_map = pixel_maps_->FramePixelMap(frame_number);
    return pixel_map ? pixel_map->DrawBitmap() : nullptr;
  }
  if (frame_number >= 0 &&
      static_cast<size_t>(frame_number) < image_data_draw_bitmaps_.size()) {
    return image_data_draw_bitmaps_[frame_number];
  }
  return nullptr;
}

bool ImageDrawable::IsAnimImage() const { return FrameCount() > 1; }

void ImageDrawable::DestroyImageDataDrawBitmaps() {
  for (auto* draw_bitmap : image_data_draw_bitmaps_) {
    if (draw_bitmap) {
      OH_Drawing_PixelMapDissolve(draw_bitmap);
    }
  }
  image_data_draw_bitmaps_.clear();
}

void ImageDrawable::AddImageAnimationListener(
    ImageDrawable::ImageAnimationListener* listener) {
  image_anim_listeners_.push_back(listener);
}

void ImageDrawable::RemoveImageAnimationListener(
    ImageDrawable::ImageAnimationListener* listener) {
  auto it = std::find(image_anim_listeners_.begin(),
                      image_anim_listeners_.end(), listener);
  if (it != image_anim_listeners_.end()) {
    image_anim_listeners_.erase((it));
  }
}

void ImageDrawable::DispatchAnimationStart() {
  for (auto listener : image_anim_listeners_) {
    listener->onAnimationStart();
  }
}

void ImageDrawable::DispatchAnimationStop() {
  for (auto listener : image_anim_listeners_) {
    listener->onAnimationStop();
  }
}

void ImageDrawable::DispatchAnimationRepeat() {
  for (auto listener : image_anim_listeners_) {
    listener->onAnimationRepeat();
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
