// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMMON_RENDER_SETTINGS_H_
#define CLAY_UI_COMMON_RENDER_SETTINGS_H_

#include <atomic>

#include "base/include/fml/memory/ref_counted.h"

namespace clay {
class RenderSettings : public fml::RefCountedThreadSafe<RenderSettings> {
 public:
  // kSMOOTH：Priority to guarantee the frame rate.
  // kNEW_CONTENT：Priority to reduce content update time. not implemented.
  enum RenderMode {
    kNORMAL = 0,
    kSMOOTH,
    kNEW_CONTENT,
  };
  RenderSettings();
  ~RenderSettings();
  void RenewMode();
  void SetIsTouching(bool);
  void SetHasAnimation(bool);

  RenderMode GetMode() const { return mode_; }
  bool ShouldLowMemoryUsage() const;

  void SetIgnoreRasterCache(bool value) { ignore_raster_cache = value; }
  bool IgnoreRasterCache() const { return ignore_raster_cache; }

 private:
  std::atomic<RenderMode> mode_ = kNORMAL;
  bool is_touching_ = false;
  bool has_animation_ = false;
  // Wether ignore raster cache for page.
  std::atomic<bool> ignore_raster_cache = false;
};
}  // namespace clay
#endif  // CLAY_UI_COMMON_RENDER_SETTINGS_H_
