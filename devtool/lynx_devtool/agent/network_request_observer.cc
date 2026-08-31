// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/network_request_observer.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <limits>
#include <utility>

#include "base/include/string/string_utils.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "third_party/modp_b64/modp_b64.h"

namespace lynx {
namespace devtool {

namespace {

constexpr size_t DEFAULT_MAX_TOTAL_BUFFER_SIZE = 50 * 1024 * 1024;
constexpr size_t DEFAULT_MAX_RESOURCE_BUFFER_SIZE = 5 * 1024 * 1024;
constexpr size_t DEFAULT_MAX_POST_DATA_SIZE = 64 * 1024;

std::string ToLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char ch) { return std::tolower(ch); });
  return value;
}

bool ReadNetworkSizeParameter(const Json::Value& params, const char* name,
                              size_t default_value, size_t& result) {
  if (params.isNull() || !params.isMember(name)) {
    result = default_value;
    return true;
  }
  const Json::Value& value = params[name];
  if ((!value.isInt64() && !value.isUInt64()) ||
      (value.isInt64() && value.asInt64() < 0) ||
      value.asUInt64() > std::numeric_limits<size_t>::max()) {
    return false;
  }
  result = static_cast<size_t>(value.asUInt64());
  return true;
}

void EncodeBody(const std::vector<uint8_t>& data, std::string& body,
                bool& base64_encoded) {
  base64_encoded = !base::IsValidUtf8(data.data(), data.size());
  if (!base64_encoded) {
    body.assign(data.begin(), data.end());
    return;
  }
  body.resize(lynx_modp_b64_encode_data_len(data.size()));
  lynx_modp_b64_encode_data(
      body.data(), reinterpret_cast<const char*>(data.data()), data.size());
}

std::string HeaderValue(const std::map<std::string, std::string>& headers,
                        const std::string& name) {
  const std::string lower_name = ToLower(name);
  for (const auto& header : headers) {
    if (ToLower(header.first) == lower_name) {
      return header.second;
    }
  }
  return std::string();
}

Json::Value HeadersToJson(const std::map<std::string, std::string>& headers) {
  Json::Value result(Json::ValueType::objectValue);
  for (const auto& header : headers) {
    result[header.first] = header.second;
  }
  return result;
}

}  // namespace

