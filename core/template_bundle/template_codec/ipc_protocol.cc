// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/ipc_protocol.h"

#include <cstring>
#include <iomanip>
#include <sstream>
#include <utility>

#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace ipc {

namespace {

constexpr const char* kProtoName = "stdio-ipc";
constexpr const char* kContentTypeJson = "application/json";
constexpr const char* kContentTypeCbor = "application/cbor";
constexpr const char* kContentTypeOctetStream = "application/octet-stream";

bool IsSupportedContentType(const std::string& content_type) {
  return content_type == kContentTypeJson || content_type == kContentTypeCbor ||
         content_type == kContentTypeOctetStream;
}

std::vector<uint8_t> ToBytes(const std::string& text) {
  return std::vector<uint8_t>(text.begin(), text.end());
}

std::string ToText(const std::vector<uint8_t>& bytes) {
  return std::string(bytes.begin(), bytes.end());
}

}  // namespace

// ===== Meta Implementation =====

std::string Meta::ToJson() const {
  rapidjson::Document doc;
  doc.SetObject();
  auto& alloc = doc.GetAllocator();

  doc.AddMember("proto", rapidjson::Value(proto.c_str(), alloc), alloc);
  doc.AddMember("proto_version", proto_version, alloc);
  doc.AddMember("type", rapidjson::Value(type.c_str(), alloc), alloc);
  if (id_is_number) {
    doc.AddMember("id", id_number, alloc);
  } else {
    doc.AddMember("id", rapidjson::Value(id.c_str(), alloc), alloc);
  }
  doc.AddMember("content_type", rapidjson::Value(content_type.c_str(), alloc),
                alloc);

  if (!method.empty()) {
    doc.AddMember("method", rapidjson::Value(method.c_str(), alloc), alloc);
  }

  if (!status.empty()) {
    doc.AddMember("status", rapidjson::Value(status.c_str(), alloc), alloc);
  }

  if (has_encoder_status) {
    doc.AddMember("encoder_status", encoder_status, alloc);
  }
  if (!error_msg.empty()) {
    doc.AddMember("error_msg", rapidjson::Value(error_msg.c_str(), alloc),
                  alloc);
  }
  if (!lepus_code.empty()) {
    doc.AddMember("lepus_code", rapidjson::Value(lepus_code.c_str(), alloc),
                  alloc);
  }
  if (!lepus_debug.empty()) {
    doc.AddMember("lepus_debug", rapidjson::Value(lepus_debug.c_str(), alloc),
                  alloc);
  }
  if (!section_size.empty()) {
    doc.AddMember("section_size", rapidjson::Value(section_size.c_str(), alloc),
                  alloc);
  }

  if (!error.code.empty()) {
    rapidjson::Value err_obj(rapidjson::kObjectType);
    err_obj.AddMember("code", rapidjson::Value(error.code.c_str(), alloc),
                      alloc);
    err_obj.AddMember("message", rapidjson::Value(error.message.c_str(), alloc),
                      alloc);
    doc.AddMember("error", err_obj, alloc);
  }

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
}

