// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/no_destructor.h"
#include "core/public/jsb/native_module_factory.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/js/bindings/modules/lynx_module_manager.h"
#include "devtool/embedder/common/debugger_embedder.h"
#include "devtool/embedder/common/devtools_embedder.h"
#include "devtool/embedder/core/lynx_devtool_set_module.h"
#include "platform/embedder/lynx_devtool/devtool_env_embedder.h"
#include "platform/embedder/lynx_devtool/lynx_devtool_provider.h"
#include "third_party/debug_router/src/debug_router/native/core/debug_router_core.h"

namespace lynx {
namespace embedder {
namespace {

class BuiltinEmbedderDevToolProvider final : public EmbedderDevToolProvider {
 public:
  std::unique_ptr<EmbedderDevTool> Create(
      devtool::LynxDevToolProxy* proxy) override {
    auto& env = DevToolEnvEmbedder::GetInstance();
    if (!env.IsLynxDebugEnabled() || !env.IsDevToolEnabled()) {
      return nullptr;
    }
    return std::make_unique<devtool::DevtoolsEmbedder>(proxy);
  }

  void InitializeModuleManager(
      runtime::js::LynxModuleManager* module_manager) override {
    auto factory = std::make_unique<runtime::NativeModuleFactory>();
    factory->Register(devtool::LynxDevToolSetModule::GetName(),
                      devtool::LynxDevToolSetModule::Create);
    module_manager->SetModuleFactory(std::move(factory));
  }

  void SetAppInfo(const char* name, const char* value) override {
    DevToolEnvEmbedder::GetInstance().SetAppInfo(name, value);
  }

  void SetDevToolEnabled(bool enabled) override {
    DevToolEnvEmbedder::GetInstance().EnableDevTool(enabled);
  }

  bool IsDevToolEnabled() override {
    return DevToolEnvEmbedder::GetInstance().IsDevToolEnabled();
  }

  int GetLocalPort() override {
    return debugrouter::core::DebugRouterCore::GetInstance().GetUSBPort();
  }

  bool ConnectDevTool(const char* url) override {
    std::unordered_map<std::string, std::string> options;
    return devtool::DebuggerEmbedder::ConnectDevtools(url, options);
  }

  void SetOpenCardCallback(lynx_env_open_card_callback callback,
                           void* context) override {
    if (!callback) {
      devtool::DebuggerEmbedder::SetOpenCardCallback(nullptr);
      return;
    }
    devtool::DebuggerEmbedder::SetOpenCardCallback(
        [callback, context](const std::string& url) {
          callback(context, url.c_str());
        });
  }

  void SetCloseCardCallback(lynx_env_close_card_callback callback,
                            void* context) override {
    if (!callback) {
      devtool::DebuggerEmbedder::SetCloseCardCallback(nullptr);
      return;
    }
    devtool::DebuggerEmbedder::SetCloseCardCallback(
        [callback, context]() { callback(context); });
  }

  void SetLogBoxEnabled(bool enabled) override {
    DevToolEnvEmbedder::GetInstance().SetDevToolSwitch(
        tasm::LynxEnv::kLynxEnableLogBox, enabled);
  }

  bool IsLogBoxEnabled() override {
    return DevToolEnvEmbedder::GetInstance().IsLogBoxEnabled();
  }
};

// This source is linked directly by the Embedder target in builtin mode.
const bool kBuiltinDevToolRegistered = []() {
  static base::NoDestructor<BuiltinEmbedderDevToolProvider> provider;
  RegisterBuiltinEmbedderDevToolProvider(provider.get());
  return true;
}();

}  // namespace
}  // namespace embedder
}  // namespace lynx
