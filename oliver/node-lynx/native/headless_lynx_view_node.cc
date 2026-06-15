// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/lynx_adaptor/ui_delegate_clay.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view_context.h"
#include "platform/embedder/lynx_devtool/devtool_env_embedder.h"
#include "platform/embedder/lynx_template_data_priv.h"
#include "platform/embedder/lynx_view_builder_priv.h"
#include "platform/embedder/lynx_view_priv.h"
#include "platform/embedder/public/capi/lynx_env_capi.h"
#include "platform/embedder/public/capi/lynx_windowless_renderer_capi.h"
#include "platform/embedder/public/lynx_generic_resource_fetcher.h"
#include "platform/embedder/public/lynx_load_meta.h"
#include "platform/embedder/public/lynx_template_data.h"
#include "platform/embedder/public/lynx_update_meta.h"
#include "platform/embedder/public/lynx_view.h"
#include "platform/embedder/public/lynx_view_client.h"
#include "platform/embedder/public/lynx_windowless_renderer.h"
#include "third_party/skia/include/core/SkColorType.h"

#undef NAPI_MODULE_INIT
#undef NAPI_MODULE
#undef NAPI_MODULE_X

#include <node_api.h>
#include <uv.h>

#include "base/include/debug/lynx_error.h"
#include "base/include/fml/message_loop.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "oliver/node-lynx/native/windowed_lynx_view_mac.h"
#include "third_party/debug_router/src/debug_router/native/core/debug_router_core.h"
#include "third_party/debug_router/src/debug_router/native/core/debug_router_message_handler.h"
#include "third_party/jsoncpp/include/json/json.h"

namespace lynx {
namespace node {

namespace {

constexpr int kLocalErrorCode = -1;
constexpr uint64_t kExplicitDestroyGracePeriodMs = 1000;

enum class DestroyReason {
  kExplicit,
  kFinalizer,
};

int ToPixelSize(double logical_size, double device_pixel_ratio) {
  double physical_size = logical_size * device_pixel_ratio;
  if (physical_size <= 0) {
    return 0;
  }
  return static_cast<int>(physical_size + 0.5);
}

std::vector<uint8_t> CopyNativeN32PixelsToRgba(const uint8_t* src,
                                               size_t row_bytes, int width,
                                               int height) {
  constexpr bool kNativePixelsAreBgra =
      kN32_SkColorType == kBGRA_8888_SkColorType;
  const size_t packed_row_bytes = static_cast<size_t>(width) * 4;
  std::vector<uint8_t> rgba(packed_row_bytes * static_cast<size_t>(height));

  for (int y = 0; y < height; ++y) {
    const uint8_t* row = src + static_cast<size_t>(y) * row_bytes;
    uint8_t* out = rgba.data() + static_cast<size_t>(y) * packed_row_bytes;
    if (kNativePixelsAreBgra) {
      for (int x = 0; x < width; ++x) {
        const size_t offset = static_cast<size_t>(x) * 4;
        out[offset] = row[offset + 2];
        out[offset + 1] = row[offset + 1];
        out[offset + 2] = row[offset];
        out[offset + 3] = row[offset + 3];
      }
    } else {
      std::memcpy(out, row, packed_row_bytes);
    }
  }

  return rgba;
}

napi_value GetUndefined(napi_env env) {
  napi_value value = nullptr;
  napi_get_undefined(env, &value);
  return value;
}

void ThrowError(napi_env env, const char* message) {
  napi_throw_error(env, nullptr, message);
}

napi_value CreateError(napi_env env, const std::string& message) {
  napi_value msg = nullptr;
  napi_value err = nullptr;
  napi_create_string_utf8(env, message.c_str(), message.size(), &msg);
  napi_create_error(env, nullptr, msg, &err);
  return err;
}

std::string GetString(napi_env env, napi_value value) {
  if (!value) {
    return "";
  }
  napi_valuetype type = napi_undefined;
  napi_typeof(env, value, &type);
  if (type == napi_undefined || type == napi_null) {
    return "";
  }
  size_t length = 0;
  napi_get_value_string_utf8(env, value, nullptr, 0, &length);
  std::vector<char> buffer(length + 1);
  napi_get_value_string_utf8(env, value, buffer.data(), buffer.size(), &length);
  return std::string(buffer.data(), length);
}

bool GetDouble(napi_env env, napi_value value, double* out) {
  return napi_get_value_double(env, value, out) == napi_ok;
}

bool GetBool(napi_env env, napi_value value, bool default_value = false) {
  if (!value) {
    return default_value;
  }
  napi_valuetype type = napi_undefined;
  napi_typeof(env, value, &type);
  if (type != napi_boolean) {
    return default_value;
  }
  bool result = default_value;
  napi_get_value_bool(env, value, &result);
  return result;
}

bool IsFunction(napi_env env, napi_value value) {
  if (!value) {
    return false;
  }
  napi_valuetype type = napi_undefined;
  napi_typeof(env, value, &type);
  return type == napi_function;
}

bool IsString(napi_env env, napi_value value) {
  if (!value) {
    return false;
  }
  napi_valuetype type = napi_undefined;
  napi_typeof(env, value, &type);
  return type == napi_string;
}

bool IsNullOrUndefined(napi_env env, napi_value value) {
  if (!value) {
    return true;
  }
  napi_valuetype type = napi_undefined;
  napi_typeof(env, value, &type);
  return type == napi_null || type == napi_undefined;
}

bool IsSupportedWindowedKeyboardKey(const std::string& key) {
  return key == "Backspace" || key == "Delete" || key == "Enter" ||
         key == "ArrowLeft" || key == "ArrowRight" || key == "ArrowUp" ||
         key == "ArrowDown";
}

bool CopyBytesFromValue(napi_env env, napi_value value,
                        std::vector<uint8_t>* out) {
  bool is_buffer = false;
  napi_is_buffer(env, value, &is_buffer);
  if (is_buffer) {
    void* data = nullptr;
    size_t length = 0;
    if (napi_get_buffer_info(env, value, &data, &length) != napi_ok) {
      return false;
    }
    const auto* begin = reinterpret_cast<const uint8_t*>(data);
    out->assign(begin, begin + length);
    return true;
  }

  bool is_array_buffer = false;
  napi_is_arraybuffer(env, value, &is_array_buffer);
  if (is_array_buffer) {
    void* data = nullptr;
    size_t length = 0;
    if (napi_get_arraybuffer_info(env, value, &data, &length) != napi_ok) {
      return false;
    }
    const auto* begin = reinterpret_cast<const uint8_t*>(data);
    out->assign(begin, begin + length);
    return true;
  }

  return false;
}

bool HasUrlScheme(const std::string& url) {
  size_t colon = url.find(':');
  if (colon == std::string::npos) {
    return false;
  }
  size_t slash = url.find('/');
  size_t question = url.find('?');
  size_t hash = url.find('#');
  size_t first_separator =
      std::min(slash == std::string::npos ? url.size() : slash,
               std::min(question == std::string::npos ? url.size() : question,
                        hash == std::string::npos ? url.size() : hash));
  return colon < first_separator;
}

std::string NormalizeUrlPath(std::string url) {
  size_t scheme_end = url.find("://");
  if (scheme_end == std::string::npos) {
    return url;
  }
  size_t path_start = url.find('/', scheme_end + 3);
  if (path_start == std::string::npos) {
    return url;
  }

  size_t suffix_start = url.find_first_of("?#", path_start);
  std::string prefix = url.substr(0, path_start);
  std::string path = url.substr(path_start, suffix_start == std::string::npos
                                                ? std::string::npos
                                                : suffix_start - path_start);
  std::string suffix =
      suffix_start == std::string::npos ? "" : url.substr(suffix_start);

  std::vector<std::string> segments;
  size_t segment_start = 1;
  while (segment_start <= path.size()) {
    size_t segment_end = path.find('/', segment_start);
    std::string segment =
        path.substr(segment_start, segment_end == std::string::npos
                                       ? std::string::npos
                                       : segment_end - segment_start);
    if (segment == "..") {
      if (!segments.empty()) {
        segments.pop_back();
      }
    } else if (!segment.empty() && segment != ".") {
      segments.push_back(segment);
    }
    if (segment_end == std::string::npos) {
      break;
    }
    segment_start = segment_end + 1;
  }

  std::string normalized = prefix + "/";
  for (size_t i = 0; i < segments.size(); ++i) {
    if (i > 0) {
      normalized += "/";
    }
    normalized += segments[i];
  }
  return normalized + suffix;
}

std::string ResolveResourceUrl(const std::string& url,
                               const std::string& base_url) {
  if (url.empty() || HasUrlScheme(url)) {
    return url;
  }
  if (url.rfind("//", 0) == 0) {
    size_t scheme_end = base_url.find(':');
    std::string scheme = scheme_end == std::string::npos
                             ? "https"
                             : base_url.substr(0, scheme_end);
    return scheme + ":" + url;
  }
  if (base_url.empty() || !HasUrlScheme(base_url)) {
    return url;
  }

  size_t scheme_end = base_url.find("://");
  if (scheme_end == std::string::npos) {
    return url;
  }
  size_t origin_end = base_url.find('/', scheme_end + 3);
  std::string origin = origin_end == std::string::npos
                           ? base_url
                           : base_url.substr(0, origin_end);
  if (url[0] == '/') {
    return NormalizeUrlPath(origin + url);
  }

  size_t query_start = base_url.find_first_of("?#");
  std::string base_without_query = query_start == std::string::npos
                                       ? base_url
                                       : base_url.substr(0, query_start);
  size_t last_slash = base_without_query.find_last_of('/');
  std::string base_dir = last_slash == std::string::npos
                             ? base_without_query + "/"
                             : base_without_query.substr(0, last_slash + 1);
  return NormalizeUrlPath(base_dir + url);
}

void CompleteResponseWithError(
    std::shared_ptr<pub::resource::LynxResourceResponse> response,
    const std::string& message) {
  response->SetCode(kLocalErrorCode);
  response->SetErrorMessage(message.c_str());
  response->Complete();
}

void CompleteResponseWithData(
    std::shared_ptr<pub::resource::LynxResourceResponse> response,
    std::vector<uint8_t> data) {
  response->SetCode(0);
  if (data.empty()) {
    response->Complete();
    return;
  }

  auto* content = new uint8_t[data.size()];
  std::memcpy(content, data.data(), data.size());
  response->SetData(
      content, data.size(),
      [](uint8_t* buffer, size_t, void*) { delete[] buffer; }, nullptr);
  response->Complete();
}

std::vector<uint8_t> ReadFile(const std::string& path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    return {};
  }
  std::streamsize size = file.tellg();
  if (size <= 0) {
    return {};
  }
  std::vector<uint8_t> data(static_cast<size_t>(size));
  file.seekg(0, std::ios::beg);
  if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
    return {};
  }
  return data;
}

std::shared_ptr<pub::LynxTemplateData> MakeTemplateData(
    const std::string& json, const std::string& processor = "",
    bool read_only = false) {
  if (json.empty()) {
    return nullptr;
  }
  auto data = std::make_shared<pub::LynxTemplateData>(json);
  if (!processor.empty()) {
    data->MarkState(processor);
  }
  if (read_only) {
    data->MarkReadOnly();
  }
  return data;
}

std::vector<std::string> GetStringArray(napi_env env, napi_value value) {
  std::vector<std::string> strings;
  bool is_array = false;
  if (!value || napi_is_array(env, value, &is_array) != napi_ok || !is_array) {
    return strings;
  }
  uint32_t length = 0;
  napi_get_array_length(env, value, &length);
  strings.reserve(length);
  for (uint32_t index = 0; index < length; ++index) {
    napi_value item = nullptr;
    napi_get_element(env, value, index, &item);
    strings.push_back(GetString(env, item));
  }
  return strings;
}

class JsDispatcher {
 public:
  static JsDispatcher* Create(napi_env env) {
    uv_loop_t* loop = nullptr;
    if (napi_get_uv_event_loop(env, &loop) != napi_ok || !loop) {
      return nullptr;
    }
    return new JsDispatcher(env, loop);
  }

