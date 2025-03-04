// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_MESSAGE_SENDER_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_MESSAGE_SENDER_H_
#include <memory>
#include <string>

#include "base/include/closure.h"
#include "third_party/jsoncpp/include/json/json.h"

namespace lynx {
namespace devtool {
// refer to chromium
constexpr int kInspectorErrorCode = -32601;
/**
 * CDP domain agents uses this MessageSender instance
 * to send the execution result of CDP messages. ViewMessageChannel
 * and GlobalMessageChannel implement this interface, allowing the
 * execution result of CDP messages to be sent to the DebugRouter.
 */
class MessageSender {
 public:
  void SendOKResponse(int64_t id) {
    Json::Value res;
    res["result"] = Json::Value(Json::ValueType::objectValue);
    res["id"] = id;
    SendMessage("CDP", res);
  }

  void SendErrorResponse(int64_t id, const std::string& error) {
    Json::Value res;
    res["error"]["code"] = kInspectorErrorCode;
    res["error"]["message"] = error;
    res["id"] = id;
    SendMessage("CDP", res);
  }

  /**
   * Sends a message of the specified type with the given content in Json::Value
   * format.
   *
   * @param type The type of message being sent, specified as a string.
   * @param msg The content of the message, formatted as a Json::Value object.
   *
   * @return This function does not return a value.
   *
   * @note It is recommended to use this method with Json::Value as the
   * parameter. In the future, once DebugRouter is refactored, the second
   * overload with std::string parameter will be removed. All message passing
   * will be streamlined to use Json::Value parameters at the C++ level.
   */
  virtual void SendMessage(const std::string& type, const Json::Value& msg) = 0;

  /**
   * Sends a message of the specified type with the given content as a JSON
   * string.
   *
   * @param type The type of message being sent, specified as a string.
   * @param msg The content of the message, which must be a JSON-formatted
   * string.
   *
   * @return This function does not return a value.
   *
   * @note Sends a message of the specified type with the given content in
   * JSON-formatted string format. This method exists temporarily due to the
   * current state of DebugRouter, which has not yet been refactored. Strings
   * that come from the platform layer and are already JSON-formatted will be
   * automatically converted to Json::Value Without the presence of this method.
   * Therefore, this function is needed to avoid redundant JSON formatting.
   * Future refactoring will remove this overload, so it is recommended to use
   * the Json::Value version of SendMessage instead.
   */
  virtual void SendMessage(const std::string& type, const std::string& msg) = 0;
  virtual ~MessageSender() = default;
};
}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_MESSAGE_SENDER_H_
