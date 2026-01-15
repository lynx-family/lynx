// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BINDINGS_RESOURCE_RESPONSE_HANDLER_IN_JS_H_
#define CORE_RUNTIME_JS_BINDINGS_RESOURCE_RESPONSE_HANDLER_IN_JS_H_

#include <future>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/runtime/common/bindings/resource/response_handler_proxy.h"
#include "core/runtime/js/jsi/jsi.h"

namespace lynx {
namespace piper {

class App;

class ResponseHandlerInJS
    : public HostObject,
      public runtime::ResponseHandlerProxy,
      public std::enable_shared_from_this<ResponseHandlerInJS> {
 public:
  ResponseHandlerInJS(Delegate&, const std::string& url,
                      const std::shared_ptr<
                          runtime::ResponsePromise<tasm::BundleResourceInfo>>&,
                      std::weak_ptr<App>);

  virtual ~ResponseHandlerInJS() override = default;

  virtual Value get(Runtime*, const PropNameID& name) override;
  virtual void set(Runtime*, const PropNameID& name,
                   const Value& value) override;
  virtual std::vector<PropNameID> getPropertyNames(Runtime& rt) override;

  void AddResourceListener(
      base::MoveOnlyClosure<void, tasm::BundleResourceInfo> closure) override;

 private:
  Value WaitingForResponse(Runtime& rt);
  Value AddListenerForResponse(Runtime& rt);

  static piper::Value ConvertBundleInfoToPiperValue(
      std::shared_ptr<App> native_app,
      const tasm::BundleResourceInfo& bundle_info);

  std::weak_ptr<App> native_app_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_BINDINGS_RESOURCE_RESPONSE_HANDLER_IN_JS_H_
