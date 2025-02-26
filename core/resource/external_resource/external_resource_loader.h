// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RESOURCE_EXTERNAL_RESOURCE_EXTERNAL_RESOURCE_LOADER_H_
#define CORE_RESOURCE_EXTERNAL_RESOURCE_EXTERNAL_RESOURCE_LOADER_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "core/public/lynx_resource_loader.h"
#include "core/resource/external_resource/external_resource_loader.h"
#include "core/runtime/piper/js/lynx_runtime.h"
#include "core/shell/lynx_actor_specialization.h"
#include "core/shell/lynx_engine.h"

namespace lynx {
namespace shell {

struct ExternalResourceInfo {
  std::vector<uint8_t> data;
  int32_t err_code;
  std::string err_msg;

  ExternalResourceInfo() = default;

  ExternalResourceInfo(std::vector<uint8_t> data, int32_t err_code,
                       std::string err_msg)
      : data(std::move(data)),
        err_code(err_code),
        err_msg(std::move(err_msg)) {}

  ExternalResourceInfo(int32_t err_code, std::string err_msg)
      : err_code(err_code), err_msg(std::move(err_msg)) {}

  bool Success() { return err_code == 0; }
};

class ExternalResourceLoader
    : public std::enable_shared_from_this<ExternalResourceLoader> {
 public:
  ExternalResourceLoader() = default;
  virtual ~ExternalResourceLoader() = default;

  ExternalResourceLoader(
      const std::shared_ptr<pub::LynxResourceLoader>& resource_loader)
      : resource_loader_(resource_loader) {}

  ExternalResourceLoader(ExternalResourceLoader&&) = default;
  ExternalResourceLoader& operator=(ExternalResourceLoader&&) = default;

  ExternalResourceInfo LoadScript(const std::string& url, long timeout);

  void LoadScriptAsync(const std::string& url, int32_t callback_id);

  void LoadLazyBundle(const std::string& url, int32_t callback_id);

  /**
   * @param url url of lazy bundle
   * @param callback_id id of the callback which should be triggered after
   * loading
   * @param ids ids of the components which should be updated after loading
   */
  void LoadLazyBundle(const std::string& url, int32_t callback_id,
                      const std::vector<std::string>& ids);

  // Load lynx_core.js and assets
  std::vector<uint8_t> LoadJSSource(const std::string& url);

  inline void SetEngineActor(
      const std::shared_ptr<LynxActor<LynxEngine>>& engine_actor) {
    engine_actor_ = engine_actor;
  }

  inline void SetRuntimeActor(
      const std::shared_ptr<LynxActor<runtime::LynxRuntime>>& runtime_actor) {
    runtime_actor_ = runtime_actor;
  }

 private:
  std::shared_ptr<pub::LynxResourceLoader> resource_loader_ = nullptr;
  std::weak_ptr<LynxActor<LynxEngine>> engine_actor_;
  std::weak_ptr<LynxActor<runtime::LynxRuntime>> runtime_actor_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_RESOURCE_EXTERNAL_RESOURCE_EXTERNAL_RESOURCE_LOADER_H_
