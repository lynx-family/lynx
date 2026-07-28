// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/default_font_manager_initializer.h"

#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/shell/common/engine.h"
#include "clay/third_party/txt/src/txt/platform.h"
#include "clay/ui/resource/font_collection.h"

namespace clay {

DefaultFontManagerInitializer& DefaultFontManagerInitializer::Instance() {
  // The coordinator may be referenced by tasks during process shutdown.
  static auto* instance = new DefaultFontManagerInitializer();
  return *instance;
}

void DefaultFontManagerInitializer::Request(
    fml::WeakPtr<Engine> engine, fml::RefPtr<fml::TaskRunner> io_task_runner,
    fml::RefPtr<fml::TaskRunner> ui_task_runner,
    uint32_t font_initialization_data) {
  {
    std::scoped_lock lock(mutex_);
    if (state_ == State::kReady) {
      return;
    }

    pending_engines_.push_back(PendingEngine{engine, ui_task_runner});
    if (state_ == State::kLoading) {
      return;
    }
    state_ = State::kLoading;
  }

  auto font_collection = FontCollection::Instance();
  io_task_runner->PostTask([this, font_collection, font_initialization_data,
                            installer_ui_task_runner =
                                std::move(ui_task_runner)]() mutable {
    auto default_font_manager =
        txt::GetDefaultFontManager(font_initialization_data);
    if (!default_font_manager) {
      FML_LOG(ERROR) << "Failed to prefetch the default font manager";
      std::scoped_lock lock(mutex_);
      state_ = State::kNotStarted;
      return;
    }

    installer_ui_task_runner->PostTask([this, font_collection,
                                        default_font_manager = std::move(
                                            default_font_manager)]() mutable {
      // FontCollection is process-wide, so install the manager and
      // invalidate its caches exactly once.
      font_collection->SetDefaultFontManager(std::move(default_font_manager));
      font_collection->ClearFontFamilyCache();

      std::vector<PendingEngine> pending_engines;
      {
        std::scoped_lock lock(mutex_);
        state_ = State::kReady;
        pending_engines.swap(pending_engines_);
      }

      for (auto& pending : pending_engines) {
        pending.ui_task_runner->PostTask([engine = pending.engine]() mutable {
          if (engine) {
            engine->OnDefaultFontManagerReady();
          }
        });
      }
    });
  });
}

}  // namespace clay