  uv_loop_t* Loop() const { return loop_; }

  bool RunsOnCurrentThread() const {
    return std::this_thread::get_id() == thread_id_;
  }

  void Post(std::function<void()> task) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_ || !async_) {
      return;
    }
    tasks_.push(std::move(task));
    uv_async_send(async_);
  }

  void Dispose() {
    uv_async_t* async = nullptr;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (closed_) {
        return;
      }
      closed_ = true;
      async = async_;
      async_ = nullptr;
      std::queue<std::function<void()>> empty;
      tasks_.swap(empty);
    }
    if (!async) {
      return;
    }
    uv_close(reinterpret_cast<uv_handle_t*>(async), [](uv_handle_t* handle) {
      auto* dispatcher = static_cast<JsDispatcher*>(handle->data);
      delete reinterpret_cast<uv_async_t*>(handle);
      delete dispatcher;
    });
  }

 private:
  JsDispatcher(napi_env env, uv_loop_t* loop)
      : loop_(loop),
        env_(env),
        thread_id_(std::this_thread::get_id()),
        async_(new uv_async_t) {
    uv_async_init(loop_, async_, [](uv_async_t* handle) {
      auto* dispatcher = static_cast<JsDispatcher*>(handle->data);
      dispatcher->Drain();
    });
    async_->data = this;
    uv_unref(reinterpret_cast<uv_handle_t*>(async_));
  }

  ~JsDispatcher() = default;

  void Drain() {
    std::queue<std::function<void()>> tasks;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      tasks.swap(tasks_);
    }
    while (!tasks.empty()) {
      napi_handle_scope scope = nullptr;
      napi_open_handle_scope(env_, &scope);
      tasks.front()();
      if (scope) {
        napi_close_handle_scope(env_, scope);
      }
      tasks.pop();
    }
  }

  uv_loop_t* loop_ = nullptr;
  napi_env env_ = nullptr;
  std::thread::id thread_id_;
  uv_async_t* async_ = nullptr;
  std::mutex mutex_;
  std::queue<std::function<void()>> tasks_;
  bool closed_ = false;
};

struct TimerPayload {
  std::function<void()> task;
};

void PostDelayed(JsDispatcher* dispatcher, uint64_t delay_nanoseconds,
                 std::function<void()> task) {
  if (!dispatcher) {
    return;
  }
  dispatcher->Post([dispatcher, delay_nanoseconds, task = std::move(task)]() {
    uint64_t delay_ms = (delay_nanoseconds + 999999) / 1000000;
    if (delay_ms == 0) {
      task();
      return;
    }
    auto* timer = new uv_timer_t;
    auto* payload = new TimerPayload{std::move(task)};
    uv_timer_init(dispatcher->Loop(), timer);
    timer->data = payload;
    uv_unref(reinterpret_cast<uv_handle_t*>(timer));
    uv_timer_start(
        timer,
        [](uv_timer_t* handle) {
          std::unique_ptr<TimerPayload> payload(
              static_cast<TimerPayload*>(handle->data));
          payload->task();
          uv_timer_stop(handle);
          uv_close(reinterpret_cast<uv_handle_t*>(handle),
                   [](uv_handle_t* closed) {
                     delete reinterpret_cast<uv_timer_t*>(closed);
                   });
        },
        delay_ms, 0);
  });
}

struct DestroyKeepAliveTimer {
  uv_loop_t* loop = nullptr;
  uv_timer_t timer;
};

std::mutex& DestroyKeepAliveMutex() {
  static std::mutex mutex;
  return mutex;
}

std::unordered_map<uv_loop_t*, DestroyKeepAliveTimer*>&
DestroyKeepAliveTimers() {
  static std::unordered_map<uv_loop_t*, DestroyKeepAliveTimer*> timers;
  return timers;
}

void ScheduleExplicitDestroyGracePeriod(napi_env env) {
  uv_loop_t* loop = nullptr;
  if (napi_get_uv_event_loop(env, &loop) != napi_ok || !loop) {
    return;
  }

  std::lock_guard<std::mutex> lock(DestroyKeepAliveMutex());
  auto& timers = DestroyKeepAliveTimers();
  auto it = timers.find(loop);
  DestroyKeepAliveTimer* state = it == timers.end() ? nullptr : it->second;
  if (!state || uv_is_closing(reinterpret_cast<uv_handle_t*>(&state->timer))) {
    state = new DestroyKeepAliveTimer();
    state->loop = loop;
    uv_timer_init(loop, &state->timer);
    state->timer.data = state;
    timers[loop] = state;
  }

  // Keep Node alive briefly after explicit destroy so Lynx async teardown can
  // drain before natural process exit.
  uv_timer_start(
      &state->timer,
      [](uv_timer_t* handle) {
        auto* state = static_cast<DestroyKeepAliveTimer*>(handle->data);
        {
          std::lock_guard<std::mutex> lock(DestroyKeepAliveMutex());
          auto& timers = DestroyKeepAliveTimers();
          auto it = timers.find(state->loop);
          if (it != timers.end() && it->second == state) {
            timers.erase(it);
          }
        }
        uv_timer_stop(handle);
        uv_close(reinterpret_cast<uv_handle_t*>(handle),
                 [](uv_handle_t* closed) {
                   delete static_cast<DestroyKeepAliveTimer*>(closed->data);
                 });
      },
      kExplicitDestroyGracePeriodMs, 0);
}

class FmlMessageLoopPump {
 public:
  static void Init(napi_env env) {
    static std::once_flag once;
    std::call_once(once, [env]() {
      base::UIThread::Init();
      uv_loop_t* loop = nullptr;
      if (napi_get_uv_event_loop(env, &loop) != napi_ok || !loop) {
        return;
      }
      auto* timer = new uv_timer_t;
      uv_timer_init(loop, timer);
      uv_unref(reinterpret_cast<uv_handle_t*>(timer));
      uv_timer_start(
          timer,
          [](uv_timer_t*) {
            auto* message_loop =
                fml::MessageLoop::IsInitializedForCurrentThread();
            if (message_loop) {
              message_loop->RunExpiredTasksNow();
            }
          },
          0, 1);
    });
  }
};

struct DebugRouterCallbackState {
  napi_env env = nullptr;
  JsDispatcher* dispatcher = nullptr;
  napi_ref open_card_ref = nullptr;
  napi_ref close_page_ref = nullptr;
  std::mutex mutex;
  bool cleanup_registered = false;
  bool app_handlers_registered = false;
};

DebugRouterCallbackState& GetDebugRouterCallbackState() {
  static DebugRouterCallbackState state;
  return state;
}

std::string MakeDebugRouterAppResponse(int code, const std::string& message) {
  Json::Value response(Json::objectValue);
  response["code"] = code;
  response["message"] = message;
  Json::FastWriter writer;
  return writer.write(response);
}

void CleanupDebugRouterCallbacks(void*) {
  auto& state = GetDebugRouterCallbackState();
  napi_env env = nullptr;
  napi_ref open_card_ref = nullptr;
  napi_ref close_page_ref = nullptr;
  JsDispatcher* dispatcher = nullptr;
  {
    std::lock_guard<std::mutex> lock(state.mutex);
    env = state.env;
    open_card_ref = state.open_card_ref;
    close_page_ref = state.close_page_ref;
    dispatcher = state.dispatcher;
    state.env = nullptr;
    state.open_card_ref = nullptr;
    state.close_page_ref = nullptr;
    state.dispatcher = nullptr;
    state.cleanup_registered = false;
  }
  lynx_env_set_open_card_callback(nullptr, nullptr);
  if (env && open_card_ref) {
    napi_delete_reference(env, open_card_ref);
  }
  if (env && close_page_ref) {
    napi_delete_reference(env, close_page_ref);
  }
  if (dispatcher) {
    dispatcher->Dispose();
  }
}

bool EnsureDebugRouterCallbackRuntime(napi_env env) {
  FmlMessageLoopPump::Init(env);
  auto& state = GetDebugRouterCallbackState();
  std::lock_guard<std::mutex> lock(state.mutex);
  state.env = env;
  if (!state.dispatcher) {
    state.dispatcher = JsDispatcher::Create(env);
  }
  if (!state.dispatcher) {
    return false;
  }
  if (!state.cleanup_registered) {
    napi_add_env_cleanup_hook(env, CleanupDebugRouterCallbacks, nullptr);
    state.cleanup_registered = true;
  }
  return true;
}

