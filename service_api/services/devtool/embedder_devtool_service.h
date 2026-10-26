// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SERVICE_API_SERVICES_DEVTOOL_EMBEDDER_DEVTOOL_SERVICE_H_
#define SERVICE_API_SERVICES_DEVTOOL_EMBEDDER_DEVTOOL_SERVICE_H_

#include <service_api/service_api.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace Json {
class Value;
}

namespace lynx {
namespace devtool {
class LynxDevToolProxy;
class LynxInspectorOwner;
}  // namespace devtool
namespace runtime {
class MessageEvent;
class NativeModuleFactory;
}  // namespace runtime
namespace shell {
class LynxShell;
}  // namespace shell
namespace service {

// Optional debugging adapter for C++ renderers. The registry owns the service;
// each renderer owns the inspector returned by CreateInspector().
class LYNX_SERVICE_DECLARE(EmbedderDevToolService)
    : public BaseService<EmbedderDevToolService> {
 public:
  virtual std::shared_ptr<devtool::LynxInspectorOwner> CreateInspector(
      devtool::LynxDevToolProxy* proxy) = 0;
  virtual std::unique_ptr<runtime::NativeModuleFactory>
  CreateModuleFactory() = 0;
  // Message conversion belongs to the optional debug implementation.
  virtual void OnReceiveMessageEvent(devtool::LynxInspectorOwner& inspector,
                                     const runtime::MessageEvent& event) = 0;
  virtual void DispatchMessageEvent(shell::LynxShell& shell,
                                    const Json::Value& message) = 0;
  virtual bool IsLynxDebugEnabled() = 0;
  virtual void EnableDevTool(bool enable) = 0;
  virtual bool IsDevToolEnabled() = 0;
  virtual void EnableLogBox(bool enable) = 0;
  virtual bool IsLogBoxEnabled() = 0;
  virtual void SetAppInfo(const std::string& name,
                          const std::string& value) = 0;
  virtual int GetLocalPort() = 0;
  virtual bool Connect(
      const std::string& url,
      const std::unordered_map<std::string, std::string>& options) = 0;
  virtual void SetOpenCardCallback(
      std::function<void(const std::string&)> callback) = 0;
  virtual void SetCloseCardCallback(std::function<void()> callback) = 0;
};

}  // namespace service
}  // namespace lynx

#endif  // SERVICE_API_SERVICES_DEVTOOL_EMBEDDER_DEVTOOL_SERVICE_H_
