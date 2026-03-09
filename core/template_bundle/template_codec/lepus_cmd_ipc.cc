// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef __EMSCRIPTEN__

#include "core/template_bundle/template_codec/lepus_cmd_ipc.h"

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "core/template_bundle/lynx_template_bundle_converter.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_reader.h"
#include "core/template_bundle/template_codec/binary_encoder/encoder.h"
#include "core/template_bundle/template_codec/ipc_protocol.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace lepus_cmd {

namespace {

std::string CborValueToDebugString(const cbor::Value& value);

std::string CborMapToDebugString(
    const std::map<std::string, cbor::Value>& map) {
  std::ostringstream oss;
  oss << "{";
  bool first = true;
  for (const auto& kv : map) {
    if (!first) {
      oss << ", ";
    }
    first = false;
    oss << "\"" << kv.first << "\": " << CborValueToDebugString(kv.second);
  }
  oss << "}";
  return oss.str();
}

std::string CborArrayToDebugString(const std::vector<cbor::Value>& array) {
  std::ostringstream oss;
  oss << "[";
  for (size_t i = 0; i < array.size(); ++i) {
    if (i > 0) {
      oss << ", ";
    }
    oss << CborValueToDebugString(array[i]);
  }
  oss << "]";
  return oss.str();
}

std::string CborValueToDebugString(const cbor::Value& value) {
  if (value.IsNull()) {
    return "null";
  }
  if (value.IsBool()) {
    return value.AsBool() ? "true" : "false";
  }
  if (value.IsUnsigned()) {
    return std::to_string(value.AsUnsigned());
  }
  if (value.IsInt()) {
    return std::to_string(value.AsInt());
  }
  if (value.IsText()) {
    return "\"" + value.AsText() + "\"";
  }
  if (value.IsBytes()) {
    return "<bytes len=" + std::to_string(value.AsBytes().size()) + ">";
  }
  if (value.IsArray()) {
    return CborArrayToDebugString(value.AsArray());
  }
  if (value.IsMap()) {
    return CborMapToDebugString(value.AsMap());
  }
  return "<unknown>";
}

std::string ResponseToDebugString(const ipc::Response& response) {
  std::ostringstream oss;
  oss << "status=" << response.meta.status;
  if (response.meta.has_encoder_status) {
    oss << ", encoder_status=" << response.meta.encoder_status;
  }
  if (!response.meta.error_msg.empty()) {
    oss << ", error_msg=\"" << response.meta.error_msg << "\"";
  }
  if (!response.meta.section_size.empty()) {
    oss << ", section_size=\"" << response.meta.section_size << "\"";
  }
  if (response.meta.status == "error") {
    oss << ", error_code=" << response.meta.error.code << ", error_message=\""
        << response.meta.error.message << "\"";
  } else {
    oss << ", body=" << CborValueToDebugString(response.body);
  }
  return oss.str();
}

}  // namespace