bool DispatchOpenCardCallback(const std::string& url) {
  auto& state = GetDebugRouterCallbackState();
  JsDispatcher* dispatcher = nullptr;
  {
    std::lock_guard<std::mutex> lock(state.mutex);
    if (!state.env || !state.dispatcher || !state.open_card_ref) {
      return false;
    }
    dispatcher = state.dispatcher;
  }
  dispatcher->Post([url]() {
    auto& state = GetDebugRouterCallbackState();
    napi_env env = nullptr;
    napi_ref callback_ref = nullptr;
    {
      std::lock_guard<std::mutex> lock(state.mutex);
      env = state.env;
      callback_ref = state.open_card_ref;
    }
    if (!env || !callback_ref) {
      return;
    }
    napi_value callback = nullptr;
    napi_get_reference_value(env, callback_ref, &callback);
    if (!callback) {
      return;
    }
    napi_value global = nullptr;
    napi_get_global(env, &global);
    napi_value arg = nullptr;
    napi_create_string_utf8(env, url.c_str(), url.size(), &arg);
    napi_value result = nullptr;
    napi_call_function(env, global, callback, 1, &arg, &result);
  });
  return true;
}

bool DispatchClosePageCallback() {
  auto& state = GetDebugRouterCallbackState();
  JsDispatcher* dispatcher = nullptr;
  {
    std::lock_guard<std::mutex> lock(state.mutex);
    if (!state.env || !state.dispatcher || !state.close_page_ref) {
      return false;
    }
    dispatcher = state.dispatcher;
  }
  dispatcher->Post([]() {
    auto& state = GetDebugRouterCallbackState();
    napi_env env = nullptr;
    napi_ref callback_ref = nullptr;
    {
      std::lock_guard<std::mutex> lock(state.mutex);
      env = state.env;
      callback_ref = state.close_page_ref;
    }
    if (!env || !callback_ref) {
      return;
    }
    napi_value callback = nullptr;
    napi_get_reference_value(env, callback_ref, &callback);
    if (!callback) {
      return;
    }
    napi_value global = nullptr;
    napi_get_global(env, &global);
    napi_value result = nullptr;
    napi_call_function(env, global, callback, 0, nullptr, &result);
  });
  return true;
}

void HandleDebugRouterOpenCard(void*, const char* url) {
  DispatchOpenCardCallback(url ? url : "");
}

class NodeLynxDebugRouterMessageHandler final
    : public debugrouter::core::DebugRouterMessageHandler {
 public:
  explicit NodeLynxDebugRouterMessageHandler(std::string name)
      : name_(std::move(name)) {}

  std::string Handle(std::string params) override {
    if (name_ == "App.openPage") {
      Json::Value root;
      Json::Reader reader;
      if (!reader.parse(params, root, false) || !root.isObject() ||
          !root["url"].isString() || root["url"].asString().empty()) {
        return MakeDebugRouterAppResponse(-1, "App.openPage requires url");
      }
      if (!DispatchOpenCardCallback(root["url"].asString())) {
        return MakeDebugRouterAppResponse(-1, "OpenCard callback is not set");
      }
      return MakeDebugRouterAppResponse(0, "success");
    }
    if (name_ == "App.closePage") {
      if (!DispatchClosePageCallback()) {
        return MakeDebugRouterAppResponse(-1, "closePage callback is not set");
      }
      return MakeDebugRouterAppResponse(0, "success");
    }
    return MakeDebugRouterAppResponse(-2, "not implemented");
  }

  std::string GetName() const override { return name_; }

 private:
  std::string name_;
};

void EnsureDebugRouterAppHandlersRegistered() {
  auto& state = GetDebugRouterCallbackState();
  std::lock_guard<std::mutex> lock(state.mutex);
  if (state.app_handlers_registered) {
    return;
  }
  debugrouter::core::DebugRouterCore::GetInstance().AddMessageHandler(
      new NodeLynxDebugRouterMessageHandler("App.openPage"));
  debugrouter::core::DebugRouterCore::GetInstance().AddMessageHandler(
      new NodeLynxDebugRouterMessageHandler("App.closePage"));
  state.app_handlers_registered = true;
}

class NodeLynxRenderer;

struct ViewState {
  napi_env env = nullptr;
  JsDispatcher* dispatcher = nullptr;
  std::mutex mutex;
  bool destroyed = false;
  bool window_closed = false;
  bool has_frame = false;
  int expected_frame_width = 0;
  int frame_width = 0;
  int frame_height = 0;
  std::vector<uint8_t> rgba_frame;
  std::vector<napi_deferred> frame_waiters;
  std::vector<napi_deferred> load_waiters;
  std::vector<napi_deferred> cdp_waiters;
  std::vector<napi_deferred> close_waiters;
  std::weak_ptr<NodeLynxRenderer> renderer;
  napi_ref resource_fetcher_ref = nullptr;
  napi_ref error_handler_ref = nullptr;
};

JsDispatcher* GetDispatcher(const std::shared_ptr<ViewState>& state) {
  if (!state) {
    return nullptr;
  }
  std::lock_guard<std::mutex> lock(state->mutex);
  if (state->destroyed) {
    return nullptr;
  }
  return state->dispatcher;
}

void ResolveWaiters(napi_env env, std::vector<napi_deferred> waiters) {
  napi_value undefined = GetUndefined(env);
  for (auto deferred : waiters) {
    napi_resolve_deferred(env, deferred, undefined);
  }
}

void RejectWaiters(napi_env env, std::vector<napi_deferred> waiters,
                   const std::string& message) {
  napi_value error = CreateError(env, message);
  for (auto deferred : waiters) {
    napi_reject_deferred(env, deferred, error);
  }
}

class NodeLynxRenderer final : public pub::LynxWindowlessRenderer {
 public:
  using AcceleratedPresenter =
      std::function<bool(const lynx_accelerated_paint_info_t&)>;
  using SoftwarePresenter =
      std::function<bool(const uint8_t* rgba, int width, int height)>;
  using TextInputVisibilityCallback = std::function<void(bool show)>;
  using TextInputRectCallback =
      std::function<void(float x, float y, float width, float height)>;

  NodeLynxRenderer(
      std::weak_ptr<ViewState> state,
      lynx_windowless_renderer_type_e renderer_type,
      AcceleratedPresenter accelerated_presenter = {},
      SoftwarePresenter software_presenter = {},
      TextInputVisibilityCallback text_input_visibility_callback = {},
      TextInputRectCallback caret_rect_callback = {},
      TextInputRectCallback marked_text_rect_callback = {})
      : pub::LynxWindowlessRenderer(renderer_type),
        state_(std::move(state)),
        accelerated_presenter_(std::move(accelerated_presenter)),
        software_presenter_(std::move(software_presenter)),
        text_input_visibility_callback_(
            std::move(text_input_visibility_callback)),
        caret_rect_callback_(std::move(caret_rect_callback)),
        marked_text_rect_callback_(std::move(marked_text_rect_callback)) {}

  bool OnSoftwarePresent(const void* allocation, size_t row_bytes,
                         size_t height) override {
    if (!allocation || row_bytes == 0 || height == 0) {
      return false;
    }
    auto state = state_.lock();
    if (!state) {
      return false;
    }
    int width = 0;
    {
      std::lock_guard<std::mutex> lock(state->mutex);
      width = state->expected_frame_width;
    }
    size_t packed_row_bytes = static_cast<size_t>(width) * 4;
    if (width <= 0 || packed_row_bytes > row_bytes) {
      width = static_cast<int>(row_bytes / 4);
      packed_row_bytes = static_cast<size_t>(width) * 4;
    }
    if (width <= 0 || packed_row_bytes == 0) {
      return false;
    }
    int frame_height = static_cast<int>(height);
    const auto* src = reinterpret_cast<const uint8_t*>(allocation);
    std::vector<uint8_t> rgba =
        CopyNativeN32PixelsToRgba(src, row_bytes, width, frame_height);
    if (software_presenter_) {
      software_presenter_(rgba.data(), width, frame_height);
    }

    std::vector<napi_deferred> frame_waiters;
    std::vector<napi_deferred> load_waiters;
    JsDispatcher* dispatcher = nullptr;
    {
      std::lock_guard<std::mutex> lock(state->mutex);
      if (state->destroyed) {
        return false;
      }
      state->frame_width = width;
      state->frame_height = frame_height;
      state->rgba_frame = std::move(rgba);
      state->has_frame = true;
      frame_waiters.swap(state->frame_waiters);
      load_waiters.swap(state->load_waiters);
      dispatcher = state->dispatcher;
    }

    if ((!frame_waiters.empty() || !load_waiters.empty()) && dispatcher) {
      dispatcher->Post([env = state->env,
                        frame_waiters = std::move(frame_waiters),
                        load_waiters = std::move(load_waiters)]() mutable {
        ResolveWaiters(env, std::move(frame_waiters));
        ResolveWaiters(env, std::move(load_waiters));
      });
    }
    return true;
  }

  bool OnAcceleratedPresent() override {
    lynx_accelerated_paint_info_t paint_info{};
    if (!GetAcceleratedPaintInfo(&paint_info) || paint_info.width == 0 ||
        paint_info.height == 0) {
      return false;
    }
    if (accelerated_presenter_ && !accelerated_presenter_(paint_info)) {
      return false;
    }
    auto state = state_.lock();
    if (!state) {
      return false;
    }
    std::vector<napi_deferred> frame_waiters;
    std::vector<napi_deferred> load_waiters;
    JsDispatcher* dispatcher = nullptr;
    {
      std::lock_guard<std::mutex> lock(state->mutex);
      if (state->destroyed) {
        return false;
      }
      state->frame_width = static_cast<int>(paint_info.width);
      state->frame_height = static_cast<int>(paint_info.height);
      state->rgba_frame.clear();
      state->has_frame = true;
      frame_waiters.swap(state->frame_waiters);
      load_waiters.swap(state->load_waiters);
      dispatcher = state->dispatcher;
    }
    if ((!frame_waiters.empty() || !load_waiters.empty()) && dispatcher) {
      dispatcher->Post([env = state->env,
                        frame_waiters = std::move(frame_waiters),
                        load_waiters = std::move(load_waiters)]() mutable {
        ResolveWaiters(env, std::move(frame_waiters));
        ResolveWaiters(env, std::move(load_waiters));
      });
    }
    return true;
  }

