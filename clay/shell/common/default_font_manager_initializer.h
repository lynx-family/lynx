// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_DEFAULT_FONT_MANAGER_INITIALIZER_H_
#define CLAY_SHELL_COMMON_DEFAULT_FONT_MANAGER_INITIALIZER_H_

#include <cstdint>
#include <mutex>
#include <vector>

#include "base/include/fml/memory/weak_ptr.h"
#include "base/include/fml/task_runner.h"

namespace clay {

class Engine;

// Coordinates process-wide Windows Skia default font manager initialization
// and notifies every engine that laid out a page while initialization was in
// progress.
class DefaultFontManagerInitializer {
 public:
  static DefaultFontManagerInitializer& Instance();

  void Request(fml::WeakPtr<Engine> engine,
               fml::RefPtr<fml::TaskRunner> io_task_runner,
               fml::RefPtr<fml::TaskRunner> ui_task_runner,
               uint32_t font_initialization_data);

 private:
  enum class State {
    kNotStarted,
    kLoading,
    kReady,
  };

  struct PendingEngine {
    fml::WeakPtr<Engine> engine;
    fml::RefPtr<fml::TaskRunner> ui_task_runner;
  };

  DefaultFontManagerInitializer() = default;

  std::mutex mutex_;
  State state_ = State::kNotStarted;
  std::vector<PendingEngine> pending_engines_;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_DEFAULT_FONT_MANAGER_INITIALIZER_H_