NetworkRequestObserver::NetworkRequestObserver(
    const std::weak_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_wp_(devtool_mediator) {}

NetworkRequestObserver::~NetworkRequestObserver() = default;

bool NetworkRequestObserver::Enable(const Json::Value& params) {
  // A repeated enable while already enabled is a no-op: buffered bodies are
  // kept and the current limits stay in effect regardless of the new params.
  if (enabled_.load(std::memory_order_relaxed)) {
    return true;
  }
  if (!params.isNull() && !params.isObject()) {
    return false;
  }
  size_t max_total_buffer_size = 0;
  size_t max_resource_buffer_size = 0;
  size_t max_post_data_size = 0;
  if (!ReadNetworkSizeParameter(params, "maxTotalBufferSize",
                                DEFAULT_MAX_TOTAL_BUFFER_SIZE,
                                max_total_buffer_size) ||
      !ReadNetworkSizeParameter(params, "maxResourceBufferSize",
                                DEFAULT_MAX_RESOURCE_BUFFER_SIZE,
                                max_resource_buffer_size) ||
      !ReadNetworkSizeParameter(params, "maxPostDataSize",
                                DEFAULT_MAX_POST_DATA_SIZE,
                                max_post_data_size)) {
    return false;
  }
  records_.clear();
  body_fifo_.clear();
  retained_bytes_ = 0;
  max_total_buffer_size_ = max_total_buffer_size;
  max_resource_buffer_size_ = max_resource_buffer_size;
  max_post_data_size_ = max_post_data_size;
  enabled_.store(true, std::memory_order_relaxed);
  return true;
}

void NetworkRequestObserver::Disable() {
  enabled_.store(false, std::memory_order_relaxed);
  records_.clear();
  body_fifo_.clear();
  retained_bytes_ = 0;
}

bool NetworkRequestObserver::IsEnabled() const {
  return enabled_.load(std::memory_order_relaxed);
}

std::string NetworkRequestObserver::RequestWillBeSent(
    NetworkRequestInfo request) {
  if (!IsEnabled()) {
    return std::string();
  }
  const std::string request_id =
      "lynx-fetch-" + std::to_string(next_request_id_.fetch_add(1));
  PostCapture(
      [request_id, request = std::move(request)](
          NetworkRequestObserver& observer, const CaptureTime& time) mutable {
        observer.RequestWillBeSentImpl(std::move(request_id),
                                       std::move(request), time);
      });
  return request_id;
}

void NetworkRequestObserver::ResponseReceived(const std::string& request_id,
                                              NetworkResponseInfo response) {
  if (!IsEnabled()) {
    return;
  }
  PostCapture(
      [request_id, response = std::move(response)](
          NetworkRequestObserver& observer, const CaptureTime& time) mutable {
        observer.ResponseReceivedImpl(request_id, std::move(response), time);
      });
}

void NetworkRequestObserver::DataReceived(const std::string& request_id,
                                          std::vector<uint8_t> data) {
  if (!IsEnabled()) {
    return;
  }
  PostCapture(
      [request_id, data = std::move(data)](NetworkRequestObserver& observer,
                                           const CaptureTime& time) mutable {
        observer.DataReceivedImpl(request_id, std::move(data), time);
      });
}

void NetworkRequestObserver::LoadingFinished(const std::string& request_id) {
  if (!IsEnabled()) {
    return;
  }
  PostCapture(
      [request_id](NetworkRequestObserver& observer, const CaptureTime& time) {
        observer.LoadingFinishedImpl(request_id, time);
      });
}

void NetworkRequestObserver::LoadingFailed(const std::string& request_id,
                                           const std::string& error_text,
                                           bool canceled) {
  if (!IsEnabled()) {
    return;
  }
  PostCapture([request_id, error_text, canceled](
                  NetworkRequestObserver& observer, const CaptureTime& time) {
    observer.LoadingFailedImpl(request_id, error_text, canceled, time);
  });
}

void NetworkRequestObserver::EventSourceMessageReceived(
    const std::string& request_id, const std::string& event_name,
    const std::string& event_id, const std::string& data) {
  if (!IsEnabled()) {
    return;
  }
  PostCapture([request_id, event_name, event_id, data](
                  NetworkRequestObserver& observer, const CaptureTime& time) {
    observer.EventSourceMessageReceivedImpl(request_id, event_name, event_id,
                                            data, time);
  });
}

NetworkRequestObserver::BodyResult NetworkRequestObserver::GetResponseBody(
    const std::string& request_id, std::string& body, bool& base64_encoded) {
  auto it = records_.find(request_id);
  if (it == records_.end()) {
    return BodyResult::REQUEST_NOT_FOUND;
  }
  RequestRecord& record = it->second;
  if (record.lifecycle == LifecycleState::REQUESTED ||
      record.lifecycle == LifecycleState::RESPONDED) {
    return BodyResult::NOT_FINISHED;
  }
  if (record.lifecycle == LifecycleState::FAILED) {
    return BodyResult::LOADING_FAILED;
  }
  if (record.response_body_state == RetentionState::EVICTED) {
    return BodyResult::EVICTED;
  }
  if (record.response_body_state == RetentionState::TOO_LARGE) {
    return BodyResult::TOO_LARGE;
  }
  if (record.response_body_state == RetentionState::NONE) {
    return BodyResult::NO_BODY;
  }
  EncodeBody(record.response_body, body, base64_encoded);
  return BodyResult::OK;
}

NetworkRequestObserver::BodyResult NetworkRequestObserver::GetRequestPostData(
    const std::string& request_id, std::string& post_data,
    bool& base64_encoded) {
  auto it = records_.find(request_id);
  if (it == records_.end()) {
    return BodyResult::REQUEST_NOT_FOUND;
  }
  RequestRecord& record = it->second;
  if (record.request_body_state == RetentionState::EVICTED) {
    return BodyResult::EVICTED;
  }
  if (record.request_body_state == RetentionState::TOO_LARGE) {
    return BodyResult::TOO_LARGE;
  }
  if (record.request_body_state == RetentionState::NONE) {
    return BodyResult::NO_BODY;
  }
  EncodeBody(record.request_body, post_data, base64_encoded);
  return BodyResult::OK;
}

const char* NetworkRequestObserver::NetworkBodyResultMessage(
    BodyResult result) {
  switch (result) {
    case BodyResult::REQUEST_NOT_FOUND:
      return "No resource with given identifier found";
    case BodyResult::NO_BODY:
      return "No body was sent or received for this request";
    case BodyResult::EVICTED:
      return "Body was evicted from the Network buffer";
    case BodyResult::TOO_LARGE:
      return "Body exceeded maxResourceBufferSize";
    case BodyResult::NOT_FINISHED:
      return "Response body is not available until loading is finished";
    case BodyResult::LOADING_FAILED:
      return "Response body is unavailable because the request failed";
    case BodyResult::OK:
      break;
  }
  return "";
}

void NetworkRequestObserver::PostCapture(CaptureTask task) {
  auto mediator = devtool_mediator_wp_.lock();
  if (!mediator) {
    return;
  }
  const CaptureTime time{
      std::chrono::duration<double>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count(),
      std::chrono::duration<double>(
          std::chrono::system_clock::now().time_since_epoch())
          .count()};
  mediator->RunOnDevToolThread(
      [weak_self = weak_from_this(), time, task = std::move(task)]() mutable {
        auto observer = weak_self.lock();
        if (!observer || !observer->IsEnabled()) {
          return;
        }
        task(*observer, time);
      },
      false);
}

void NetworkRequestObserver::SendCDPEvent(const Json::Value& event) {
  auto mediator = devtool_mediator_wp_.lock();
  if (!mediator) {
    return;
  }
  mediator->SendCDPEvent(event);
}

void NetworkRequestObserver::RequestWillBeSentImpl(std::string request_id,
                                                   NetworkRequestInfo request,
                                                   const CaptureTime& time) {
  RequestRecord record;
  record.request_url = request.url;
  auto result = records_.emplace(request_id, std::move(record));
  RequestRecord& stored = result.first->second;
  Json::Value event;
  event["method"] = "Network.requestWillBeSent";
  Json::Value& params = event["params"];
  params["requestId"] = request_id;
  params["loaderId"] = "lynx-loader";
  params["documentURL"] = request.url;
  params["timestamp"] = time.monotonic;
  params["wallTime"] = time.wall;
  params["initiator"]["type"] = "script";
  params["redirectHasExtraInfo"] = false;
  params["type"] = "Fetch";
  Json::Value& cdp_request = params["request"];
  cdp_request["url"] = request.url;
  cdp_request["method"] = request.method.empty() ? "GET" : request.method;
  cdp_request["headers"] = HeadersToJson(request.headers);
  cdp_request["initialPriority"] = "Medium";
  cdp_request["referrerPolicy"] = "no-referrer-when-downgrade";
  if (!request.body.empty()) {
    cdp_request["hasPostData"] = true;
    if (request.body.size() <= max_post_data_size_ &&
        base::IsValidUtf8(request.body.data(), request.body.size())) {
      cdp_request["postData"] =
          std::string(request.body.begin(), request.body.end());
    }
  }
  RetainRequestBody(stored, request_id, std::move(request.body));
  SendCDPEvent(event);
}

void NetworkRequestObserver::ResponseReceivedImpl(const std::string& request_id,
                                                  NetworkResponseInfo response,
                                                  const CaptureTime& time) {
  RequestRecord* record = FindActiveRecord(request_id);
  if (record == nullptr || record->lifecycle != LifecycleState::REQUESTED) {
    return;
  }

  Json::Value event;
  if (response.status <= 0) {
    record->lifecycle = LifecycleState::FAILED;
    event["method"] = "Network.loadingFailed";
    event["params"]["requestId"] = request_id;
    event["params"]["timestamp"] = time.monotonic;
    event["params"]["type"] = "Fetch";
    event["params"]["errorText"] = response.status_text.empty()
                                       ? "Network request failed"
                                       : response.status_text;
    SendCDPEvent(event);
    return;
  }

  record->lifecycle = LifecycleState::RESPONDED;
  record->response_body_state = RetentionState::RETAINED;

  event["method"] = "Network.responseReceived";
  Json::Value& params = event["params"];
  params["requestId"] = request_id;
  params["loaderId"] = "lynx-loader";
  params["timestamp"] = time.monotonic;
  params["type"] = "Fetch";
  params["hasExtraInfo"] = false;
  Json::Value& cdp_response = params["response"];
  cdp_response["url"] =
      response.url.empty() ? record->request_url : response.url;
  cdp_response["status"] = response.status;
  cdp_response["statusText"] = response.status_text;
  cdp_response["headers"] = HeadersToJson(response.headers);
  const std::string content_type =
      HeaderValue(response.headers, "content-type");
  const size_t separator = content_type.find(';');
  cdp_response["mimeType"] = content_type.substr(0, separator);
  cdp_response["charset"] = "";
  cdp_response["connectionReused"] = false;
  cdp_response["connectionId"] = 0;
  cdp_response["encodedDataLength"] = Json::Value::UInt64(0);
  cdp_response["securityState"] = "unknown";
  SendCDPEvent(event);
}

void NetworkRequestObserver::DataReceivedImpl(const std::string& request_id,
                                              std::vector<uint8_t> data,
                                              const CaptureTime& time) {
  RequestRecord* record = FindActiveRecord(request_id);
  if (record == nullptr || record->lifecycle != LifecycleState::RESPONDED) {
    return;
  }
  const size_t data_size = data.size();
  record->encoded_data_length += data_size;
  AppendResponseBody(*record, request_id, std::move(data));

  Json::Value event;
  event["method"] = "Network.dataReceived";
  event["params"]["requestId"] = request_id;
  event["params"]["timestamp"] = time.monotonic;
  event["params"]["dataLength"] = Json::Value::UInt64(data_size);
  event["params"]["encodedDataLength"] = Json::Value::UInt64(data_size);
  SendCDPEvent(event);
}

void NetworkRequestObserver::LoadingFinishedImpl(const std::string& request_id,
                                                 const CaptureTime& time) {
  RequestRecord* record = FindActiveRecord(request_id);
  if (record == nullptr || record->lifecycle != LifecycleState::RESPONDED) {
    return;
  }
  record->lifecycle = LifecycleState::FINISHED;
  Json::Value event;
  event["method"] = "Network.loadingFinished";
  event["params"]["requestId"] = request_id;
  event["params"]["timestamp"] = time.monotonic;
  event["params"]["encodedDataLength"] =
      Json::Value::UInt64(record->encoded_data_length);
  SendCDPEvent(event);
}

void NetworkRequestObserver::LoadingFailedImpl(const std::string& request_id,
                                               const std::string& error_text,
                                               bool canceled,
                                               const CaptureTime& time) {
  RequestRecord* record = FindActiveRecord(request_id);
  if (record == nullptr || record->lifecycle == LifecycleState::FINISHED ||
      record->lifecycle == LifecycleState::FAILED) {
    return;
  }
  EvictBody(*record, BodyKind::RESPONSE);
  record->lifecycle = LifecycleState::FAILED;
  Json::Value event;
  event["method"] = "Network.loadingFailed";
  event["params"]["requestId"] = request_id;
  event["params"]["timestamp"] = time.monotonic;
  event["params"]["type"] = "Fetch";
  event["params"]["errorText"] = error_text;
  event["params"]["canceled"] = canceled;
  SendCDPEvent(event);
}

void NetworkRequestObserver::EventSourceMessageReceivedImpl(
    const std::string& request_id, const std::string& event_name,
    const std::string& event_id, const std::string& data,
    const CaptureTime& time) {
  RequestRecord* record = FindActiveRecord(request_id);
  if (record == nullptr || record->lifecycle != LifecycleState::RESPONDED) {
    return;
  }
  Json::Value event;
  event["method"] = "Network.eventSourceMessageReceived";
  event["params"]["requestId"] = request_id;
  event["params"]["timestamp"] = time.monotonic;
  event["params"]["eventName"] = event_name.empty() ? "message" : event_name;
  event["params"]["eventId"] = event_id;
  event["params"]["data"] = data;
  SendCDPEvent(event);
}

void NetworkRequestObserver::RetainRequestBody(RequestRecord& record,
                                               const std::string& request_id,
                                               std::vector<uint8_t> body) {
  if (body.empty()) {
    record.request_body_state = RetentionState::NONE;
    return;
  }
  if (body.size() > max_resource_buffer_size_) {
    record.request_body_state = RetentionState::TOO_LARGE;
    return;
  }
  record.request_body = std::move(body);
  record.request_body_state = RetentionState::RETAINED;
  retained_bytes_ += record.request_body.size();
  body_fifo_.push_back({request_id, BodyKind::REQUEST});
  EvictUntilWithinLimit();
}

void NetworkRequestObserver::AppendResponseBody(RequestRecord& record,
                                                const std::string& request_id,
                                                std::vector<uint8_t> data) {
  if (record.response_body_state == RetentionState::TOO_LARGE ||
      record.response_body_state == RetentionState::EVICTED) {
    return;
  }
  const size_t size = data.size();
  if (record.response_body.size() > max_resource_buffer_size_ ||
      size > max_resource_buffer_size_ - record.response_body.size()) {
    retained_bytes_ -= record.response_body.size();
    std::vector<uint8_t>().swap(record.response_body);
    record.response_body_state = RetentionState::TOO_LARGE;
    return;
  }
  if (size != 0) {
    if (record.response_body.empty()) {
      body_fifo_.push_back({request_id, BodyKind::RESPONSE});
      record.response_body = std::move(data);
    } else {
      record.response_body.insert(record.response_body.end(), data.begin(),
                                  data.end());
    }
  }
  record.response_body_state = RetentionState::RETAINED;
  retained_bytes_ += size;
  EvictUntilWithinLimit();
}

void NetworkRequestObserver::EvictUntilWithinLimit() {
  while (retained_bytes_ > max_total_buffer_size_ && !body_fifo_.empty()) {
    BodyCacheEntry entry = std::move(body_fifo_.front());
    body_fifo_.pop_front();
    auto record_it = records_.find(entry.request_id);
    if (record_it != records_.end()) {
      EvictBody(record_it->second, entry.kind);
    }
  }
}

void NetworkRequestObserver::EvictBody(RequestRecord& record, BodyKind kind) {
  if (kind == BodyKind::REQUEST) {
    if (record.request_body_state == RetentionState::RETAINED) {
      retained_bytes_ -= record.request_body.size();
      std::vector<uint8_t>().swap(record.request_body);
      record.request_body_state = RetentionState::EVICTED;
    }
    return;
  }
  if (record.response_body_state == RetentionState::RETAINED) {
    retained_bytes_ -= record.response_body.size();
    std::vector<uint8_t>().swap(record.response_body);
    record.response_body_state = RetentionState::EVICTED;
  }
}

NetworkRequestObserver::RequestRecord* NetworkRequestObserver::FindActiveRecord(
    const std::string& request_id) {
  auto it = records_.find(request_id);
  return it == records_.end() ? nullptr : &it->second;
}

}  // namespace devtool
}  // namespace lynx
