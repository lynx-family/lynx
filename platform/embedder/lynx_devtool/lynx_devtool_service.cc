// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <utility>

#include "core/public/devtool/lynx_inspector_owner.h"
#include "core/public/jsb/native_module_factory.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/common/bindings/event/message_event.h"
#include "core/shell/lynx_shell.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "debug_router/native/core/debug_router_core.h"
#include "devtool/embedder/common/debugger_embedder.h"
#include "devtool/embedder/core/inspector_owner_embedder.h"
#include "devtool/embedder/core/lynx_devtool_set_module.h"
#include "platform/embedder/lynx_devtool/devtool_env_embedder.h"
#include "platform/embedder/lynx_devtool/lynx_devtool_service_factory.h"
#include "third_party/jsoncpp/include/json/json.h"

namespace lynx {
namespace embedder {

class EmbedderDevToolServiceImpl : public service::EmbedderDevToolService {
 public:
  std::shared_ptr<devtool::LynxInspectorOwner> CreateInspector(
      devtool::LynxDevToolProxy* proxy) override {
    auto owner = std::make_shared<devtool::InspectorOwnerEmbedder>();
    owner->Init(proxy, owner);
    return owner;
  }

  std::unique_ptr<runtime::NativeModuleFactory> CreateModuleFactory() override {
    auto factory = std::make_unique<runtime::NativeModuleFactory>();
    factory->Register(devtool::LynxDevToolSetModule::GetName(),
                      devtool::LynxDevToolSetModule::Create);
    return factory;
  }

  void OnReceiveMessageEvent(devtool::LynxInspectorOwner& inspector,
                             const runtime::MessageEvent& event) override {
    Json::Value message;
    message[tasm::kType] = event.type();
    message[tasm::kTimestamp] = event.time_stamp();
    message[tasm::kTarget] = event.GetTargetString();
    message[tasm::kOrigin] = event.GetOriginString();
    const auto* data = event.message();
    message[tasm::kData] = data ? data->str() : "";
    inspector.OnReceiveMessageEvent(message);
  }

  void DispatchMessageEvent(shell::LynxShell& shell,
                            const Json::Value& message) override {
    if (!message.isObject()) {
      return;
    }
    auto type = message.get(tasm::kType, "").asString();
    auto origin = runtime::ContextProxy::ConvertStringToContextType(
        message.get(tasm::kOrigin, "").asString());
    auto target = runtime::ContextProxy::ConvertStringToContextType(
        message.get(tasm::kTarget, "").asString());
    auto data_value = pub::PubValueFactoryDefault();
    auto event = fml::MakeRefCounted<runtime::MessageEvent>(
        type, origin, target,
        data_value.CreateString(message.get(tasm::kData, "").asString()));
    shell.DispatchMessageEvent(event);
  }

  bool IsLynxDebugEnabled() override {
    return DevToolEnvEmbedder::GetInstance().IsLynxDebugEnabled();
  }
  void EnableDevTool(bool enable) override {
    DevToolEnvEmbedder::GetInstance().EnableDevTool(enable);
  }
  bool IsDevToolEnabled() override {
    return DevToolEnvEmbedder::GetInstance().IsDevToolEnabled();
  }
  void EnableLogBox(bool enable) override {
    DevToolEnvEmbedder::GetInstance().SetDevToolSwitch(
        tasm::LynxEnv::kLynxEnableLogBox, enable);
  }
  bool IsLogBoxEnabled() override {
    return DevToolEnvEmbedder::GetInstance().IsLogBoxEnabled();
  }
  void SetAppInfo(const std::string& name, const std::string& value) override {
    DevToolEnvEmbedder::GetInstance().SetAppInfo(name, value);
  }
  int GetLocalPort() override {
    return debugrouter::core::DebugRouterCore::GetInstance().GetUSBPort();
  }
  bool Connect(
      const std::string& url,
      const std::unordered_map<std::string, std::string>& options) override {
    return devtool::DebuggerEmbedder::ConnectDevtools(url, options);
  }
  void SetOpenCardCallback(
      std::function<void(const std::string&)> callback) override {
    devtool::DebuggerEmbedder::SetOpenCardCallback(std::move(callback));
  }
  void SetCloseCardCallback(std::function<void()> callback) override {
    devtool::DebuggerEmbedder::SetCloseCardCallback(std::move(callback));
  }
};

service::EmbedderDevToolService* CreateEmbedderDevToolService() {
  return new EmbedderDevToolServiceImpl();
}

}  // namespace embedder
}  // namespace lynx
