// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_NETWORK_NETWORK_REQUEST_OBSERVER_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_NETWORK_NETWORK_REQUEST_OBSERVER_H_

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "third_party/jsoncpp/include/json/json.h"

namespace lynx {
namespace devtool {

class LynxDevToolMediator;

struct NetworkRequestInfo {
  std::string url;
  std::string method;
  std::map<std::string, std::string> headers;
  std::vector<uint8_t> body;
};

struct NetworkResponseInfo {
  std::string url;
  int status{0};
  std::string status_text;
  std::map<std::string, std::string> headers;
};

// Single-threaded Network capture state machine resident on the DevTool
// thread. The public capture methods are marshaling boundaries callable from
// any thread: they fast-return while disabled, allocate the request id on the
// calling thread and post the capture onto the DevTool runner. Captures, CDP
// commands, body queries and CDP event sends all run on that single FIFO.
// Whether a queued capture is processed is determined by the enabled state
// when it executes on the DevTool thread.
class NetworkRequestObserver
    : public std::enable_shared_from_this<NetworkRequestObserver> {
 public:
  enum class BodyResult {
    OK,
    REQUEST_NOT_FOUND,
    NO_BODY,
    EVICTED,
    TOO_LARGE,
    NOT_FINISHED,
    LOADING_FAILED,
  };

  explicit NetworkRequestObserver(
      const std::weak_ptr<LynxDevToolMediator>& devtool_mediator);
  ~NetworkRequestObserver();

  // Capture entry points, callable from any thread. Callers may check
  // IsEnabled() before materializing platform payloads; every entry point
  // rechecks it. Payloads accepted by value can be moved into the DevTool task.
  std::string RequestWillBeSent(NetworkRequestInfo request);
  void ResponseReceived(const std::string& request_id,
                        NetworkResponseInfo response);
  void DataReceived(const std::string& request_id, std::vector<uint8_t> data);
  void LoadingFinished(const std::string& request_id);
  void LoadingFailed(const std::string& request_id,
                     const std::string& error_text, bool canceled = false);
  void EventSourceMessageReceived(const std::string& request_id,
                                  const std::string& event_name,
                                  const std::string& event_id,
                                  const std::string& data);

  // Commands and queries; called on the DevTool thread (CDP command handlers).
  // Applies a Network.enable with the CDP "params" object (buffer limits and
  // their defaults are Network-domain knowledge). Returns false when the
  // params are invalid.
  bool Enable(const Json::Value& params);
  void Disable();
  bool IsEnabled() const;
  BodyResult GetResponseBody(const std::string& request_id, std::string& body,
                             bool& base64_encoded);
  BodyResult GetRequestPostData(const std::string& request_id,
                                std::string& post_data, bool& base64_encoded);

  // Network-domain helper shared with the CDP command implementation: maps a
  // BodyResult to the CDP error message.
  static const char* NetworkBodyResultMessage(BodyResult result);

 private:
  struct CaptureTime {
    double monotonic;
    double wall;
  };
  using CaptureTask =
      std::function<void(NetworkRequestObserver&, const CaptureTime&)>;

  // Marshals the capture onto the DevTool thread and drops it when the observer
  // is disabled at execution time or the mediator (or observer) is already
  // gone.
  void PostCapture(CaptureTask task);

  void SendCDPEvent(const Json::Value& event);

  void RequestWillBeSentImpl(std::string request_id, NetworkRequestInfo request,
                             const CaptureTime& time);
  void ResponseReceivedImpl(const std::string& request_id,
                            NetworkResponseInfo response,
                            const CaptureTime& time);
  void DataReceivedImpl(const std::string& request_id,
                        std::vector<uint8_t> data, const CaptureTime& time);
  void LoadingFinishedImpl(const std::string& request_id,
                           const CaptureTime& time);
  void LoadingFailedImpl(const std::string& request_id,
                         const std::string& error_text, bool canceled,
                         const CaptureTime& time);
  void EventSourceMessageReceivedImpl(const std::string& request_id,
                                      const std::string& event_name,
                                      const std::string& event_id,
                                      const std::string& data,
                                      const CaptureTime& time);

  enum class LifecycleState { REQUESTED, RESPONDED, FINISHED, FAILED };
  enum class RetentionState { NONE, RETAINED, EVICTED, TOO_LARGE };
  enum class BodyKind { REQUEST, RESPONSE };

  struct BodyCacheEntry {
    std::string request_id;
    BodyKind kind;
  };

  struct RequestRecord {
    std::string request_url;
    LifecycleState lifecycle{LifecycleState::REQUESTED};
    RetentionState request_body_state{RetentionState::NONE};
    RetentionState response_body_state{RetentionState::NONE};
    std::vector<uint8_t> request_body;
    std::vector<uint8_t> response_body;
    size_t encoded_data_length{0};
  };

  void RetainRequestBody(RequestRecord& record, const std::string& request_id,
                         std::vector<uint8_t> body);
  void AppendResponseBody(RequestRecord& record, const std::string& request_id,
                          std::vector<uint8_t> data);
  void EvictUntilWithinLimit();
  void EvictBody(RequestRecord& record, BodyKind kind);
  RequestRecord* FindActiveRecord(const std::string& request_id);

  std::weak_ptr<LynxDevToolMediator> devtool_mediator_wp_;
  std::atomic<bool> enabled_{false};
  size_t max_total_buffer_size_;
  size_t max_resource_buffer_size_;
  size_t max_post_data_size_;
  size_t retained_bytes_{0};
  std::atomic<uint64_t> next_request_id_{1};
  std::unordered_map<std::string, RequestRecord> records_;
  std::deque<BodyCacheEntry> body_fifo_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_NETWORK_NETWORK_REQUEST_OBSERVER_H_
