// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_LYNX_IMAGE_CONFIG_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_LYNX_IMAGE_CONFIG_H_

namespace lynx {
namespace tasm {
namespace harmony {
class LynxImageConfig {
 public:
  LynxImageConfig() = default;
  ~LynxImageConfig() = default;
  void SetEnableImageLoadCallback(bool enable);
  bool GetEnableImageLoadCallback();

 private:
  bool enable_image_load_callback_{false};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_LYNX_IMAGE_CONFIG_H_