  void ShowTextInput(bool show) override {
    if (text_input_visibility_callback_) {
      text_input_visibility_callback_(show);
    }
  }

  void UpdateCaretPosition(float x, float y, float width,
                           float height) override {
    if (caret_rect_callback_) {
      caret_rect_callback_(x, y, width, height);
    }
  }

  void SetMarkedTextRect(float x, float y, float width, float height) override {
    if (marked_text_rect_callback_) {
      marked_text_rect_callback_(x, y, width, height);
    }
  }

  void OnPostTask(lynx_task_t task, uint64_t interval_nanoseconds) override {
    auto state = state_.lock();
    JsDispatcher* dispatcher = nullptr;
    std::weak_ptr<NodeLynxRenderer> renderer;
    {
      if (!state) {
        return;
      }
      std::lock_guard<std::mutex> lock(state->mutex);
      if (state->destroyed || !state->dispatcher) {
        return;
      }
      dispatcher = state->dispatcher;
      renderer = state->renderer;
    }
    PostDelayed(dispatcher, interval_nanoseconds, [renderer, task]() {
      if (auto locked = renderer.lock()) {
        locked->RunTask(task);
      }
    });
  }

 private:
  std::weak_ptr<ViewState> state_;
  AcceleratedPresenter accelerated_presenter_;
  SoftwarePresenter software_presenter_;
  TextInputVisibilityCallback text_input_visibility_callback_;
  TextInputRectCallback caret_rect_callback_;
  TextInputRectCallback marked_text_rect_callback_;
};

class HeadlessClient final : public pub::LynxViewClient {
 public:
  explicit HeadlessClient(std::weak_ptr<ViewState> state)
      : state_(std::move(state)) {}

  void OnLoadSuccess() override {
    auto state = state_.lock();
    if (!state) {
      return;
    }
    std::vector<napi_deferred> waiters;
    JsDispatcher* dispatcher = nullptr;
    {
      std::lock_guard<std::mutex> lock(state->mutex);
      if (state->destroyed) {
        return;
      }
      waiters.swap(state->load_waiters);
      dispatcher = state->dispatcher;
    }
    if (!waiters.empty() && dispatcher) {
      dispatcher->Post(
          [env = state->env, waiters = std::move(waiters)]() mutable {
            ResolveWaiters(env, std::move(waiters));
          });
    }
  }

  void OnReceivedError(int error_code, const char* message) override {
    auto state = state_.lock();
    if (!state) {
      return;
    }
    std::string error = "Lynx render error ";
    error += std::to_string(error_code);
    error += ": ";
    error += message ? message : "";
    std::vector<napi_deferred> load_waiters;
    std::vector<napi_deferred> frame_waiters;
    JsDispatcher* dispatcher = nullptr;
    {
      std::lock_guard<std::mutex> lock(state->mutex);
      if (state->destroyed) {
        return;
      }
      load_waiters.swap(state->load_waiters);
      frame_waiters.swap(state->frame_waiters);
      dispatcher = state->dispatcher;
    }
    if ((!load_waiters.empty() || !frame_waiters.empty()) && dispatcher) {
      dispatcher->Post([env = state->env,
                        load_waiters = std::move(load_waiters),
                        frame_waiters = std::move(frame_waiters),
                        error = std::move(error)]() mutable {
        RejectWaiters(env, std::move(load_waiters), error);
        RejectWaiters(env, std::move(frame_waiters), error);
      });
    }
  }

 private:
  std::weak_ptr<ViewState> state_;
};

class HeadlessTemplateClient final : public embedder::TemplateRendererClient {
 public:
  explicit HeadlessTemplateClient(std::weak_ptr<ViewState> state)
      : state_(std::move(state)) {}

  void OnErrorOccurred(
      int level, int32_t error_code, const std::string& message,
      const std::string& fix_suggestion,
      const std::unordered_map<std::string, std::string>& custom_info,
      bool is_logbox_only) override {
    auto state = state_.lock();
    JsDispatcher* dispatcher = nullptr;
    std::vector<napi_deferred> load_waiters;
    std::vector<napi_deferred> frame_waiters;
    bool has_error_handler = false;
    {
      if (!state) {
        return;
      }
      std::lock_guard<std::mutex> lock(state->mutex);
      if (state->destroyed || !state->dispatcher) {
        return;
      }
      if (IsLoadTemplateError(error_code, is_logbox_only)) {
        load_waiters.swap(state->load_waiters);
        frame_waiters.swap(state->frame_waiters);
      }
      has_error_handler = state->error_handler_ref != nullptr;
      dispatcher = state->dispatcher;
    }
    if (!dispatcher) {
      return;
    }
    std::string level_string = base::LynxError::GetLevelString(level);
    std::string load_error =
        "Lynx render error " + std::to_string(error_code) + ": " + message;
    dispatcher->Post([state, level_string = std::move(level_string), error_code,
                      message, fix_suggestion, custom_info, is_logbox_only,
                      load_error = std::move(load_error),
                      load_waiters = std::move(load_waiters),
                      frame_waiters = std::move(frame_waiters),
                      has_error_handler]() mutable {
      napi_env env = state->env;
      RejectWaiters(env, std::move(load_waiters), load_error);
      RejectWaiters(env, std::move(frame_waiters), load_error);
      if (!has_error_handler) {
        return;
      }
      napi_value handler = nullptr;
      if (napi_get_reference_value(env, state->error_handler_ref, &handler) !=
              napi_ok ||
          !handler) {
        return;
      }

      napi_value global = nullptr;
      napi_get_global(env, &global);
      napi_value args[6] = {nullptr, nullptr, nullptr,
                            nullptr, nullptr, nullptr};
      napi_create_string_utf8(env, level_string.c_str(), level_string.size(),
                              &args[0]);
      napi_create_int32(env, error_code, &args[1]);
      napi_create_string_utf8(env, message.c_str(), message.size(), &args[2]);
      napi_create_string_utf8(env, fix_suggestion.c_str(),
                              fix_suggestion.size(), &args[3]);
      napi_create_object(env, &args[4]);
      for (const auto& item : custom_info) {
        napi_value value = nullptr;
        napi_create_string_utf8(env, item.second.c_str(), item.second.size(),
                                &value);
        napi_set_named_property(env, args[4], item.first.c_str(), value);
      }
      napi_get_boolean(env, is_logbox_only, &args[5]);
      napi_value result = nullptr;
      napi_call_function(env, global, handler, 6, args, &result);
    });
  }

 private:
  static bool IsLoadTemplateError(int32_t error_code, bool is_logbox_only) {
    return !is_logbox_only &&
           (error_code == error::E_APP_BUNDLE_LOAD_RENDER_FAILED ||
            error_code == error::E_APP_BUNDLE_LOAD_PARSE_FAILED);
  }

  std::weak_ptr<ViewState> state_;
};

struct ResourceCallbackData {
  std::shared_ptr<pub::resource::LynxResourceResponse> response;
  std::atomic_bool completed{false};
};

bool CompleteResourceCallbackWithError(ResourceCallbackData* callback_data,
                                       const std::string& message) {
  if (!callback_data || !callback_data->response ||
      callback_data->completed.exchange(true)) {
    return false;
  }
  auto response = callback_data->response;
  CompleteResponseWithError(response, message);
  callback_data->response.reset();
  return true;
}

bool CompleteResourceCallbackWithData(ResourceCallbackData* callback_data,
                                      std::vector<uint8_t> bytes) {
  if (!callback_data || !callback_data->response ||
      callback_data->completed.exchange(true)) {
    return false;
  }
  auto response = callback_data->response;
  CompleteResponseWithData(response, std::move(bytes));
  callback_data->response.reset();
  return true;
}

void ResourceCallbackFinalize(napi_env, void* finalize_data, void*) {
  auto* callback_data = static_cast<ResourceCallbackData*>(finalize_data);
  CompleteResourceCallbackWithError(callback_data,
                                    "resource fetch callback was released");
  delete callback_data;
}

napi_value ResourceFetchCallback(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2] = {nullptr, nullptr};
  void* data = nullptr;
  napi_get_cb_info(env, info, &argc, args, nullptr, &data);
  auto* callback_data = static_cast<ResourceCallbackData*>(data);
  if (!callback_data || !callback_data->response ||
      callback_data->completed.load()) {
    return GetUndefined(env);
  }

  napi_valuetype error_type = napi_undefined;
  if (argc > 0 && args[0]) {
    napi_typeof(env, args[0], &error_type);
  }
  if (error_type != napi_undefined && error_type != napi_null) {
    std::string message = "resource fetch failed";
    napi_value message_value = nullptr;
    if (napi_get_named_property(env, args[0], "message", &message_value) ==
        napi_ok) {
      std::string error_message = GetString(env, message_value);
      if (!error_message.empty()) {
        message = error_message;
      }
    }
    CompleteResourceCallbackWithError(callback_data, message);
    return GetUndefined(env);
  }

  if (argc < 2 || !args[1]) {
    CompleteResourceCallbackWithError(callback_data,
                                      "resource fetch returned empty data");
    return GetUndefined(env);
  }

  std::vector<uint8_t> bytes;
  if (!CopyBytesFromValue(env, args[1], &bytes)) {
    CompleteResourceCallbackWithError(
        callback_data, "resource fetch returned non-binary data");
    return GetUndefined(env);
  }
  CompleteResourceCallbackWithData(callback_data, std::move(bytes));
  return GetUndefined(env);
}

class HeadlessResourceFetcher final : public pub::LynxGenericResourceFetcher {
 public:
  HeadlessResourceFetcher(std::weak_ptr<ViewState> state,
                          std::string resources_path)
      : state_(std::move(state)), resources_path_(std::move(resources_path)) {}

  void SetBaseUrl(std::string base_url) {
    std::lock_guard<std::mutex> lock(base_url_mutex_);
    base_url_ = std::move(base_url);
  }

  std::string InterceptUrl(const std::string& origin_url,
                           bool should_decode) override {
    (void)should_decode;
    std::string base_url;
    {
      std::lock_guard<std::mutex> lock(base_url_mutex_);
      base_url = base_url_;
    }
    return ResolveResourceUrl(origin_url, base_url);
  }

