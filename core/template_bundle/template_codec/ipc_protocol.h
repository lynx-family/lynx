// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_IPC_PROTOCOL_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_IPC_PROTOCOL_H_

#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "core/template_bundle/template_codec/cbor_codec.h"

namespace lynx {
namespace ipc {

// Protocol constants
constexpr uint32_t kMaxFrameSize = 64 * 1024 * 1024;  // 64MB max frame
constexpr uint32_t kMaxMetaSize = 1024 * 1024;        // 1MB max meta
constexpr uint8_t kProtocolVersion = 1;

// Error codes following the spec
namespace error {
// Protocol layer errors
constexpr const char* kInvalidFrame = "PROTO.INVALID_FRAME";
constexpr const char* kFrameTooLarge = "PROTO.FRAME_TOO_LARGE";
constexpr const char* kInvalidMetaJson = "PROTO.INVALID_META_JSON";
constexpr const char* kUnsupportedProtoVersion =
    "PROTO.UNSUPPORTED_PROTO_VERSION";
constexpr const char* kUnsupportedContentType =
    "PROTO.UNSUPPORTED_CONTENT_TYPE";

// IPC layer errors
constexpr const char* kMethodNotFound = "IPC.METHOD_NOT_FOUND";
constexpr const char* kInvalidParams = "IPC.INVALID_PARAMS";
constexpr const char* kDeadlineExceeded = "IPC.DEADLINE_EXCEEDED";
constexpr const char* kInternalError = "IPC.INTERNAL";
}  // namespace error

// Error info structure
struct ErrorInfo {
  std::string code;
  std::string message;
  cbor::Value data;  // Optional additional data

  ErrorInfo() = default;
  ErrorInfo(const std::string& c, const std::string& m) : code(c), message(m) {}
  ErrorInfo(const std::string& c, const std::string& m, cbor::Value d)
      : code(c), message(m), data(std::move(d)) {}
};

// Frame header structure
struct FrameHeader {
  uint32_t meta_len = 0;
  uint32_t body_len = 0;
};

// Meta structure for request/response
struct Meta {
  std::string proto;          // Protocol name, e.g. "stdio-ipc"
  int64_t proto_version = 0;  // Protocol version
  std::string type;           // "request" or "response"
  std::string id;             // Request/response ID (string form)
  bool id_is_number = false;
  int64_t id_number = 0;
  std::string method;        // Method name (for request)
  std::string content_type;  // Content type (e.g., "application/cbor")
  std::string status;        // "ok" or "error" (for response)
  ErrorInfo error;           // Error info (for response with status="error")
  // Optional response extension fields for encoder compatibility.
  bool has_encoder_status = false;
  int64_t encoder_status = 0;
  std::string error_msg;
  std::string lepus_code;
  std::string lepus_debug;
  std::string section_size;

  // Serialize to JSON string
  std::string ToJson() const;
  // Deserialize from JSON string
  static Meta FromJson(const std::string& json);
};

// Request structure
struct Request {
  Meta meta;
  cbor::Value body;
};

// Response structure
struct Response {
  Meta meta;
  cbor::Value body;  // Empty when status="error"
};

// Handler function type
using RequestHandler = std::function<Response(const Request&)>;

// IPC Protocol class
class IpcProtocol {
 public:
  IpcProtocol();
  ~IpcProtocol();

  // Initialize with input/output streams
  void Init(std::istream* in, std::ostream* out);

  // Register a handler for a method
  void RegisterHandler(const std::string& method, RequestHandler handler);

  // Run the IPC loop (blocking)
  void Run();

  // Stop the IPC loop
  void Stop();

  // Send a response
  void SendResponse(const Response& response);

  // Create a success response
  static Response CreateSuccessResponse(const std::string& id,
                                        const std::string& content_type,
                                        cbor::Value body);

  // Create an error response
  static Response CreateErrorResponse(const std::string& id,
                                      const ErrorInfo& error);

 private:
  enum class ReadFrameError {
    kNone,
    kInvalidFrame,
    kFrameTooLarge,
  };

  // Read a frame from input
  bool ReadFrame(FrameHeader& header, std::string& meta_json,
                 std::vector<uint8_t>& body, ReadFrameError& error);

  // Write a frame to output
  bool WriteFrame(const Meta& meta, const std::vector<uint8_t>& body);

  // Process a request
  void ProcessRequest(const Request& request);

  std::istream* in_ = nullptr;
  std::ostream* out_ = nullptr;
  std::map<std::string, RequestHandler> handlers_;
  bool running_ = false;
  uint32_t request_counter_ = 0;
};

}  // namespace ipc
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_IPC_PROTOCOL_H_
