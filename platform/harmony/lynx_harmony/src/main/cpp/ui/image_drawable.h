// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_IMAGE_DRAWABLE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_IMAGE_DRAWABLE_H_

#include <native_drawing/drawing_canvas.h>
#include <native_drawing/drawing_pixel_map.h>

#include <memory>
#include <vector>

#include "base/include/thread/timed_task.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_helper.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {
class ImageData;
class ImageDrawable {
 public:
  enum class ImageMode {
    kAspectFit,
    kAspectFill,
    kScaleToFill,
  };

  class ImageAnimationListener {
   public:
    virtual void onAnimationStart() {}
    virtual void onAnimationRepeat() {}
    virtual void onAnimationStop() {}
  };

  explicit ImageDrawable(const std::weak_ptr<UIBase>& ui_base);
  void UpdateBounds(float left, float top, float width, float height,
                    float padding_left, float padding_top, float padding_right,
                    float padding_bottom, float scale_density);
  void UpdateDrawCurrent(std::unique_ptr<LynxBaseImage> pixelmap);
  void UpdateDrawCurrent(std::shared_ptr<ImageData> image_data);
  void UpdateDrawMatrix();
  void UpdateMode(ImageMode mode);
  void UpdateImageRendering(starlight::ImageRenderingType);
  void UpdateTintColor(uint32_t tint_color);
  void UpdateLoopCount(int32_t loop_count);
  void Render(OH_Drawing_Canvas* canvas);
  void DrawPixelMap(OH_Drawing_Canvas* canvas,
                    OH_Drawing_PixelMap* draw_bitmap);
  bool HasContent() {
    return pixel_maps_ != nullptr || !image_data_draw_bitmaps_.empty();
  }
  bool IsAnimate();
  bool IsAnimateRunning() { return is_running_; };
  void StartAnimation();
  void StopAnimation();
  void PauseAnimation();
  OH_Drawing_PixelMap* GetCurrentDrawBitmap();
  ~ImageDrawable();
  void ResetContent();
  void DestroyMatrix();
  void InvalidateSelf();

  void AddImageAnimationListener(ImageAnimationListener* listener);
  void RemoveImageAnimationListener(ImageAnimationListener* listener);

 private:
  std::unique_ptr<LynxBaseImage> pixel_maps_{nullptr};
  std::shared_ptr<ImageData> image_data_{nullptr};
  std::vector<OH_Drawing_PixelMap*> image_data_draw_bitmaps_;
  std::unique_ptr<base::TimedTaskManager> timer_task_manager_{nullptr};
  ImageMode mode_{ImageMode::kScaleToFill};
  std::weak_ptr<UIBase> ui_base_;
  std::vector<ImageAnimationListener*> image_anim_listeners_;
  OH_Drawing_Rect* src_rect_{nullptr};
  OH_Drawing_Rect* dst_rect_{nullptr};
  OH_Drawing_ColorFilter* color_filter_{nullptr};
  OH_Drawing_Filter* filter_{nullptr};
  OH_Drawing_SamplingOptions* sample_{nullptr};
  uint32_t image_width_{0};
  uint32_t image_height_{0};
  float width_{0.f};
  float height_{0.f};
  float left_{0.f};
  float top_{0.f};
  float right_{0.f};
  float bottom_{0.f};
  OH_Drawing_Matrix* matrix_{nullptr};
  OH_Drawing_Brush* brush_{nullptr};
  int32_t loop_count_{0};

  bool is_running_{false};
  bool is_paused_{false};

  uint64_t start_time_ms_{0};
  uint64_t last_frame_anim_time_ms_{0};
  uint64_t expected_render_time_ms_{0};
  uint64_t paused_start_time_ms_diff_{0};
  uint64_t paused_last_frame_anim_time_ms_diff_{0};
  uint64_t frame_scheduling_offset_ms_{0};
  uint64_t loop_duration_{0};

  int32_t paused_last_drawn_frame_index_{-1};
  int32_t last_drawn_frame_index_{-1};

  void ScheduleNextFrame(uint64_t target_animation_time_ms);

  int32_t GetFrameIndexToRender(uint64_t animation_time_ms,
                                uint64_t last_frame_time_ms);

  int32_t GetFrameNumberWithinLoop(uint64_t time_in_current_loop_ms);

  uint64_t GetTargetRenderTimeForNextFrameMS(uint64_t animation_time_ms);

  bool IsInfiniteAnimation() { return loop_count_ == 0; };

  void UnScheduleSelf();

  uint32_t FrameCount() const;

  int32_t FrameDuration(int32_t frame_number) const;

  OH_Drawing_PixelMap* FrameDrawBitmap(int32_t frame_number) const;

  bool IsAnimImage() const;

  void DestroyImageDataDrawBitmaps();

  void DispatchAnimationStart();

  void DispatchAnimationStop();

  void DispatchAnimationRepeat();
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_IMAGE_DRAWABLE_H_