Meta Meta::FromJson(const std::string& json) {
  Meta meta;
  rapidjson::Document doc;
  doc.Parse(json.c_str());

  if (doc.HasParseError() || !doc.IsObject()) {
    return meta;
  }

  if (doc.HasMember("proto") && doc["proto"].IsString()) {
    meta.proto = doc["proto"].GetString();
  }
  if (doc.HasMember("proto_version") && doc["proto_version"].IsInt64()) {
    meta.proto_version = doc["proto_version"].GetInt64();
  }
  if (doc.HasMember("type") && doc["type"].IsString()) {
    meta.type = doc["type"].GetString();
  }
  if (doc.HasMember("id")) {
    if (doc["id"].IsString()) {
      meta.id = doc["id"].GetString();
    } else if (doc["id"].IsInt64()) {
      meta.id_is_number = true;
      meta.id_number = doc["id"].GetInt64();
      meta.id = std::to_string(meta.id_number);
    } else if (doc["id"].IsUint64()) {
      meta.id_is_number = true;
      meta.id_number = static_cast<int64_t>(doc["id"].GetUint64());
      meta.id = std::to_string(meta.id_number);
    }
  }
  if (doc.HasMember("method") && doc["method"].IsString()) {
    meta.method = doc["method"].GetString();
  }
  if (doc.HasMember("content_type") && doc["content_type"].IsString()) {
    meta.content_type = doc["content_type"].GetString();
  }
  if (doc.HasMember("status") && doc["status"].IsString()) {
    meta.status = doc["status"].GetString();
  }
  if (doc.HasMember("encoder_status") && doc["encoder_status"].IsInt64()) {
    meta.has_encoder_status = true;
    meta.encoder_status = doc["encoder_status"].GetInt64();
  }
  if (doc.HasMember("error_msg") && doc["error_msg"].IsString()) {
    meta.error_msg = doc["error_msg"].GetString();
  }
  if (doc.HasMember("lepus_code") && doc["lepus_code"].IsString()) {
    meta.lepus_code = doc["lepus_code"].GetString();
  }
  if (doc.HasMember("lepus_debug") && doc["lepus_debug"].IsString()) {
    meta.lepus_debug = doc["lepus_debug"].GetString();
  }
  if (doc.HasMember("section_size") && doc["section_size"].IsString()) {
    meta.section_size = doc["section_size"].GetString();
  }
  if (doc.HasMember("error") && doc["error"].IsObject()) {
    const auto& err = doc["error"];
    if (err.HasMember("code") && err["code"].IsString()) {
      meta.error.code = err["code"].GetString();
    }
    if (err.HasMember("message") && err["message"].IsString()) {
      meta.error.message = err["message"].GetString();
    }
  }

  return meta;
}

// ===== IpcProtocol Implementation =====

IpcProtocol::IpcProtocol() = default;
IpcProtocol::~IpcProtocol() = default;

void IpcProtocol::Init(std::istream* in, std::ostream* out) {
  in_ = in;
  out_ = out;
}

void IpcProtocol::RegisterHandler(const std::string& method,
                                  RequestHandler handler) {
  handlers_[method] = handler;
}

bool IpcProtocol::ReadFrame(FrameHeader& header, std::string& meta_json,
                            std::vector<uint8_t>& body, ReadFrameError& error) {
  error = ReadFrameError::kNone;
  if (!in_) return false;

  // Frame = [MetaLen][MetaBytes][BodyLen][BodyBytes]
  // Read MetaLen (4 bytes, big-endian)
  uint32_t meta_len = 0;
  in_->read(reinterpret_cast<char*>(&meta_len), 4);
  if (in_->gcount() != 4) {
    error = ReadFrameError::kInvalidFrame;
    return false;
  }
  // Convert from big-endian
  meta_len = ((meta_len & 0xFF000000) >> 24) | ((meta_len & 0x00FF0000) >> 8) |
             ((meta_len & 0x0000FF00) << 8) | ((meta_len & 0x000000FF) << 24);

  if (meta_len == 0 || meta_len > kMaxMetaSize) {
    error = meta_len > kMaxMetaSize ? ReadFrameError::kFrameTooLarge
                                    : ReadFrameError::kInvalidFrame;
    return false;
  }

  // Read Meta JSON
  meta_json.resize(meta_len);
  in_->read(meta_json.data(), meta_len);
  if (static_cast<uint32_t>(in_->gcount()) != meta_len) {
    error = ReadFrameError::kInvalidFrame;
    return false;
  }

  // Read BodyLen (4 bytes, big-endian)
  uint32_t body_len = 0;
  in_->read(reinterpret_cast<char*>(&body_len), 4);
  if (in_->gcount() != 4) {
    error = ReadFrameError::kInvalidFrame;
    return false;
  }
  // Convert from big-endian
  body_len = ((body_len & 0xFF000000) >> 24) | ((body_len & 0x00FF0000) >> 8) |
             ((body_len & 0x0000FF00) << 8) | ((body_len & 0x000000FF) << 24);

  // Validate sizes
  if (body_len > kMaxFrameSize) {
    error = ReadFrameError::kFrameTooLarge;
    return false;
  }

  header.meta_len = meta_len;
  header.body_len = body_len;

  // Read Body
  body.resize(body_len);
  if (body_len > 0) {
    in_->read(reinterpret_cast<char*>(body.data()), body_len);
    if (static_cast<uint32_t>(in_->gcount()) != body_len) {
      error = ReadFrameError::kInvalidFrame;
      return false;
    }
  }

  return true;
}

