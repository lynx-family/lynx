// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_HEADLESS_PUBLIC_EMBDDER_HEADLESS_DELEGATE_H_
#define CLAY_SHELL_PLATFORM_HEADLESS_PUBLIC_EMBDDER_HEADLESS_DELEGATE_H_

namespace lynx {
class HeadlessDelegate {
 public:
  virtual const char* GetClipboardData() const = 0;
  virtual void SetClipboardData(const char* data) = 0;
  virtual void ActivateSystemCursor(int type, const char* path) = 0;
  virtual void ShowTextInput() = 0;
  virtual void HideTextInput() = 0;
  virtual void SetMarkedTextRect(float x, float y, float width,
                                 float height) = 0;
  virtual void SetEditableTransform(const float transform_matrix[16]) = 0;
};
}  // namespace lynx

#endif  // CLAY_SHELL_PLATFORM_HEADLESS_PUBLIC_EMBDDER_HEADLESS_DELEGATE_H_