  void FetchResource(
      std::shared_ptr<pub::resource::LynxResourceRequest> request,
      std::shared_ptr<pub::resource::LynxResourceResponse> response) override {
    std::string url = request->GetUrl() ? request->GetUrl() : "";
    lynx_resource_type_e type = request->GetType();
    if (type == kLynxResourceTypeLynxCoreJS ||
        url.find("lynx_core.js") != std::string::npos) {
      auto data = ReadFile(resources_path_ + "/lynx_core.js");
      if (data.empty()) {
        CompleteResponseWithError(response,
                                  "failed to read resources/lynx_core.js");
      } else {
        CompleteResponseWithData(response, std::move(data));
      }
      return;
    }

    {
      std::lock_guard<std::mutex> lock(base_url_mutex_);
      url = ResolveResourceUrl(url, base_url_);
    }

    auto state = state_.lock();
    JsDispatcher* dispatcher = nullptr;
    {
      if (!state) {
        CompleteResponseWithError(response, "resource fetcher is unavailable");
        return;
      }
      std::lock_guard<std::mutex> lock(state->mutex);
      if (state->destroyed || !state->dispatcher ||
          !state->resource_fetcher_ref) {
        CompleteResponseWithError(response, "resource fetcher is unavailable");
        return;
      }
      dispatcher = state->dispatcher;
    }

    dispatcher->Post([state, response, url = std::move(url), type]() mutable {
      napi_env env = state->env;
      napi_value fetcher = nullptr;
      if (napi_get_reference_value(env, state->resource_fetcher_ref,
                                   &fetcher) != napi_ok ||
          !fetcher) {
        CompleteResponseWithError(response,
                                  "resource fetcher reference is invalid");
        return;
      }

      napi_value global = nullptr;
      napi_get_global(env, &global);
      napi_value args[3] = {nullptr, nullptr, nullptr};
      napi_create_string_utf8(env, url.c_str(), url.size(), &args[0]);
      napi_create_int32(env, static_cast<int32_t>(type), &args[1]);
      auto* callback_data = new ResourceCallbackData{response};
      napi_status callback_status =
          napi_create_function(env, "resourceCallback", NAPI_AUTO_LENGTH,
                               ResourceFetchCallback, callback_data, &args[2]);
      if (callback_status != napi_ok ||
          napi_add_finalizer(env, args[2], callback_data,
                             ResourceCallbackFinalize, nullptr,
                             nullptr) != napi_ok) {
        CompleteResourceCallbackWithError(
            callback_data, "failed to create resource fetch callback");
        delete callback_data;
        return;
      }

      napi_value result = nullptr;
      napi_status status =
          napi_call_function(env, global, fetcher, 3, args, &result);
      if (status != napi_ok) {
        bool exception_pending = false;
        napi_is_exception_pending(env, &exception_pending);
        if (exception_pending) {
          napi_value ignored = nullptr;
          napi_get_and_clear_last_exception(env, &ignored);
        }
        CompleteResourceCallbackWithError(callback_data,
                                          "resource fetcher call failed");
      }
    });
  }

  void FetchResourcePath(
      std::shared_ptr<pub::resource::LynxResourceRequest> request,
      std::shared_ptr<pub::resource::LynxResourceResponse> response) override {
    FetchResource(std::move(request), std::move(response));
  }

 private:
  std::weak_ptr<ViewState> state_;
  std::string resources_path_;
  std::mutex base_url_mutex_;
  std::string base_url_;
};

class HeadlessLynxViewNode {
 public:
  HeadlessLynxViewNode(napi_env env, std::string resources_path, double width,
                       double height, double device_pixel_ratio,
                       napi_value resource_fetcher, bool windowed = false,
                       WindowedLynxViewOptions window_options = {})
      : env_(env),
        resources_path_(std::move(resources_path)),
        width_(width),
        height_(height),
        device_pixel_ratio_(device_pixel_ratio),
        windowed_(windowed),
        window_options_(std::move(window_options)) {
    if (window_options_.width <= 0) {
      window_options_.width = width_;
    }
    if (window_options_.height <= 0) {
      window_options_.height = height_;
    }
    if (window_options_.device_pixel_ratio <= 0) {
      window_options_.device_pixel_ratio = device_pixel_ratio_;
    }
    dispatcher_ = JsDispatcher::Create(env);
    state_ = std::make_shared<ViewState>();
    state_->env = env;
    state_->dispatcher = dispatcher_;
    state_->expected_frame_width = ToPixelSize(width_, device_pixel_ratio_);
    if (IsFunction(env, resource_fetcher)) {
      napi_create_reference(env, resource_fetcher, 1,
                            &state_->resource_fetcher_ref);
    }
    BuildView();
  }

  ~HeadlessLynxViewNode() { Destroy(DestroyReason::kFinalizer); }

  static napi_value Init(napi_env env, napi_value exports) {
    FmlMessageLoopPump::Init(env);

    napi_property_descriptor properties[] = {
        {"_loadTemplate", nullptr, LoadTemplate, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"_updateData", nullptr, UpdateData, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"_updateGlobalProps", nullptr, UpdateGlobalProps, nullptr, nullptr,
         nullptr, napi_default, nullptr},
        {"_invokeCDPFromSDK", nullptr, InvokeCDPFromSDK, nullptr, nullptr,
         nullptr, napi_default, nullptr},
        {"_sendTouchEvent", nullptr, SendTouchEvent, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"_setErrorHandler", nullptr, SetErrorHandler, nullptr, nullptr,
         nullptr, napi_default, nullptr},
        {"_waitForFrame", nullptr, WaitForFrame, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"_forceFrame", nullptr, ForceFrame, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"_captureFrame", nullptr, CaptureFrame, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"_destroy", nullptr, DestroyMethod, nullptr, nullptr, nullptr,
         napi_default, nullptr},
    };

    napi_value cons = nullptr;
    napi_define_class(env, "HeadlessLynxViewNative", NAPI_AUTO_LENGTH, New,
                      nullptr, sizeof(properties) / sizeof(properties[0]),
                      properties, &cons);
    napi_set_named_property(env, exports, "HeadlessLynxViewNative", cons);

    napi_property_descriptor windowed_properties[] = {
        {"_loadTemplate", nullptr, LoadTemplate, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"_updateData", nullptr, UpdateData, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"_updateGlobalProps", nullptr, UpdateGlobalProps, nullptr, nullptr,
         nullptr, napi_default, nullptr},
        {"_invokeCDPFromSDK", nullptr, InvokeCDPFromSDK, nullptr, nullptr,
         nullptr, napi_default, nullptr},
        {"_setErrorHandler", nullptr, SetErrorHandler, nullptr, nullptr,
         nullptr, napi_default, nullptr},
        {"_waitForFrame", nullptr, WaitForFrame, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"_forceFrame", nullptr, ForceFrame, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"_captureFrame", nullptr, CaptureFrame, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"_show", nullptr, ShowWindow, nullptr, nullptr, nullptr, napi_default,
         nullptr},
        {"_close", nullptr, CloseWindow, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"_waitUntilClosed", nullptr, WaitUntilClosed, nullptr, nullptr,
         nullptr, napi_default, nullptr},
        {"_click", nullptr, ClickWindow, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"_typeText", nullptr, TypeTextWindow, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"_pressKey", nullptr, PressKeyWindow, nullptr, nullptr, nullptr,
         napi_default, nullptr},
        {"_destroy", nullptr, DestroyMethod, nullptr, nullptr, nullptr,
         napi_default, nullptr},
    };

    napi_value windowed_cons = nullptr;
    napi_define_class(
        env, "WindowedLynxViewNative", NAPI_AUTO_LENGTH, NewWindowed, nullptr,
        sizeof(windowed_properties) / sizeof(windowed_properties[0]),
        windowed_properties, &windowed_cons);
    napi_set_named_property(env, exports, "WindowedLynxViewNative",
                            windowed_cons);
    return exports;
  }

 private:
  static HeadlessLynxViewNode* Unwrap(napi_env env, napi_value js_this) {
    HeadlessLynxViewNode* obj = nullptr;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
    return obj;
  }

