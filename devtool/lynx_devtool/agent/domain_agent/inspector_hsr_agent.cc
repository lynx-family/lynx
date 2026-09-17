// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_hsr_agent.h"

#include <utility>

#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator_base.h"

namespace lynx {
namespace devtool {
namespace {

// A move-only callback owns the response until completion or abandonment.
class PendingHSRResponse {
 public:
  PendingHSRResponse(std::shared_ptr<CDPResponder> responder,
                     fml::RefPtr<fml::TaskRunner> runner,
                     HSRScriptRequest::Operation operation)
      : responder_(std::move(responder)),
        runner_(std::move(runner)),
        operation_(operation) {}
  PendingHSRResponse(PendingHSRResponse&&) = default;

  ~PendingHSRResponse() {
    if (responder_) {
      (*this)(Json::Value(), "HSR request was abandoned before completion");
    }
  }

  void operator()(Json::Value&& result, const std::string& error) const {
    // Queue every reply so destruction on this thread cannot overtake a
    // completion already posted by another thread. CDPResponder replies once.
    runner_->PostTask([responder = responder_, operation = operation_,
                       result = std::move(result), error]() mutable {
      if (!error.empty()) {
        responder->SendError(CDPErrorCode::ServerError, error);
        return;
      }
      bool valid = result.isObject();
      if (valid && operation == HSRScriptRequest::Operation::kEvaluate) {
        valid = result["valueType"].isString();
        if (valid) {
          const auto type = result["valueType"].asString();
          valid = (type == "json" && result.isMember("value")) ||
                  (type == "undefined" && !result.isMember("value"));
        }
      } else if (valid) {
        valid = result.empty();
      }
      if (!valid) {
        responder->SendError(CDPErrorCode::InternalError,
                             "Invalid HSR result JSON");
        return;
      }
      responder->SendSuccess(std::move(result));
    });
  }

 private:
  std::shared_ptr<CDPResponder> responder_;
  fml::RefPtr<fml::TaskRunner> runner_;
  HSRScriptRequest::Operation operation_;
};

}  // namespace

InspectorHSRAgent::InspectorHSRAgent()
    : InspectorHSRAgent(GlobalDevToolPlatformFacade::GetInstance()) {}

InspectorHSRAgent::InspectorHSRAgent(GlobalDevToolPlatformFacade& facade)
    : facade_(facade) {}

void InspectorHSRAgent::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  const std::string method = message["method"].asString();
  HSRScriptRequest request;
  std::string error;
  bool valid;
  if (method == "HSR.loadScript") {
    valid = ParseHSRLoadScript(message["params"], request, error);
  } else if (method == "HSR.evaluate") {
    valid = ParseHSREvaluate(message["params"], request, error);
  } else {
    responder->SendError(CDPErrorCode::MethodNotFound,
                         "'" + method + "' wasn't found");
    return;
  }
  if (!valid) {
    responder->SendError(CDPErrorCode::InvalidParams, error);
    return;
  }
  Execute(responder, std::move(request));
}

void InspectorHSRAgent::Execute(const std::shared_ptr<CDPResponder>& responder,
                                HSRScriptRequest&& request) {
  auto runner = LynxDevToolMediatorBase::GetDevToolsThread().GetTaskRunner();
  if (!runner) {
    responder->SendError(CDPErrorCode::ServerError,
                         "Cannot find default task runner");
    return;
  }
  GlobalDevToolPlatformFacade::HSRScriptCallback callback(
      PendingHSRResponse(responder, runner, request.operation));
  fml::TaskRunner::RunNowOrPostTask(
      runner, [facade = &facade_, request = std::move(request),
               callback = std::move(callback)]() mutable {
        facade->HandleHSRScript(std::move(request), std::move(callback));
      });
}

}  // namespace devtool
}  // namespace lynx
