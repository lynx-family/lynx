// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"

#include "base/trace/native/trace_event.h"
#include "core/runtime/profile/runtime_profiler_manager.h"
#include "core/services/recorder/recorder_controller.h"
#include "core/services/replay/replay_controller.h"
#include "devtool/lynx_devtool/agent/global_devtool_platform_facade.h"
#include "devtool/lynx_devtool/base/file_stream.h"
#include "third_party/modp_b64/modp_b64.h"

namespace lynx {
namespace devtool {

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
constexpr int kDefaultBufferSize = 20 * 1024;  // 20M
LynxGlobalDevToolMediator::LynxGlobalDevToolMediator()
    : tracing_session_id_(-1) {
#else
LynxGlobalDevToolMediator::LynxGlobalDevToolMediator() {
#endif
  ui_task_runner_ = base::UIThread::GetRunner();
}

LynxGlobalDevToolMediator& LynxGlobalDevToolMediator::GetInstance() {
  static lynx::base::NoDestructor<LynxGlobalDevToolMediator> instance;
  return *instance;
}

void LynxGlobalDevToolMediator::RecordingStart(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  LOGI("start recording");
  int64_t id = message["id"].asInt64();
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_, [] {
      lynx::tasm::recorder::RecorderController::StartRecord();
    });
  } else {
    sender->SendErrorResponse(id, "Cannot find ui task runner");
    return;
  }

  Json::Value res;
  res["result"] = Json::Value(Json::ValueType::objectValue);
  res["id"] = id;
  sender->SendMessage("CDP", res);
}

void LynxGlobalDevToolMediator::RecordingEnd(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  LOGI("End recording");
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_, [sender, message] {
      lynx::base::MoveOnlyClosure<void, std::vector<std::string>&,
                                  std::vector<int64_t>&>
          send_complete([sender](std::vector<std::string>& files,
                                 std::vector<int64_t>& sessions) {
            Json::Value msg;
            msg["method"] = "Recording.recordingComplete";
            Json::Value handlers, filenames, session_ids;
            for (auto i : sessions) {
              session_ids.append(i);
            }
            for (auto item : files) {
              int stream_handle = FileStream::Open(item);
              handlers.append(stream_handle);
              filenames.append(item);
            }
            msg["params"]["stream"] = handlers;
            msg["params"]["filenames"] = filenames;
            msg["params"]["sessionIDs"] = session_ids;
            msg["params"]["recordFormat"] = "json";
            sender->SendMessage("CDP", msg);
          });
      lynx::tasm::recorder::RecorderController::EndRecord(
          std::move(send_complete));
      sender->SendOKResponse(message["id"].asInt64());
    });
  }
}

void LynxGlobalDevToolMediator::ReplayStart(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  LOGI("start replay test");
  int64_t id = message["id"].asInt64();
  if (ui_task_runner_) {
    RunOnTaskRunner(ui_task_runner_,
                    [] { lynx::tasm::replay::ReplayController::StartTest(); });
  } else {
    sender->SendErrorResponse(id, "Cannot find ui task runner");
    return;
  }

  Json::Value res;
  res["result"] = Json::Value(Json::ValueType::objectValue);
  res["id"] = id;
  sender->SendMessage("CDP", res);
}

void LynxGlobalDevToolMediator::ReplayEnd(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (default_task_runner_) {
    RunOnTaskRunner(default_task_runner_, [sender, message] {
      int64_t id = message["id"].asInt64();
      if (lynx::tasm::replay::ReplayController::Enable()) {
        LOGI("send replay end");
        Json::Value content(Json::ValueType::objectValue);
        content["method"] = "Replay.end";
        std::string file_path = message["params"].asString();
        int stream_handle = FileStream::Open(file_path);
        if (stream_handle == -1) {
          sender->SendErrorResponse(id, "file path doesn't exist");
          return;
        }
        content["params"]["stream"] = std::to_string(stream_handle);
        sender->SendMessage("CDP", content);
      } else {
        sender->SendErrorResponse(id, "Replay doesn't enable");
      }
    });
  }
}

