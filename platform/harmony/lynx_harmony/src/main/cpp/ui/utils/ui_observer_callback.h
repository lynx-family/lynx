// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_UI_OBSERVER_CALLBACK_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_UI_OBSERVER_CALLBACK_H_

#include <string>

namespace lynx {
namespace tasm {
namespace harmony {

class UIObserverCallback {
 public:
  virtual void PostTask() = 0;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_UI_OBSERVER_CALLBACK_H_