bool IpcProtocol::WriteFrame(const Meta& meta,
                             const std::vector<uint8_t>& body) {
  if (!out_) return false;

  std::string meta_json = meta.ToJson();
  const uint32_t meta_len = static_cast<uint32_t>(meta_json.size());
  uint32_t body_len = static_cast<uint32_t>(body.size());

  // Write MetaLen (4 bytes, big-endian)
  uint32_t meta_len_be =
      ((meta_len & 0x000000FF) << 24) | ((meta_len & 0x0000FF00) << 8) |
      ((meta_len & 0x00FF0000) >> 8) | ((meta_len & 0xFF000000) >> 24);
  out_->write(reinterpret_cast<const char*>(&meta_len_be), 4);

  // Write Meta JSON
  if (meta_len > 0) {
    out_->write(meta_json.data(), meta_len);
  }

  // Write BodyLen (4 bytes, big-endian)
  uint32_t body_len_be =
      ((body_len & 0x000000FF) << 24) | ((body_len & 0x0000FF00) << 8) |
      ((body_len & 0x00FF0000) >> 8) | ((body_len & 0xFF000000) >> 24);
  out_->write(reinterpret_cast<const char*>(&body_len_be), 4);

  // Write Body
  if (body_len > 0) {
    out_->write(reinterpret_cast<const char*>(body.data()), body_len);
  }

  out_->flush();
  return out_->good();
}

void IpcProtocol::ProcessRequest(const Request& request) {
  Response response;
  response.meta.id = request.meta.id;
  response.meta.id_is_number = request.meta.id_is_number;
  response.meta.id_number = request.meta.id_number;
  response.meta.proto = kProtoName;
  response.meta.proto_version = kProtocolVersion;
  response.meta.type = "response";
  response.meta.content_type = request.meta.content_type;

  auto it = handlers_.find(request.meta.method);
  if (it == handlers_.end()) {
    response.meta.status = "error";
    response.meta.error = ErrorInfo(error::kMethodNotFound,
                                    "Method not found: " + request.meta.method);
  } else {
    try {
      response = it->second(request);
    } catch (const std::exception& e) {
      response.meta.status = "error";
      response.meta.error = ErrorInfo(error::kInternalError, e.what());
    }
  }

  // Send response
  std::vector<uint8_t> body;
  if (response.meta.status != "error" && !response.body.IsNull()) {
    if (response.meta.content_type == kContentTypeCbor) {
      cbor::Encoder encoder;
      body = encoder.Encode(response.body);
    } else if (response.meta.content_type == kContentTypeJson) {
      if (!response.body.IsText()) {
        Response error_response = CreateErrorResponse(
            request.meta.id,
            ErrorInfo(error::kInternalError,
                      "response body must be text for application/json"));
        error_response.meta.proto = kProtoName;
        error_response.meta.proto_version = kProtocolVersion;
        error_response.meta.id_is_number = request.meta.id_is_number;
        error_response.meta.id_number = request.meta.id_number;
        error_response.meta.content_type = request.meta.content_type;
        WriteFrame(error_response.meta, {});
        return;
      }
      body = ToBytes(response.body.AsText());
    } else if (response.meta.content_type == kContentTypeOctetStream) {
      if (response.body.IsBytes()) {
        body = response.body.AsBytes();
      } else if (response.body.IsText()) {
        body = ToBytes(response.body.AsText());
      } else {
        Response error_response = CreateErrorResponse(
            request.meta.id, ErrorInfo(error::kInternalError,
                                       "response body must be bytes/text for "
                                       "application/octet-stream"));
        error_response.meta.proto = kProtoName;
        error_response.meta.proto_version = kProtocolVersion;
        error_response.meta.id_is_number = request.meta.id_is_number;
        error_response.meta.id_number = request.meta.id_number;
        error_response.meta.content_type = request.meta.content_type;
        WriteFrame(error_response.meta, {});
        return;
      }
    } else {
      Response error_response = CreateErrorResponse(
          request.meta.id, ErrorInfo(error::kUnsupportedContentType,
                                     "Unsupported content_type in response"));
      error_response.meta.proto = kProtoName;
      error_response.meta.proto_version = kProtocolVersion;
      error_response.meta.id_is_number = request.meta.id_is_number;
      error_response.meta.id_number = request.meta.id_number;
      error_response.meta.content_type = request.meta.content_type;
      WriteFrame(error_response.meta, {});
      return;
    }
  }
  WriteFrame(response.meta, body);
}