void LynxGlobalDevToolMediator::EndReplayTest(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const std::string& file_path) {
  if (default_task_runner_) {
    RunOnTaskRunner(default_task_runner_, [sender, file_path] {
      Json::Value msg(Json::ValueType::objectValue);
      msg["method"] = "Replay.end";
      msg["params"] = file_path;
      GetInstance().ReplayEnd(sender, msg);
    });
  }
}

void LynxGlobalDevToolMediator::IORead(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  std::string handle_str = message["params"]["handle"].asString();
  if (!std::isdigit(handle_str[0])) {
    int id = static_cast<int>(message["id"].asInt64());
    sender->SendErrorResponse(id, "Get invalid stream handle");
    return;
  }

  Json::Value res;
  res["id"] = message["id"].asInt64();
  res["result"]["base64Encoded"] = true;
  int size = static_cast<int>(message["params"]["size"].asInt64());
  if (size > 0) {
    if (default_task_runner_) {
      RunOnTaskRunner(default_task_runner_, [handle_str, size, sender, res] {
        std::unique_ptr<char[]> buff = std::make_unique<char[]>(size);
        int total_read = FileStream::Read(std::stoi(handle_str),
                                          static_cast<char*>(buff.get()), size);
        if (total_read > 0) {
          int encode_length = modp_b64_encode_len(total_read);
          std::unique_ptr<char[]> encode_buff =
              std::make_unique<char[]>(encode_length);
          modp_b64_encode(encode_buff.get(), buff.get(), total_read);
          Json::Value result = res;
          result["result"]["data"] =
              std::string(encode_buff.get(), encode_length - 1);
          if (total_read == size) {
            result["result"]["eof"] = false;
          } else {
            result["result"]["eof"] = true;
          }
          sender->SendMessage("CDP", result);
        } else {
          Json::Value result = res;
          result["result"]["eof"] = true;
          sender->SendMessage("CDP", result);
        }
      });
    } else {
      sender->SendErrorResponse(res["id"].asInt(),
                                "Cannot find default task runner");
      return;
    }
  } else {
    res["result"]["eof"] = true;
    sender->SendMessage("CDP", res);
  }
}

void LynxGlobalDevToolMediator::IOClose(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  std::string handle_str = message["params"]["handle"].asString();
  if (!std::isdigit(handle_str[0])) {
    int id = static_cast<int>(message["id"].asInt64());
    sender->SendErrorResponse(id, "Get invalid stream handle");
    return;
  }
  Json::Value res;
  res["id"] = message["id"].asInt64();
  if (default_task_runner_) {
    RunOnTaskRunner(default_task_runner_, [handle_str, sender, res] {
      FileStream::Close(std::stoi(handle_str));
      sender->SendMessage("CDP", res);
    });
  } else {
    sender->SendErrorResponse(res["id"].asInt(),
                              "Cannot find default task runner");
    return;
  }
}

void LynxGlobalDevToolMediator::MemoryStartTracing(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (default_task_runner_) {
    RunOnTaskRunner(default_task_runner_, [sender, message]() {
      int id = static_cast<int>(message["id"].asInt64());
      GlobalDevToolPlatformFacade::GetInstance().StartMemoryTracing();
      Json::Value response(Json::ValueType::objectValue);
      response["result"] = Json::Value(Json::ValueType::objectValue);
      response["id"] = id;
      sender->SendMessage("CDP", response);
    });
  }
}

void LynxGlobalDevToolMediator::MemoryStopTracing(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (default_task_runner_) {
    RunOnTaskRunner(default_task_runner_, [sender, message]() {
      int id = static_cast<int>(message["id"].asInt64());
      GlobalDevToolPlatformFacade::GetInstance().StopMemoryTracing();
      Json::Value response(Json::ValueType::objectValue);
      response["result"] = Json::Value(Json::ValueType::objectValue);
      response["id"] = id;
      sender->SendMessage("CDP", response);
    });
  }
}

