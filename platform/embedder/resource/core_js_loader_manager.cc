// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/resource/core_js_loader_manager.h"

#include <utility>

#include "base/include/no_destructor.h"

namespace lynx {
ICoreJsLoader::~ICoreJsLoader() = default;

CoreJsLoaderManager* CoreJsLoaderManager::GetInstance() {
  static lynx::base::NoDestructor<CoreJsLoaderManager> instance;
  return instance.get();
}

ICoreJsLoader* CoreJsLoaderManager::GetLoader() { return loader_.get(); }

void CoreJsLoaderManager::SetLoader(std::unique_ptr<ICoreJsLoader> loader) {
  loader_ = std::move(loader);
}
}  // namespace lynx