void IpcProtocol::Run() {
  running_ = true;

  while (running_ && in_ && out_ && in_->good()) {
    FrameHeader header;
    std::string meta_json;
    std::vector<uint8_t> body_data;
    ReadFrameError read_error = ReadFrameError::kNone;

    if (!ReadFrame(header, meta_json, body_data, read_error)) {
      if (in_->eof()) break;
      // Cannot reliably recover framing in stdio stream.
      Response error_response;
      error_response.meta.proto = kProtoName;
      error_response.meta.proto_version = kProtocolVersion;
      error_response.meta.type = "response";
      error_response.meta.content_type = kContentTypeJson;
      error_response.meta.status = "error";
      error_response.meta.error = ErrorInfo(
          read_error == ReadFrameError::kFrameTooLarge ? error::kFrameTooLarge
                                                       : error::kInvalidFrame,
          "Failed to read frame");
      WriteFrame(error_response.meta, {});
      break;
    }

    // Parse meta
    Meta meta = Meta::FromJson(meta_json);

    if (meta.proto != kProtoName) {
      Response error_response;
      error_response.meta.id = meta.id;
      error_response.meta.id_is_number = meta.id_is_number;
      error_response.meta.id_number = meta.id_number;
      error_response.meta.proto = kProtoName;
      error_response.meta.proto_version = kProtocolVersion;
      error_response.meta.type = "response";
      error_response.meta.content_type = kContentTypeJson;
      error_response.meta.status = "error";
      error_response.meta.error =
          ErrorInfo(error::kInvalidMetaJson, "invalid or missing proto");
      WriteFrame(error_response.meta, {});
      continue;
    }

    if (meta.proto_version != kProtocolVersion) {
      Response error_response;
      error_response.meta.id = meta.id;
      error_response.meta.id_is_number = meta.id_is_number;
      error_response.meta.id_number = meta.id_number;
      error_response.meta.proto = kProtoName;
      error_response.meta.proto_version = kProtocolVersion;
      error_response.meta.type = "response";
      error_response.meta.content_type = kContentTypeJson;
      error_response.meta.status = "error";
      error_response.meta.error = ErrorInfo(error::kUnsupportedProtoVersion,
                                            "Unsupported proto_version");
      WriteFrame(error_response.meta, {});
      continue;
    }

    if (meta.type == "request") {
      if (meta.id.empty() || meta.method.empty()) {
        Response error_response;
        error_response.meta.id = meta.id;
        error_response.meta.id_is_number = meta.id_is_number;
        error_response.meta.id_number = meta.id_number;
        error_response.meta.proto = kProtoName;
        error_response.meta.proto_version = kProtocolVersion;
        error_response.meta.type = "response";
        error_response.meta.content_type = kContentTypeJson;
        error_response.meta.status = "error";
        error_response.meta.error =
            ErrorInfo(error::kInvalidMetaJson, "missing id or method");
        WriteFrame(error_response.meta, {});
        continue;
      }

      if (!IsSupportedContentType(meta.content_type)) {
        Response error_response;
        error_response.meta.id = meta.id;
        error_response.meta.id_is_number = meta.id_is_number;
        error_response.meta.id_number = meta.id_number;
        error_response.meta.proto = kProtoName;
        error_response.meta.proto_version = kProtocolVersion;
        error_response.meta.type = "response";
        error_response.meta.content_type = meta.content_type;
        error_response.meta.status = "error";
        error_response.meta.error =
            ErrorInfo(error::kUnsupportedContentType,
                      "Unsupported content_type: " + meta.content_type);
        WriteFrame(error_response.meta, {});
        continue;
      }

      // Parse body as CBOR
      cbor::Value body;
      if (!body_data.empty()) {
        if (meta.content_type == kContentTypeCbor) {
          cbor::Decoder decoder;
          body = decoder.Decode(body_data);
          if (decoder.HasError()) {
            Response error_response;
            error_response.meta.id = meta.id;
            error_response.meta.id_is_number = meta.id_is_number;
            error_response.meta.id_number = meta.id_number;
            error_response.meta.proto = kProtoName;
            error_response.meta.proto_version = kProtocolVersion;
            error_response.meta.type = "response";
            error_response.meta.content_type = kContentTypeJson;
            error_response.meta.status = "error";
            error_response.meta.error =
                ErrorInfo(error::kInvalidParams,
                          "Failed to decode CBOR body: " + decoder.Error());
            WriteFrame(error_response.meta, {});
            continue;
          }
        } else if (meta.content_type == kContentTypeJson) {
          rapidjson::Document doc;
          const auto json_text = ToText(body_data);
          doc.Parse(json_text.c_str());
          if (doc.HasParseError()) {
            Response error_response;
            error_response.meta.id = meta.id;
            error_response.meta.id_is_number = meta.id_is_number;
            error_response.meta.id_number = meta.id_number;
            error_response.meta.proto = kProtoName;
            error_response.meta.proto_version = kProtocolVersion;
            error_response.meta.type = "response";
            error_response.meta.content_type = kContentTypeJson;
            error_response.meta.status = "error";
            error_response.meta.error =
                ErrorInfo(error::kInvalidParams, "Failed to parse JSON body");
            WriteFrame(error_response.meta, {});
            continue;
          }
          body = cbor::Value(json_text);
        } else if (meta.content_type == kContentTypeOctetStream) {
          body = cbor::Value(body_data);
        } else {
          Response error_response;
          error_response.meta.id = meta.id;
          error_response.meta.id_is_number = meta.id_is_number;
          error_response.meta.id_number = meta.id_number;
          error_response.meta.proto = kProtoName;
          error_response.meta.proto_version = kProtocolVersion;
          error_response.meta.type = "response";
          error_response.meta.content_type = kContentTypeJson;
          error_response.meta.status = "error";
          error_response.meta.error =
              ErrorInfo(error::kUnsupportedContentType,
                        "Unsupported content_type: " + meta.content_type);
          WriteFrame(error_response.meta, {});
          continue;
        }
      }

      Request request{meta, body};
      ProcessRequest(request);
    }
    // For "response" type, we don't handle it here since this is server-side
  }

  running_ = false;
}

void IpcProtocol::Stop() { running_ = false; }

Response IpcProtocol::CreateSuccessResponse(const std::string& id,
                                            const std::string& content_type,
                                            cbor::Value body) {
  Response response;
  response.meta.id = id;
  response.meta.proto = kProtoName;
  response.meta.proto_version = kProtocolVersion;
  response.meta.type = "response";
  response.meta.content_type = content_type;
  response.meta.status = "ok";
  response.body = std::move(body);
  return response;
}

Response IpcProtocol::CreateErrorResponse(const std::string& id,
                                          const ErrorInfo& error) {
  Response response;
  response.meta.id = id;
  response.meta.proto = kProtoName;
  response.meta.proto_version = kProtocolVersion;
  response.meta.type = "response";
  response.meta.content_type = kContentTypeOctetStream;
  response.meta.status = "error";
  response.meta.error = error;
  response.meta.has_encoder_status = true;
  response.meta.encoder_status = 1;
  response.meta.error_msg = error.message;
  response.meta.lepus_code = "";
  response.meta.lepus_debug = "";
  response.meta.section_size = "";
  return response;
}

}  // namespace ipc
}  // namespace lynx