void LynxGlobalDevToolMediator::SystemInfoGetInfo(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  content["modelName"] =
      GlobalDevToolPlatformFacade::GetInstance().GetSystemModelName();
#if defined(OS_ANDROID)
  content["platform"] = "Android";
#else
  content["platform"] = "iOS";
#endif
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void LynxGlobalDevToolMediator::TracingStart(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (default_task_runner_) {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
    RunOnTaskRunner(default_task_runner_, [this, sender, message]() {
#else
    RunOnTaskRunner(default_task_runner_, [sender, message]() {
#endif
      int id = static_cast<int>(message["id"].asInt64());
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
      LOGI("start tracing");
      if (this->tracing_session_id_ > 0) {
        sender->SendErrorResponse(id, "Tracing already started");
        return;
      }

      auto config = std::make_shared<lynx::trace::TraceConfig>();
      const auto& params = message["params"];
      if (params.isMember("traceConfig")) {
        const auto& trace_config = params["traceConfig"];
        const auto to_vector = [](const Json::Value& array,
                                  std::vector<std::string>& result) {
          for (const auto& e : array) {
            result.push_back(e.asString());
          }
          return result;
        };
        to_vector(trace_config["includedCategories"],
                  config->included_categories);
        to_vector(trace_config["excludedCategories"],
                  config->excluded_categories);
        config->enable_systrace = trace_config["enableSystrace"].asBool();
        config->buffer_size = trace_config.isMember("bufferSize")
                                  ? trace_config["bufferSize"].asInt()
                                  : kDefaultBufferSize;

        if (trace_config.isMember("recordMod")) {
          const auto& record_mod = trace_config["recordMod"];
          if (record_mod == "recordContinuously") {
            config->record_mode = lynx::trace::TraceConfig::RECORD_CONTINUOUSLY;
          }
        }
        config->js_profile_interval =
            trace_config.isMember("JSProfileInterval")
                ? trace_config["JSProfileInterval"].asInt()
                : -1;
        if (config->js_profile_interval > 0) {
          // JSProfileType has 3 options:
          // 1. disable: don't allow js profile
          // 2. quickjs: if JSProfileInterval is greater than 0, allow quickjs
          // and lepusng profile
          // 3. v8: if JSProfileInterval is greater than 0, allow v8 profile
          auto js_profile_type = trace_config.isMember("JSProfileType")
                                     ? trace_config["JSProfileType"].asString()
                                     : "quickjs";
          if (js_profile_type == "quickjs") {
            config->js_profile_type = trace::RuntimeProfilerType::quickjs;
          } else if (js_profile_type == "v8") {
            config->js_profile_type = trace::RuntimeProfilerType::v8;
          }
        }
      } else {
        config->excluded_categories = {"*"};
      }

      auto controller =
          GlobalDevToolPlatformFacade::GetInstance().GetTraceController();
      if (controller == nullptr) {
        sender->SendErrorResponse(id, "Failed to get trace controller");
        return;
      }

      controller->AddTracePlugin(
          GlobalDevToolPlatformFacade::GetInstance().GetFPSTracePlugin());
      controller->AddTracePlugin(
          GlobalDevToolPlatformFacade::GetInstance().GetInstanceTracePlugin());

      if (config->js_profile_interval > 0) {
        controller->AddTracePlugin(lynx::profile::GetRuntimeProfilerManager());
      }
      if (std::find(config->included_categories.begin(),
                    config->included_categories.end(),
                    LYNX_TRACE_CATEGORY_SCREENSHOTS) !=
          config->included_categories.end()) {
        controller->AddTracePlugin(GlobalDevToolPlatformFacade::GetInstance()
                                       .GetFrameViewTracePlugin());
      }
      this->tracing_session_id_ = controller->StartTracing(config);
      if (this->tracing_session_id_ > 0) {
        controller->AddCompleteCallback(
            this->tracing_session_id_, [config, sender]() {
              Json::Value msg;
              msg["method"] = "Tracing.tracingComplete";
              int stream_handle = FileStream::Open(config->file_path);
              msg["params"]["dataLossOccurred"] = (stream_handle <= 0);
              msg["params"]["stream"] = std::to_string(stream_handle);
              msg["params"]["traceFormat"] = "proto";
              msg["params"]["streamCompression"] = "none";
              sender->SendMessage("CDP", msg);
            });
        TRACE_EVENT_INSTANT(
            "vitals", "LynxEngineVersion", "version",
            GlobalDevToolPlatformFacade::GetInstance().GetLynxVersion());
        Json::Value res;
        res["result"] = Json::Value(Json::ValueType::objectValue);
        res["id"] = id;

        sender->SendMessage("CDP", res);
      } else {
        sender->SendErrorResponse(id, "Failed to start tracing");
      }
#else
      sender->SendErrorResponse(id, "Tracing not enabled");
#endif
    });
  }
}

void LynxGlobalDevToolMediator::TracingEnd(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (default_task_runner_) {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
    RunOnTaskRunner(default_task_runner_, [this, sender, message]() {
#else
    RunOnTaskRunner(default_task_runner_, [sender, message]() {
#endif
      int id = static_cast<int>(message["id"].asInt64());
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
      LOGI("End tracing");
      if (this->tracing_session_id_ <= 0) {
        sender->SendErrorResponse(id, "Tracing is not started");
        return;
      }

      auto controller =
          GlobalDevToolPlatformFacade::GetInstance().GetTraceController();
      if (controller == nullptr) {
        sender->SendErrorResponse(id, "Failed to get trace controller");
        return;
      }
      sender->SendOKResponse(static_cast<int>(message["id"].asInt64()));

      controller->StopTracing(this->tracing_session_id_);
      //  controller->RemoveCompleteCallbacks(this->tracing_session_id_);
      this->tracing_session_id_ = -1;
#else
      sender->SendErrorResponse(id, "Tracing not enabled");
#endif
    });
  }
}

void LynxGlobalDevToolMediator::SetStartupTracingConfig(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (default_task_runner_) {
    RunOnTaskRunner(default_task_runner_, [sender, message]() {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
      const int id = static_cast<int>(message["id"].asInt());
      auto controller =
          GlobalDevToolPlatformFacade::GetInstance().GetTraceController();
      if (controller == nullptr) {
        sender->SendErrorResponse(id, "Failed to get trace controller");
        return;
      }
      const auto& params = message["params"];
      if (params.isMember("config")) {
        const auto& config = params["config"];
        controller->SetStartupTracingConfig(config.asString());
        sender->SendOKResponse(id);
      } else {
        sender->SendErrorResponse(id, "Set Startup Tracing config is null");
      }
#endif
    });
  }
}

void LynxGlobalDevToolMediator::GetStartupTracingConfig(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (default_task_runner_) {
    RunOnTaskRunner(default_task_runner_, [sender, message]() {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
      const int id = static_cast<int>(message["id"].asInt());
      auto controller =
          GlobalDevToolPlatformFacade::GetInstance().GetTraceController();
      if (controller == nullptr) {
        sender->SendErrorResponse(id, "Failed to get trace controller");
        return;
      }
      const auto config = controller->GetStartupTracingConfig();
      Json::Value response(Json::ValueType::objectValue);
      Json::Value result(Json::ValueType::objectValue);
      result["config"] = config;
      response["result"] = result;
      response["id"] = message["id"].asInt64();
      sender->SendMessage("CDP", response);
#endif
    });
  }
}

void LynxGlobalDevToolMediator::GetStartupTracingFile(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  if (default_task_runner_) {
    RunOnTaskRunner(default_task_runner_, [sender, message]() {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
      const int id = static_cast<int>(message["id"].asInt());
      auto controller =
          GlobalDevToolPlatformFacade::GetInstance().GetTraceController();
      if (controller == nullptr) {
        sender->SendErrorResponse(id, "Failed to get trace controller");
        return;
      }

      const std::string file_path = controller->GetStartupTracingFilePath();
      if (file_path != "") {
        sender->SendOKResponse(id);
        Json::Value msg;
        msg["method"] = "Tracing.tracingComplete";
        int stream_handle = FileStream::Open(file_path);
        msg["params"]["dataLossOccurred"] = (stream_handle <= 0);
        msg["params"]["stream"] = std::to_string(stream_handle);
        msg["params"]["isStartupTracing"] = true;
        sender->SendMessage("CDP", msg);
      } else {
        bool isTracingStarted = controller->IsTracingStarted();
        if (isTracingStarted) {
          sender->SendErrorResponse(id, "Startup Tracing is running");
        } else {
          sender->SendErrorResponse(id, "Failed to get startup tracing file");
        }
      }
#endif
    });
  }
}

}  // namespace devtool
}  // namespace lynx
