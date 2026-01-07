// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PAINTING_CONTEXT_DARWIN_UTILS_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PAINTING_CONTEXT_DARWIN_UTILS_H_

namespace lynx {
namespace tasm {

class PaintingContextDarwinUtils {
 public:
  template <typename F>
  static void ExecuteSafely(const F& func);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PAINTING_CONTEXT_DARWIN_UTILS_H_
