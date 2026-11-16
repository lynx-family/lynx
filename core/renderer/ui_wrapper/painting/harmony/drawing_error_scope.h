// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_DRAWING_ERROR_SCOPE_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_DRAWING_ERROR_SCOPE_H_

#include <native_drawing/drawing_error_code.h>

// Reset was introduced in API 18. A weak reference keeps API 17 loadable.
extern "C" void OH_Drawing_ErrorCodeReset(void) __attribute__((weak));

namespace lynx::tasm {

class HarmonyDrawingErrorScope {
 public:
  HarmonyDrawingErrorScope()
      : has_fresh_error_state_(OH_Drawing_ErrorCodeReset != nullptr) {
    if (has_fresh_error_state_) OH_Drawing_ErrorCodeReset();
  }

  // Older runtimes have no way to clear another operation's error. Callers
  // still check allocations, geometry and APIs with explicit return values.
  bool Succeeded() const {
    return !has_fresh_error_state_ ||
           OH_Drawing_ErrorCodeGet() == OH_DRAWING_SUCCESS;
  }

 private:
  const bool has_fresh_error_state_;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_DRAWING_ERROR_SCOPE_H_