  static napi_value New(napi_env env, napi_callback_info info) {
    napi_value js_this = nullptr;
    size_t argc = 5;
    napi_value args[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

    if (argc < 5) {
      ThrowError(env, "HeadlessLynxViewNative requires constructor arguments");
      return nullptr;
    }

    double width = 0;
    double height = 0;
    double dpr = 0;
    if (!GetDouble(env, args[1], &width) || !GetDouble(env, args[2], &height) ||
        !GetDouble(env, args[3], &dpr)) {
      ThrowError(env, "invalid viewport options");
      return nullptr;
    }

    auto* obj = new HeadlessLynxViewNode(env, GetString(env, args[0]), width,
                                         height, dpr, args[4]);
    napi_wrap(
        env, js_this, obj,
        [](napi_env, void* data, void*) {
          delete static_cast<HeadlessLynxViewNode*>(data);
        },
        nullptr, nullptr);
    return js_this;
  }

  static napi_value NewWindowed(napi_env env, napi_callback_info info) {
    napi_value js_this = nullptr;
    size_t argc = 8;
    napi_value args[8] = {nullptr, nullptr, nullptr, nullptr,
                          nullptr, nullptr, nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

    if (argc < 5) {
      ThrowError(env, "WindowedLynxViewNative requires constructor arguments");
      return nullptr;
    }

    double width = 0;
    double height = 0;
    double dpr = 0;
    if (!GetDouble(env, args[1], &width) || !GetDouble(env, args[2], &height) ||
        !GetDouble(env, args[3], &dpr)) {
      ThrowError(env, "invalid viewport options");
      return nullptr;
    }

    WindowedLynxViewOptions window_options;
    window_options.width = width;
    window_options.height = height;
    window_options.device_pixel_ratio = dpr;
    window_options.title = argc > 5 ? GetString(env, args[5]) : "";
    window_options.resizable = argc > 6 ? GetBool(env, args[6], true) : true;
    window_options.visible = argc > 7 ? GetBool(env, args[7], true) : true;

    auto* obj =
        new HeadlessLynxViewNode(env, GetString(env, args[0]), width, height,
                                 dpr, args[4], true, std::move(window_options));
    if (!obj->window_host_) {
      delete obj;
      ThrowError(env, "WindowedLynxView is only supported on macOS");
      return nullptr;
    }
    napi_wrap(
        env, js_this, obj,
        [](napi_env, void* data, void*) {
          delete static_cast<HeadlessLynxViewNode*>(data);
        },
        nullptr, nullptr);
    return js_this;
  }

  static napi_value LoadTemplate(napi_env env, napi_callback_info info) {
    napi_value js_this = nullptr;
    size_t argc = 7;
    napi_value args[7] = {nullptr, nullptr, nullptr, nullptr,
                          nullptr, nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
    auto* obj = Unwrap(env, js_this);
    if (!obj || obj->destroyed_) {
      napi_value promise = nullptr;
      napi_deferred deferred = nullptr;
      napi_create_promise(env, &deferred, &promise);
      napi_reject_deferred(env, deferred,
                           CreateError(env, "view is destroyed"));
      return promise;
    }

    std::vector<uint8_t> bytes;
    if (argc < 2 || !CopyBytesFromValue(env, args[1], &bytes)) {
      napi_value promise = nullptr;
      napi_deferred deferred = nullptr;
      napi_create_promise(env, &deferred, &promise);
      napi_reject_deferred(env, deferred,
                           CreateError(env, "template must be binary data"));
      return promise;
    }
    napi_value promise = nullptr;
    napi_deferred deferred = nullptr;
    napi_create_promise(env, &deferred, &promise);
    {
      std::lock_guard<std::mutex> lock(obj->state_->mutex);
      obj->state_->load_waiters.push_back(deferred);
      obj->state_->has_frame = false;
      obj->state_->rgba_frame.clear();
    }

    std::string url = argc > 0 ? GetString(env, args[0]) : "";
    if (obj->fetcher_) {
      obj->fetcher_->SetBaseUrl(url);
    }
    std::string initial_data = argc > 2 ? GetString(env, args[2]) : "";
    std::string global_props = argc > 3 ? GetString(env, args[3]) : "";
    std::string processor = argc > 4 ? GetString(env, args[4]) : "";
    bool read_only = argc > 5 ? GetBool(env, args[5]) : false;
    bool enable_recycle_template_bundle =
        argc > 6 ? GetBool(env, args[6]) : false;

    auto initial_template_data =
        MakeTemplateData(initial_data, processor, read_only);
    if (!global_props.empty()) {
      obj->global_props_ = MakeTemplateData(global_props);
      obj->TemplateRenderer()->UpdateGlobalProps(
          obj->global_props_->Impl()->template_data->GetValue());
    }

    obj->TemplateRenderer()->LoadTemplate(
        url, std::move(bytes), nullptr,
        initial_template_data ? initial_template_data->Impl()->template_data
                              : nullptr,
        enable_recycle_template_bundle);
    return promise;
  }

  static napi_value UpdateData(napi_env env, napi_callback_info info) {
    napi_value js_this = nullptr;
    size_t argc = 4;
    napi_value args[4] = {nullptr, nullptr, nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
    auto* obj = Unwrap(env, js_this);
    if (!obj || obj->destroyed_) {
      ThrowError(env, "view is destroyed");
      return nullptr;
    }
    std::string data = argc > 0 ? GetString(env, args[0]) : "";
    if (data.empty()) {
      ThrowError(env, "update data must not be empty");
      return nullptr;
    }
    std::string global_props = argc > 1 ? GetString(env, args[1]) : "";
    std::string processor = argc > 2 ? GetString(env, args[2]) : "";
    bool read_only = argc > 3 ? GetBool(env, args[3]) : false;
    auto update_data = MakeTemplateData(data, processor, read_only);
    if (!global_props.empty()) {
      obj->global_props_ = MakeTemplateData(global_props);
    }
    obj->TemplateRenderer()->UpdateMetaData(
        update_data->Impl()->template_data,
        obj->global_props_
            ? obj->global_props_->Impl()->template_data->GetValue()
            : lepus::Value());
    return GetUndefined(env);
  }

  static napi_value UpdateGlobalProps(napi_env env, napi_callback_info info) {
    napi_value js_this = nullptr;
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
    auto* obj = Unwrap(env, js_this);
    if (!obj || obj->destroyed_) {
      ThrowError(env, "view is destroyed");
      return nullptr;
    }
    std::string global_props = argc > 0 ? GetString(env, args[0]) : "";
    if (global_props.empty()) {
      ThrowError(env, "global props must not be empty");
      return nullptr;
    }
    obj->global_props_ = MakeTemplateData(global_props);
    obj->TemplateRenderer()->UpdateGlobalProps(
        obj->global_props_->Impl()->template_data->GetValue());
    return GetUndefined(env);
  }

  static napi_value InvokeCDPFromSDK(napi_env env, napi_callback_info info) {
    napi_value js_this = nullptr;
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
    auto* obj = Unwrap(env, js_this);
    napi_value promise = nullptr;
    napi_deferred deferred = nullptr;
    napi_create_promise(env, &deferred, &promise);
    if (!obj || obj->destroyed_) {
      napi_reject_deferred(env, deferred,
                           CreateError(env, "view is destroyed"));
      return promise;
    }
#if ENABLE_INSPECTOR
    std::string cdp_msg = argc > 0 ? GetString(env, args[0]) : "";
    if (!GetDispatcher(obj->state_)) {
      napi_reject_deferred(env, deferred,
                           CreateError(env, "JS dispatcher is unavailable"));
      return promise;
    }
    {
      std::lock_guard<std::mutex> lock(obj->state_->mutex);
      obj->state_->cdp_waiters.push_back(deferred);
    }
    std::weak_ptr<ViewState> weak_state = obj->state_;
    obj->TemplateRenderer()->InvokeCDPFromSDK(
        cdp_msg, [weak_state, deferred](const std::string& response) {
          auto state = weak_state.lock();
          if (!state) {
            return;
          }
          JsDispatcher* dispatcher = nullptr;
          {
            std::lock_guard<std::mutex> lock(state->mutex);
            if (state->destroyed) {
              return;
            }
            dispatcher = state->dispatcher;
          }
          if (!dispatcher) {
            return;
          }
          dispatcher->Post([state, deferred, response]() {
            {
              std::lock_guard<std::mutex> lock(state->mutex);
              auto it = std::find(state->cdp_waiters.begin(),
                                  state->cdp_waiters.end(), deferred);
              if (it == state->cdp_waiters.end()) {
                return;
              }
              state->cdp_waiters.erase(it);
            }
            napi_env env = state->env;
            napi_value result = nullptr;
            napi_create_string_utf8(env, response.c_str(), response.size(),
                                    &result);
            napi_resolve_deferred(env, deferred, result);
          });
        });
#else
    napi_reject_deferred(env, deferred,
                         CreateError(env, "inspector is not enabled"));
#endif
    return promise;
  }

  static napi_value SendTouchEvent(napi_env env, napi_callback_info info) {
    napi_value js_this = nullptr;
    size_t argc = 2;
    napi_value args[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
    auto* obj = Unwrap(env, js_this);
    if (!obj || obj->destroyed_) {
      ThrowError(env, "view is destroyed");
      return nullptr;
    }
    std::string name = argc > 0 ? GetString(env, args[0]) : "";
    double tag = 0;
    if (argc > 1) {
      GetDouble(env, args[1], &tag);
    }
    obj->view_->SendTouchEvent(name, static_cast<int32_t>(tag), 0, 0, 0, 0, 0,
                               0);
    return GetUndefined(env);
  }

  static napi_value SetErrorHandler(napi_env env, napi_callback_info info) {
    napi_value js_this = nullptr;
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
    auto* obj = Unwrap(env, js_this);
    if (!obj || obj->destroyed_) {
      ThrowError(env, "view is destroyed");
      return nullptr;
    }
    if (obj->state_->error_handler_ref) {
      napi_delete_reference(env, obj->state_->error_handler_ref);
      obj->state_->error_handler_ref = nullptr;
    }
    if (argc > 0 && IsFunction(env, args[0])) {
      napi_create_reference(env, args[0], 1, &obj->state_->error_handler_ref);
    }
    return GetUndefined(env);
  }

  static napi_value WaitForFrame(napi_env env, napi_callback_info info) {
    napi_value js_this = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
    auto* obj = Unwrap(env, js_this);
    napi_value promise = nullptr;
    napi_deferred deferred = nullptr;
    napi_create_promise(env, &deferred, &promise);
    if (!obj || obj->destroyed_) {
      napi_reject_deferred(env, deferred,
                           CreateError(env, "view is destroyed"));
      return promise;
    }

    bool has_frame = false;
    {
      std::lock_guard<std::mutex> lock(obj->state_->mutex);
      has_frame = obj->state_->has_frame;
      if (!has_frame) {
        obj->state_->frame_waiters.push_back(deferred);
      }
    }
    if (has_frame) {
      napi_resolve_deferred(env, deferred, GetUndefined(env));
    }
    return promise;
  }

  static napi_value ForceFrame(napi_env env, napi_callback_info info) {
    napi_value js_this = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
    auto* obj = Unwrap(env, js_this);
    if (!obj || obj->destroyed_) {
      ThrowError(env, "view is destroyed");
      return nullptr;
    }
    obj->ForceRenderFrame();
    return GetUndefined(env);
  }

  static napi_value CaptureFrame(napi_env env, napi_callback_info info) {
    napi_value js_this = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
    auto* obj = Unwrap(env, js_this);
    if (!obj || obj->destroyed_) {
      ThrowError(env, "view is destroyed");
      return nullptr;
    }

    int width = 0;
    int height = 0;
    std::vector<uint8_t> frame;
    {
      std::lock_guard<std::mutex> lock(obj->state_->mutex);
      if (!obj->state_->has_frame || obj->state_->rgba_frame.empty()) {
        ThrowError(env, "no frame has been rendered");
        return nullptr;
      }
      width = obj->state_->frame_width;
      height = obj->state_->frame_height;
      frame = obj->state_->rgba_frame;
    }

    napi_value result = nullptr;
    napi_create_object(env, &result);
    napi_value width_value = nullptr;
    napi_value height_value = nullptr;
    napi_value data_value = nullptr;
    napi_create_int32(env, width, &width_value);
    napi_create_int32(env, height, &height_value);
    napi_create_buffer_copy(env, frame.size(), frame.data(), nullptr,
                            &data_value);
    napi_set_named_property(env, result, "width", width_value);
    napi_set_named_property(env, result, "height", height_value);
    napi_set_named_property(env, result, "data", data_value);
    return result;
  }

  static napi_value ShowWindow(napi_env env, napi_callback_info info) {
    napi_value js_this = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
    auto* obj = Unwrap(env, js_this);
    if (!obj || obj->destroyed_) {
      ThrowError(env, "view is destroyed");
      return nullptr;
    }
    if (!obj->windowed_ || !obj->window_host_) {
      ThrowError(env, "WindowedLynxView is only supported on macOS");
      return nullptr;
    }
    if (!obj->window_host_->Show()) {
      ThrowError(env, "failed to show WindowedLynxView");
      return nullptr;
    }
    return GetUndefined(env);
  }

  static napi_value CloseWindow(napi_env env, napi_callback_info info) {
    napi_value js_this = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
    auto* obj = Unwrap(env, js_this);
    if (!obj || obj->destroyed_) {
      ThrowError(env, "view is destroyed");
      return nullptr;
    }
    if (obj->window_host_) {
      obj->window_host_->Close();
      obj->NotifyWindowClosedFromHost();
    }
    return GetUndefined(env);
  }

  static napi_value WaitUntilClosed(napi_env env, napi_callback_info info) {
    napi_value js_this = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
    auto* obj = Unwrap(env, js_this);
    napi_value promise = nullptr;
    napi_deferred deferred = nullptr;
    napi_create_promise(env, &deferred, &promise);
    if (!obj || obj->destroyed_) {
      napi_reject_deferred(env, deferred,
                           CreateError(env, "view is destroyed"));
      return promise;
    }
    if (!obj->windowed_ || !obj->window_host_) {
      napi_reject_deferred(
          env, deferred,
          CreateError(env, "WindowedLynxView is only supported on macOS"));
      return promise;
    }

    bool closed = false;
    {
      std::lock_guard<std::mutex> lock(obj->state_->mutex);
      closed = obj->state_->window_closed ||
               (obj->window_host_ && obj->window_host_->IsClosed());
      if (!closed) {
        obj->state_->close_waiters.push_back(deferred);
      }
    }
    if (closed) {
      napi_resolve_deferred(env, deferred, GetUndefined(env));
    }
    return promise;
  }

  static napi_value ClickWindow(napi_env env, napi_callback_info info) {
    napi_value js_this = nullptr;
    size_t argc = 2;
    napi_value args[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
    auto* obj = Unwrap(env, js_this);
    if (!obj || obj->destroyed_) {
      ThrowError(env, "view is destroyed");
      return nullptr;
    }
    if (!obj->windowed_ || !obj->window_host_) {
      ThrowError(env, "WindowedLynxView is only supported on macOS");
      return nullptr;
    }
    double x = 0;
    double y = 0;
    if (argc < 2 || !GetDouble(env, args[0], &x) ||
        !GetDouble(env, args[1], &y)) {
      ThrowError(env, "click requires x and y");
      return nullptr;
    }
    obj->window_host_->Click(x, y);
    return GetUndefined(env);
  }

  static napi_value TypeTextWindow(napi_env env, napi_callback_info info) {
    napi_value js_this = nullptr;
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
    auto* obj = Unwrap(env, js_this);
    if (!obj || obj->destroyed_) {
      ThrowError(env, "view is destroyed");
      return nullptr;
    }
    if (!obj->windowed_ || !obj->window_host_) {
      ThrowError(env, "WindowedLynxView is only supported on macOS");
      return nullptr;
    }
    if (argc < 1 || !IsString(env, args[0])) {
      ThrowError(env, "typeText requires text");
      return nullptr;
    }
    std::string text = GetString(env, args[0]);
    obj->window_host_->TypeText(text);
    return GetUndefined(env);
  }

  static napi_value PressKeyWindow(napi_env env, napi_callback_info info) {
    napi_value js_this = nullptr;
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
    auto* obj = Unwrap(env, js_this);
    if (!obj || obj->destroyed_) {
      ThrowError(env, "view is destroyed");
      return nullptr;
    }
    if (!obj->windowed_ || !obj->window_host_) {
      ThrowError(env, "WindowedLynxView is only supported on macOS");
      return nullptr;
    }
    std::string key = argc > 0 ? GetString(env, args[0]) : "";
    if (!IsSupportedWindowedKeyboardKey(key)) {
      ThrowError(env, "pressKey received an unsupported key");
      return nullptr;
    }
    obj->window_host_->PressKey(key);
    return GetUndefined(env);
  }

  static napi_value DestroyMethod(napi_env env, napi_callback_info info) {
    napi_value js_this = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
    auto* obj = Unwrap(env, js_this);
    if (obj) {
      obj->Destroy(DestroyReason::kExplicit);
    }
    return GetUndefined(env);
  }

  void SendPointerEventFromHost(const lynx_pointer_event_t& event) {
    if (!dispatcher_) {
      return;
    }
    dispatcher_->Post([this, event]() mutable {
      if (destroyed_ || !renderer_) {
        return;
      }
      lynx_pointer_event_t event_copy = event;
      renderer_->SendPointerEvent(&event_copy);
    });
  }

  void SendKeyEventFromHost(const WindowedKeyEvent& event) {
    if (!dispatcher_) {
      return;
    }
    dispatcher_->Post([this, event]() mutable {
      if (destroyed_ || !renderer_) {
        return;
      }
      lynx_key_event_t event_copy = event.event;
      event_copy.character =
          event.character.empty() ? nullptr : event.character.c_str();
      renderer_->SendKeyEvent(&event_copy);
    });
  }

  void UpdateWindowMetricsFromHost(double width, double height,
                                   double device_pixel_ratio) {
    if (!dispatcher_) {
      return;
    }
    dispatcher_->Post([this, width, height, device_pixel_ratio]() {
      if (destroyed_ || !view_) {
        return;
      }
      width_ = width;
      height_ = height;
      device_pixel_ratio_ = device_pixel_ratio;
      {
        std::lock_guard<std::mutex> lock(state_->mutex);
        state_->expected_frame_width = ToPixelSize(width_, device_pixel_ratio_);
      }
      view_->UpdateScreenMetrics(static_cast<float>(width_),
                                 static_cast<float>(height_),
                                 static_cast<float>(device_pixel_ratio_));
      view_->SetFrame(0, 0, static_cast<float>(width_),
                      static_cast<float>(height_));
    });
  }

  void NotifyWindowClosedFromHost() {
    if (!state_) {
      return;
    }
    std::vector<napi_deferred> close_waiters;
    JsDispatcher* dispatcher = nullptr;
    {
      std::lock_guard<std::mutex> lock(state_->mutex);
      if (state_->window_closed) {
        return;
      }
      state_->window_closed = true;
      dispatcher = state_->dispatcher;
      close_waiters.swap(state_->close_waiters);
    }
    if (!dispatcher || close_waiters.empty()) {
      return;
    }
    dispatcher->Post(
        [env = env_, close_waiters = std::move(close_waiters)]() mutable {
          ResolveWaiters(env, std::move(close_waiters));
        });
  }

  void BuildView() {
    lynx_windowless_renderer_type_e renderer_type = kRendererTypeSoftware;
    if (windowed_) {
      window_host_ = NodeLynxWindowHost::Create(
          dispatcher_ ? dispatcher_->Loop() : nullptr, window_options_,
          [this](const lynx_pointer_event_t& event) {
            SendPointerEventFromHost(event);
          },
          [this](const WindowedKeyEvent& event) {
            SendKeyEventFromHost(event);
          },
          [this](double width, double height, double device_pixel_ratio) {
            UpdateWindowMetricsFromHost(width, height, device_pixel_ratio);
          },
          [this]() { NotifyWindowClosedFromHost(); });
      // Keep the visible AppKit host backed by Metal, but use the software
      // windowless renderer as the frame source. The current accelerated Metal
      // CAPI path does not consistently call OnAcceleratedPresent, which leaves
      // waitForFrame and screenshot automation without a frame to observe.
      renderer_ = std::make_shared<NodeLynxRenderer>(
          state_, renderer_type,
          [this](const lynx_accelerated_paint_info_t& paint_info) {
            return window_host_ && window_host_->PresentFrame(paint_info);
          },
          [this](const uint8_t* rgba, int width, int height) {
            return window_host_ &&
                   window_host_->PresentPixels(rgba, width, height);
          },
          [this](bool show) {
            if (window_host_) {
              window_host_->SetTextInputActive(show);
            }
          },
          [this](float x, float y, float width, float height) {
            if (window_host_) {
              window_host_->UpdateCaretPosition(x, y, width, height);
            }
          },
          [this](float x, float y, float width, float height) {
            if (window_host_) {
              window_host_->SetMarkedTextRect(x, y, width, height);
            }
          });
    } else {
      renderer_ = std::make_shared<NodeLynxRenderer>(state_, renderer_type);
    }
    state_->renderer = renderer_;
    client_ = std::make_shared<HeadlessClient>(state_);
    fetcher_ =
        std::make_shared<HeadlessResourceFetcher>(state_, resources_path_);

    pub::LynxView::Builder builder;
    builder.SetScreenSize(static_cast<float>(width_),
                          static_cast<float>(height_),
                          static_cast<float>(device_pixel_ratio_));
    builder.SetFrame(0, 0, static_cast<float>(width_),
                     static_cast<float>(height_));
    builder.SetWindowlessRenderer(renderer_);
    builder.SetGenericResourceFetcher(fetcher_);
    view_ = builder.Build();
    if (!view_ || !view_->Impl() || !view_->Impl()->lynx_template_renderer) {
      Destroy(DestroyReason::kFinalizer);
      ThrowError(env_, "failed to create LynxView");
      return;
    }
    ConfigureTextureBackendForRenderer(renderer_type);
    view_->AddClient(client_);
    template_client_ = std::make_unique<HeadlessTemplateClient>(state_);
    TemplateRenderer()->AddClient(template_client_.get());
    view_->OnEnterForeground();
  }

  void ConfigureTextureBackendForRenderer(
      lynx_windowless_renderer_type_e renderer_type) {
    auto* page_view = GetPageView();
    if (!page_view) {
      return;
    }
    // The Skia headless path does not resolve lazy texture-backed images, so
    // software windowless rendering must use raster image resources.
    page_view->SetUseTextureBackend(renderer_type != kRendererTypeSoftware);
  }

  clay::PageView* GetPageView() {
    if (!view_ || !view_->Impl() || !view_->Impl()->lynx_ui_renderer) {
      return nullptr;
    }
    auto* ui_delegate = view_->Impl()->lynx_ui_renderer->GetUIDelegate();
    auto* clay_delegate = static_cast<tasm::UIDelegateClay*>(ui_delegate);
    if (!clay_delegate || !clay_delegate->GetViewContext() ||
        !clay_delegate->GetViewContext()->GetPageView()) {
      return nullptr;
    }
    return clay_delegate->GetViewContext()->GetPageView();
  }

  void ForceRenderFrame() {
    auto* page_view = GetPageView();
    if (page_view) {
      page_view->RequestPaint();
    }
  }

  void Destroy(DestroyReason reason) {
    if (destroyed_) {
      return;
    }
    destroyed_ = true;
    if (reason == DestroyReason::kExplicit) {
      ScheduleExplicitDestroyGracePeriod(env_);
    }
    if (state_) {
      std::vector<napi_deferred> load_waiters;
      std::vector<napi_deferred> frame_waiters;
      std::vector<napi_deferred> cdp_waiters;
      std::vector<napi_deferred> close_waiters;
      {
        std::lock_guard<std::mutex> lock(state_->mutex);
        state_->destroyed = true;
        state_->dispatcher = nullptr;
        load_waiters.swap(state_->load_waiters);
        frame_waiters.swap(state_->frame_waiters);
        cdp_waiters.swap(state_->cdp_waiters);
        close_waiters.swap(state_->close_waiters);
      }
      RejectWaiters(env_, std::move(load_waiters), "view is destroyed");
      RejectWaiters(env_, std::move(frame_waiters), "view is destroyed");
      RejectWaiters(env_, std::move(cdp_waiters), "view is destroyed");
      RejectWaiters(env_, std::move(close_waiters), "view is destroyed");
    }

    if (window_host_) {
      window_host_->Close();
      window_host_.reset();
    }
    if (view_ && template_client_) {
      TemplateRenderer()->RemoveClient(template_client_.get());
    }
    view_.reset();
    client_.reset();
    template_client_.reset();
    fetcher_.reset();
    renderer_.reset();

    if (state_ && state_->resource_fetcher_ref) {
      napi_delete_reference(env_, state_->resource_fetcher_ref);
      state_->resource_fetcher_ref = nullptr;
    }
    if (state_ && state_->error_handler_ref) {
      napi_delete_reference(env_, state_->error_handler_ref);
      state_->error_handler_ref = nullptr;
    }
    if (dispatcher_) {
      dispatcher_->Dispose();
      dispatcher_ = nullptr;
    }
  }

  embedder::LynxTemplateRenderer* TemplateRenderer() {
    return view_->Impl()->lynx_template_renderer.get();
  }

  napi_env env_ = nullptr;
  std::string resources_path_;
  double width_ = 0;
  double height_ = 0;
  double device_pixel_ratio_ = 1;
  bool windowed_ = false;
  WindowedLynxViewOptions window_options_;
  bool destroyed_ = false;
  JsDispatcher* dispatcher_ = nullptr;
  std::shared_ptr<ViewState> state_;
  std::shared_ptr<NodeLynxRenderer> renderer_;
  std::shared_ptr<HeadlessClient> client_;
  std::unique_ptr<HeadlessTemplateClient> template_client_;
  std::shared_ptr<HeadlessResourceFetcher> fetcher_;
  std::unique_ptr<NodeLynxWindowHost> window_host_;
  std::unique_ptr<pub::LynxView> view_;
  std::shared_ptr<pub::LynxTemplateData> global_props_;
};

class LynxEnvNode {
 public:
  static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor properties[] = {
        {"setDevtoolSwitch", nullptr, SetDevtoolSwitch, nullptr, nullptr,
         nullptr, napi_static, nullptr},
        {"setAppInfo", nullptr, SetAppInfo, nullptr, nullptr, nullptr,
         napi_static, nullptr},
        {"connectDevtools", nullptr, ConnectDevtools, nullptr, nullptr, nullptr,
         napi_static, nullptr},
        {"setOpenCardCallback", nullptr, SetOpenCardCallback, nullptr, nullptr,
         nullptr, napi_static, nullptr},
        {"setClosePageCallback", nullptr, SetClosePageCallback, nullptr,
         nullptr, nullptr, napi_static, nullptr},
    };
    napi_value cons = nullptr;
    napi_define_class(env, "LynxEnv", NAPI_AUTO_LENGTH, Constructor, nullptr,
                      sizeof(properties) / sizeof(properties[0]), properties,
                      &cons);
    napi_set_named_property(env, exports, "LynxEnv", cons);
    napi_value init_global_env = nullptr;
    napi_create_function(env, "initGlobalEnv", NAPI_AUTO_LENGTH, InitGlobalEnv,
                         nullptr, &init_global_env);
    napi_set_named_property(env, exports, "initGlobalEnv", init_global_env);
    return exports;
  }

 private:
  static napi_value Constructor(napi_env env, napi_callback_info info) {
    napi_value js_this = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
    return js_this;
  }

  static napi_value InitGlobalEnv(napi_env env, napi_callback_info) {
    FmlMessageLoopPump::Init(env);
    return GetUndefined(env);
  }

  static napi_value SetDevtoolSwitch(napi_env env, napi_callback_info info) {
#if ENABLE_INSPECTOR
    size_t argc = 2;
    napi_value args[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    std::string key = argc > 0 ? GetString(env, args[0]) : "";
    bool value = argc > 1 ? GetBool(env, args[1]) : false;
    embedder::DevToolEnvEmbedder::GetInstance().SetDevToolSwitch(key, value);
#endif
    return GetUndefined(env);
  }

  static napi_value SetAppInfo(napi_env env, napi_callback_info info) {
#if ENABLE_INSPECTOR
    size_t argc = 2;
    napi_value args[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    auto keys =
        argc > 0 ? GetStringArray(env, args[0]) : std::vector<std::string>();
    auto values =
        argc > 1 ? GetStringArray(env, args[1]) : std::vector<std::string>();
    std::unordered_map<std::string, std::string> options;
    for (size_t i = 0; i < keys.size() && i < values.size(); ++i) {
      options[keys[i]] = values[i];
    }
    embedder::DevToolEnvEmbedder::GetInstance().SetAppInfo(options);
    embedder::DevToolEnvEmbedder::GetInstance().EnableDevTool(true);
#endif
    return GetUndefined(env);
  }

  static napi_value ConnectDevtools(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    std::string schema = argc > 0 ? GetString(env, args[0]) : "";
    EnsureDebugRouterAppHandlersRegistered();
    napi_value result = nullptr;
    napi_get_boolean(env, lynx_env_connect_devtool(schema.c_str()) != 0,
                     &result);
    return result;
  }

  static napi_value SetOpenCardCallback(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc == 0 || IsNullOrUndefined(env, args[0])) {
      auto& state = GetDebugRouterCallbackState();
      napi_ref old_ref = nullptr;
      {
        std::lock_guard<std::mutex> lock(state.mutex);
        old_ref = state.open_card_ref;
        state.open_card_ref = nullptr;
      }
      lynx_env_set_open_card_callback(nullptr, nullptr);
      if (old_ref) {
        napi_delete_reference(env, old_ref);
      }
      return GetUndefined(env);
    }
    if (!IsFunction(env, args[0])) {
      ThrowError(env, "setOpenCardCallback expects a function");
      return GetUndefined(env);
    }
    if (!EnsureDebugRouterCallbackRuntime(env)) {
      ThrowError(env, "failed to initialize debug-router callback runtime");
      return GetUndefined(env);
    }
    EnsureDebugRouterAppHandlersRegistered();
    napi_ref new_ref = nullptr;
    if (napi_create_reference(env, args[0], 1, &new_ref) != napi_ok) {
      ThrowError(env, "failed to create open-card callback reference");
      return GetUndefined(env);
    }
    auto& state = GetDebugRouterCallbackState();
    napi_ref old_ref = nullptr;
    {
      std::lock_guard<std::mutex> lock(state.mutex);
      old_ref = state.open_card_ref;
      state.open_card_ref = new_ref;
    }
    lynx_env_set_open_card_callback(HandleDebugRouterOpenCard, nullptr);
    if (old_ref) {
      napi_delete_reference(env, old_ref);
    }
    return GetUndefined(env);
  }

  static napi_value SetClosePageCallback(napi_env env,
                                         napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc == 0 || IsNullOrUndefined(env, args[0])) {
      auto& state = GetDebugRouterCallbackState();
      napi_ref old_ref = nullptr;
      {
        std::lock_guard<std::mutex> lock(state.mutex);
        old_ref = state.close_page_ref;
        state.close_page_ref = nullptr;
      }
      if (old_ref) {
        napi_delete_reference(env, old_ref);
      }
      return GetUndefined(env);
    }
    if (!IsFunction(env, args[0])) {
      ThrowError(env, "setClosePageCallback expects a function");
      return GetUndefined(env);
    }
    if (!EnsureDebugRouterCallbackRuntime(env)) {
      ThrowError(env, "failed to initialize debug-router callback runtime");
      return GetUndefined(env);
    }
    EnsureDebugRouterAppHandlersRegistered();
    napi_ref new_ref = nullptr;
    if (napi_create_reference(env, args[0], 1, &new_ref) != napi_ok) {
      ThrowError(env, "failed to create close-page callback reference");
      return GetUndefined(env);
    }
    auto& state = GetDebugRouterCallbackState();
    napi_ref old_ref = nullptr;
    {
      std::lock_guard<std::mutex> lock(state.mutex);
      old_ref = state.close_page_ref;
      state.close_page_ref = new_ref;
    }
    if (old_ref) {
      napi_delete_reference(env, old_ref);
    }
    return GetUndefined(env);
  }
};

EXTERN_C_START static napi_value Init(napi_env env, napi_value exports) {
  HeadlessLynxViewNode::Init(env, exports);
  LynxEnvNode::Init(env, exports);
  return exports;
}
EXTERN_C_END

static napi_module node_lynx_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "node_lynx",
    .nm_priv = nullptr,
    .reserved = {nullptr},
};

extern "C" __attribute__((constructor)) void RegisterNodeLynxModule(void) {
  napi_module_register(&node_lynx_module);
}

}  // namespace
}  // namespace node
}  // namespace lynx
