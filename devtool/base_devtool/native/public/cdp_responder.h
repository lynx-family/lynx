// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_CDP_RESPONDER_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_CDP_RESPONDER_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "devtool/base_devtool/native/public/cdp_error_code.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "third_party/jsoncpp/include/json/json.h"

namespace lynx {
namespace devtool {

/**
 * CDPResponder owns the lifecycle of a single CDP command response.
 *
 * A CDP request travels from DevToolMessageDispatcher::DispatchCDPMessage down
 * to a domain agent and finally produces one response. Instead of passing the
 * raw MessageSender around and letting every agent hand-assemble the envelope,
 * DispatchCDPMessage constructs a CDPResponder and passes it down as a
 * shared_ptr. Agents only fill in the response body (via Result()) or report an
 * error (via SendErrorResponse()); they never build the envelope themselves.
 *
 * Because the responder holds the sender and the response state, the last
 * shared_ptr to be released triggers the destructor, which emits a fallback
 * response if none has been sent yet. This guarantees that every CDP request is
 * answered by exactly one, uniformly formatted response, regardless of whether
 * the command completes synchronously or asynchronously.
 *
 * A CDP response envelope has one of two shapes:
 *   // OK response
 *   { "id": <int64>, "result": <object> }
 *   // Error response
 *   { "id": <int64>, "error": { "code": <int32>, "message": <string> } }
 *
 * CDPResponder is intentionally lightweight and implemented inline in this
 * header.
 */
class CDPResponder {
 public:
  CDPResponder(const std::shared_ptr<MessageSender>& sender,
               std::optional<int64_t> id)
      : sender_(sender), id_(id), result_(Json::objectValue) {}

  // Not copyable: a responder represents a single, one-shot response.
  CDPResponder(const CDPResponder&) = delete;
  CDPResponder& operator=(const CDPResponder&) = delete;

  // Emits a fallback OK response when neither Send() nor SendErrorResponse()
  // has been called and the sender has not been retrieved for compatibility
  // use. This is what guarantees "exactly one response per request".
  ~CDPResponder() {
    if (sender_) {
      Send();
    }
  }

  // Hands the underlying sender to legacy call sites that still assemble the
  // envelope themselves. After this call the responder no longer owns the
  // sender and will not emit any (including fallback) response.
  std::shared_ptr<MessageSender> RetrieveSender() { return std::move(sender_); }

  // The only way to populate the OK response body, so that no field is missed.
  Json::Value& Result() { return result_; }

  // Sends the OK response { id, result } and releases the sender so the
  // response can be emitted at most once.
  void Send() {
    if (!sender_) {
      return;
    }
    Json::Value response(Json::objectValue);
    if (id_.has_value()) {
      response["id"] = static_cast<Json::Int64>(id_.value());
    }
    response["result"] = result_;
    sender_->SendMessage("CDP", response);
    sender_.reset();
  }

  // Sends the error response { id, error: { code, message } } and releases the
  // sender. |message| should be a short, complete description and is sent
  // verbatim (following Chromium's crdtp model, the code carries the
  // machine-readable classification and is not derived into the message). When
  // |message| is empty it falls back to the error type name for the code, e.g.
  // "Method not found" or "Internal error".
  void SendErrorResponse(CDPErrorCode code, const std::string& message = "") {
    if (!sender_) {
      return;
    }
    Json::Value response(Json::objectValue);
    if (id_.has_value()) {
      response["id"] = static_cast<Json::Int64>(id_.value());
    } else {
      response["id"] = Json::Value(Json::nullValue);
    }
    Json::Value error(Json::objectValue);
    error["code"] = static_cast<int>(code);
    error["message"] = message.empty() ? ErrorTypeName(code) : message;
    response["error"] = error;
    sender_->SendMessage("CDP", response);
    sender_.reset();
  }

 private:
  // Human-readable type name for a code, used as the default message when the
  // caller does not provide one.
  static const char* ErrorTypeName(CDPErrorCode code) {
    switch (code) {
      case CDPErrorCode::kParseError:
        return "Parse error";
      case CDPErrorCode::kInvalidRequest:
        return "Invalid request";
      case CDPErrorCode::kMethodNotFound:
        return "Method not found";
      case CDPErrorCode::kInvalidParams:
        return "Invalid params";
      case CDPErrorCode::kInternalError:
        return "Internal error";
      case CDPErrorCode::kServerError:
        return "Server error";
    }
    return "Internal error";
  }

  std::shared_ptr<MessageSender> sender_;  // one-shot, reset after sending
  std::optional<int64_t> id_;              // cdp command id
  Json::Value result_;                     // body of the OK response
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_CDP_RESPONDER_H_
