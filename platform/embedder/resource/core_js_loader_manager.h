// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_RESOURCE_CORE_JS_LOADER_MANAGER_H_
#define PLATFORM_EMBEDDER_RESOURCE_CORE_JS_LOADER_MANAGER_H_

#include <memory>

namespace lynx {
class ICoreJsLoader {
 public:
  virtual ~ICoreJsLoader();

  virtual const char* GetCoreJs() = 0;
  virtual bool JsCoreUpdated() = 0;
  virtual void CheckUpdate() = 0;
};

class CoreJsLoaderManager {
 public:
  static CoreJsLoaderManager* GetInstance();

  ICoreJsLoader* GetLoader();
  void SetLoader(std::unique_ptr<ICoreJsLoader> loader);

  CoreJsLoaderManager() = default;

  CoreJsLoaderManager(const CoreJsLoaderManager&) = delete;
  CoreJsLoaderManager& operator=(const CoreJsLoaderManager&) = delete;
  CoreJsLoaderManager(CoreJsLoaderManager&&) = delete;
  CoreJsLoaderManager& operator=(CoreJsLoaderManager&&) = delete;

 private:
  std::unique_ptr<ICoreJsLoader> loader_;
};
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_RESOURCE_CORE_JS_LOADER_MANAGER_H_
