// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_HEADLESS_PUBLIC_EMBDDER_HEADLESS_DELEGATE_H_
#define CLAY_SHELL_PLATFORM_HEADLESS_PUBLIC_EMBDDER_HEADLESS_DELEGATE_H_

#include <future>

#include "build/build_config.h"

namespace lynx {
class HeadlessDelegate {
 public:
#if defined(OS_LINUX)
  // Linux windowless sync callbacks delivered on the platform thread.
  virtual void OnInvalidate() {}
  virtual void SubmitFrameFuture(std::future<bool> future) {}
#endif
  virtual const char* GetClipboardData() const = 0;
  virtual void SetClipboardData(const char* data) = 0;
  virtual void ActivateSystemCursor(int type, const char* path) = 0;
  virtual void UpdateCaretPosition(float x, float y, float width,
                                   float height) = 0;
  virtual void ShowTextInput() = 0;
  virtual void HideTextInput() = 0;
  virtual void SetCursorPosition(int position) = 0;
  virtual void SetMarkedTextRect(float x, float y, float width,
                                 float height) = 0;
  virtual void SetEditableTransform(const float transform_matrix[16]) = 0;
};
}  // namespace lynx

#endif  // CLAY_SHELL_PLATFORM_HEADLESS_PUBLIC_EMBDDER_HEADLESS_DELEGATE_H_
