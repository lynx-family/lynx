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

// Keeps CDPResponder's destructor from acknowledging an abandoned request.
// All terminal replies, including callback destruction, use the same runner.
class PendingHSRResponse {
 public:
  PendingHSRResponse(std::shared_ptr<CDPResponder> responder,
                     fml::RefPtr<fml::TaskRunner> runner,
                     HSRScriptRequest::Operation operation)
      : responder_(std::move(responder)),
        runner_(std::move(runner)),
        operation_(operation) {}

  ~PendingHSRResponse() {
    fml::TaskRunner::RunNowOrPostTask(runner_, [responder = responder_] {
      responder->SendError(CDPErrorCode::ServerError,
                           "HSR request was abandoned before completion");
    });
  }

  void Complete(const std::string& result_json, const std::string& error) {
    fml::TaskRunner::RunNowOrPostTask(
        runner_,
        [responder = responder_, operation = operation_, result_json, error] {
          if (!error.empty()) {
            responder->SendError(CDPErrorCode::ServerError, error);
            return;
          }
          Json::Value result;
          Json::CharReaderBuilder builder;
          Json::CharReaderBuilder::strictMode(&builder.settings_);
          std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
          bool valid = reader->parse(result_json.data(),
                                     result_json.data() + result_json.size(),
                                     &result, nullptr) &&
                       result.isObject();
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
    : facade_(facade) {
  functions_map_["HSR.loadScript"] = &InspectorHSRAgent::LoadScript;
  functions_map_["HSR.evaluate"] = &InspectorHSRAgent::Evaluate;
}

void InspectorHSRAgent::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  const std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    responder->SendError(CDPErrorCode::MethodNotFound,
                         "'" + method + "' wasn't found");
    return;
  }
  (this->*(iter->second))(responder, message["params"]);
}

void InspectorHSRAgent::LoadScript(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  HSRScriptRequest request;
  std::string error;
  if (!ParseHSRLoadScript(params, request, error)) {
    responder->SendError(CDPErrorCode::InvalidParams, error);
    return;
  }
  Execute(responder, std::move(request));
}

void InspectorHSRAgent::Evaluate(const std::shared_ptr<CDPResponder>& responder,
                                 const Json::Value& params) {
  HSRScriptRequest request;
  std::string error;
  if (!ParseHSREvaluate(params, request, error)) {
    responder->SendError(CDPErrorCode::InvalidParams, error);
    return;
  }
  Execute(responder, std::move(request));
}

void InspectorHSRAgent::Execute(const std::shared_ptr<CDPResponder>& responder,
                                HSRScriptRequest request) {
  auto runner = LynxDevToolMediatorBase::GetDevToolsThread().GetTaskRunner();
  if (!runner) {
    responder->SendError(CDPErrorCode::ServerError,
                         "Cannot find default task runner");
    return;
  }
  auto pending = std::make_shared<PendingHSRResponse>(responder, runner,
                                                      request.operation);
  fml::TaskRunner::RunNowOrPostTask(
      runner,
      [facade = &facade_, request = std::move(request), pending]() mutable {
        facade->HandleHSRScript(
            std::move(request),
            [pending](const std::string& result, const std::string& error) {
              pending->Complete(result, error);
            });
      });
}

}  // namespace devtool
}  // namespace lynx