void RunIpcMode() {
  ipc::IpcProtocol protocol;
  protocol.Init(&std::cin, &std::cout);

  auto encode_handler = [](const ipc::Request& request) -> ipc::Response {
    std::cerr << "[ipc][encode] input: " << CborValueToDebugString(request.body)
              << std::endl;
    auto log_and_return = [](const ipc::Response& response) {
      std::cerr << "[ipc][encode] output: " << ResponseToDebugString(response)
                << std::endl;
      return response;
    };

    if (request.meta.content_type != "application/json") {
      return log_and_return(ipc::IpcProtocol::CreateErrorResponse(
          request.meta.id, ipc::ErrorInfo(ipc::error::kInvalidParams,
                                          "encode expects application/json")));
    }
    if (!request.body.IsText()) {
      return log_and_return(ipc::IpcProtocol::CreateErrorResponse(
          request.meta.id, ipc::ErrorInfo(ipc::error::kInvalidParams,
                                          "json body must be text")));
    }

    rapidjson::Document body_doc;
    body_doc.Parse(request.body.AsText().c_str());
    if (body_doc.HasParseError() || !body_doc.IsObject()) {
      return log_and_return(ipc::IpcProtocol::CreateErrorResponse(
          request.meta.id, ipc::ErrorInfo(ipc::error::kInvalidParams,
                                          "json body must be object")));
    }

    try {
      auto result = tasm::encode(request.body.AsText());
      if (result.status != 0) {
        auto error = ipc::IpcProtocol::CreateErrorResponse(
            request.meta.id,
            ipc::ErrorInfo(ipc::error::kInternalError,
                           "encode failed: " + result.error_msg));
        error.meta.content_type = "application/octet-stream";
        error.meta.has_encoder_status = true;
        error.meta.encoder_status = result.status;
        error.meta.error_msg = result.error_msg;
        error.meta.lepus_code = "";
        error.meta.lepus_debug = "";
        error.meta.section_size = "";
        return log_and_return(error);
      }

      auto response = ipc::IpcProtocol::CreateSuccessResponse(
          request.meta.id, "application/octet-stream",
          cbor::Value(result.buffer));
      response.meta.has_encoder_status = true;
      response.meta.encoder_status = result.status;
      response.meta.error_msg = "";
      response.meta.lepus_code = "";
      response.meta.lepus_debug = "";
      response.meta.section_size = std::to_string(result.buffer.size());
      return log_and_return(response);
    } catch (const std::exception& e) {
      auto error = ipc::IpcProtocol::CreateErrorResponse(
          request.meta.id,
          ipc::ErrorInfo(ipc::error::kInternalError, e.what()));
      error.meta.content_type = "application/octet-stream";
      return log_and_return(error);
    }
  };

  protocol.RegisterHandler("encode", encode_handler);
  protocol.RegisterHandler("tasm.encode", encode_handler);

  protocol.RegisterHandler(
      "decode", [](const ipc::Request& request) -> ipc::Response {
        std::cerr << "[ipc][decode] input: "
                  << CborValueToDebugString(request.body) << std::endl;
        auto log_and_return = [](const ipc::Response& response) {
          std::cerr << "[ipc][decode] output: "
                    << ResponseToDebugString(response) << std::endl;
          return response;
        };

        if (request.meta.content_type != "application/cbor") {
          return log_and_return(ipc::IpcProtocol::CreateErrorResponse(
              request.meta.id,
              ipc::ErrorInfo(ipc::error::kInvalidParams,
                             "decode expects application/cbor")));
        }
        if (!request.body.IsMap()) {
          return log_and_return(ipc::IpcProtocol::CreateErrorResponse(
              request.meta.id, ipc::ErrorInfo(ipc::error::kInvalidParams,
                                              "request body must be map")));
        }

        const auto& body_map = request.body.AsMap();
        const auto binary_it = body_map.find("binary");
        if (binary_it == body_map.end() || !binary_it->second.IsBytes()) {
          return log_and_return(ipc::IpcProtocol::CreateErrorResponse(
              request.meta.id,
              ipc::ErrorInfo(ipc::error::kInvalidParams, "missing binary")));
        }

        try {
          auto reader = tasm::LynxBinaryReader::CreateLynxBinaryReader(
              binary_it->second.AsBytes());
          if (!reader.Decode()) {
            return log_and_return(ipc::IpcProtocol::CreateErrorResponse(
                request.meta.id,
                ipc::ErrorInfo(ipc::error::kInternalError,
                               "decode failed: " + reader.error_message_)));
          }

          auto bundle = reader.GetTemplateBundle();
          auto result = tasm::LynxTemplateBundleConverter::
              ConvertTemplateBundleToSerializedString(bundle);
          cbor::Value response_body(result);
          return log_and_return(ipc::IpcProtocol::CreateSuccessResponse(
              request.meta.id, "application/json", std::move(response_body)));
        } catch (const std::exception& e) {
          return log_and_return(ipc::IpcProtocol::CreateErrorResponse(
              request.meta.id,
              ipc::ErrorInfo(ipc::error::kInternalError, e.what())));
        }
      });

  protocol.Run();
}

}  // namespace lepus_cmd
}  // namespace lynx

#endif  // __EMSCRIPTEN__
