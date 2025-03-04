// Copyright (C) 2017 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/trace/native/trace_controller_impl.h"

#include <fcntl.h>

#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <utility>

#include "base/include/log/logging.h"
#include "base/include/thread/timed_task.h"
#include "base/trace/native/trace_event_utils_perfetto.h"
#include "base/trace/native/track_event_wrapper.h"
#include "third_party/rapidjson/document.h"

PERFETTO_DEFINE_CATEGORIES();
PERFETTO_TRACK_EVENT_STATIC_STORAGE();

using TrackEventMessage = ::perfetto::protos::pbzero::TrackEvent;
using TrackEvent = ::perfetto::TrackEvent;
using TrackEventInternal = ::perfetto::internal::TrackEventInternal;
using TrackEvent_Type = ::perfetto::protos::pbzero::TrackEvent_Type;

namespace lynx {

namespace trace {

// Implementations of the definition of the
// "base/trace/native/trace_event_utils_perfetto.h"
constexpr ::perfetto::CounterTrack ConvertToPerfCounterTrack(
    const lynx::perfetto::CounterTrack& counter_tarck) {
  if (counter_tarck.is_global_) {
    if (counter_tarck.unit_name_ != nullptr) {
      return ::perfetto::CounterTrack::Global(counter_tarck.name_,
                                              counter_tarck.unit_name_);
    } else {
      return ::perfetto::CounterTrack::Global(
          counter_tarck.name_,
          static_cast<::perfetto::CounterTrack::Unit>(counter_tarck.unit_));
    }
  }
  return ::perfetto::CounterTrack(
      counter_tarck.name_, counter_tarck.category_,
      static_cast<::perfetto::CounterTrack::Unit>(counter_tarck.unit_),
      counter_tarck.unit_name_, counter_tarck.unit_multiplier_,
      counter_tarck.is_incremental_);
}

uint64_t GetFlowId() {
  static std::atomic<uint64_t> sTraceEventFlowId = 0;
  return sTraceEventFlowId++;
}

void TraceEventImplementation(const char* category_name,
                              const std::string& name, TraceEventType phase,
                              const lynx::perfetto::Track* track_id,
                              const uint64_t& timestamp,
                              const FuncType& callback) {
  TraceEventImplementation(category_name, nullptr, phase, track_id, timestamp,
                           [&name, callback = std::move(callback)](
                               lynx::perfetto::EventContext ctx) {
                             ctx.event()->set_name(name);
                             if (callback) {
                               callback(ctx);
                             }
                           });
}

void TraceEventImplementation(const char* category_name, const char* name,
                              TraceEventType phase,
                              const lynx::perfetto::Track* track_id,
                              const uint64_t& timestamp,
                              const FuncType& callback) {
  TrackEvent::Trace([&](TrackEvent::TraceContext ctx) {
    if (!TrackEvent::IsDynamicCategoryEnabled(
            &ctx, ::perfetto::DynamicCategory{category_name})) {
      return;
    }

    ::perfetto::TraceTimestamp trace_timestamp = ::perfetto::
        TraceTimestampTraits<uint64_t>::ConvertTimestampToTraceTimeNs(
            timestamp ?: TrackEventInternal::GetTimeNs());
    // TODO(yongjie): enable DCHECK later.
    // DCHECK(trace_timestamp.clock_id == TrackEventInternal::GetClockId());

    // Make sure incremental state is valid.
    ::perfetto::TraceWriterBase* trace_writer =
        ctx.tls_inst()->trace_writer.get();
    ::perfetto::internal::TrackEventIncrementalState* incr_state =
        ctx.GetIncrementalState();
    if (incr_state->was_cleared) {
      incr_state->was_cleared = false;
      TrackEventInternal::ResetIncrementalState(trace_writer,
                                                trace_timestamp.nanoseconds);
    }

    // Write the track descriptor before any event on the track.
    TrackEventInternal::WriteTrackDescriptorIfNeeded(
        track_id == nullptr ? TrackEventInternal::kDefaultTrack
                            : ::perfetto::Track(track_id->id()),
        trace_writer, incr_state);

    // Write process track descriptor
    TrackEventInternal::WriteTrackDescriptorIfNeeded(
        ::perfetto::ProcessTrack::Current(), trace_writer, incr_state);

    // Write the event itself.
    {
      auto event_ctx = TrackEventInternal::WriteEvent(
          trace_writer, incr_state, nullptr, name,
          static_cast<TrackEvent_Type>(phase), trace_timestamp.nanoseconds);
      event_ctx.event()->add_categories(category_name, strlen(category_name));
      if (track_id != nullptr) {
        event_ctx.event()->set_track_uuid(
            ::perfetto::Track(track_id->id()).uuid);
      }
      if (callback) {
        lynx::perfetto::TrackEvent event(&event_ctx);
        lynx::perfetto::EventContext out_ctx(&event);
        callback(std::move(out_ctx));
      }
    }  // event_ctx
  });
}

void TraceEventImplementation(const char* category_name,
                              const lynx::perfetto::CounterTrack& counter_track,
                              TraceEventType phase, const uint64_t& timestamp,
                              const uint64_t& counter) {
  auto track = ConvertToPerfCounterTrack(counter_track);

  TrackEvent::Trace([&](TrackEvent::TraceContext ctx) {
    if (!TrackEvent::IsDynamicCategoryEnabled(
            &ctx, ::perfetto::DynamicCategory{category_name})) {
      return;
    }
    ::perfetto::TraceTimestamp trace_timestamp = ::perfetto::
        TraceTimestampTraits<uint64_t>::ConvertTimestampToTraceTimeNs(
            timestamp ?: TrackEventInternal::GetTimeNs());
    // DCHECK(trace_timestamp.clock_id == TrackEventInternal::GetClockId());

    // Make sure incremental state is valid.
    ::perfetto::TraceWriterBase* trace_writer =
        ctx.tls_inst()->trace_writer.get();
    ::perfetto::internal::TrackEventIncrementalState* incr_state =
        ctx.GetIncrementalState();
    if (incr_state->was_cleared) {
      incr_state->was_cleared = false;
      TrackEventInternal::ResetIncrementalState(trace_writer,
                                                trace_timestamp.nanoseconds);
    }

    // Write the track descriptor before any event on the track.
    TrackEventInternal::WriteTrackDescriptorIfNeeded(track, trace_writer,
                                                     incr_state);

    // Write process track descriptor
    TrackEventInternal::WriteTrackDescriptorIfNeeded(
        ::perfetto::ProcessTrack::Current(), trace_writer, incr_state);

    // Write the event itself.
    {
      auto event_ctx = TrackEventInternal::WriteEvent(
          trace_writer, incr_state, nullptr, nullptr,
          static_cast<TrackEvent_Type>(phase), trace_timestamp.nanoseconds);

      event_ctx.event()->set_track_uuid(track.uuid);
      event_ctx.event()->set_double_counter_value(counter);
    }  // event_ctx
  });
}

bool TraceEventCategoryEnabled(const char* category) {
  return TrackEvent::IsDynamicCategoryEnabled(
      ::perfetto::DynamicCategory(category));
}

void TraceRuntimeProfile(const std::string& runtime_profile,
                         const uint64_t track_id, const int32_t profile_id) {
  static uint64_t size = 100 * 1024;  // 100kb
  TrackEvent::Trace([&](TrackEvent::TraceContext ctx) {
    uint64_t count = static_cast<uint64_t>(runtime_profile.size()) / size + 1;
    ctx.Flush();
    for (uint64_t j = 0; j < count; j++) {
      auto packet = ctx.NewTracePacket();
      auto profile_packet = packet->set_js_profile_packet();
      profile_packet->set_track_id(track_id);
      profile_packet->set_profile_id(profile_id);
      bool is_done = false;
      uint64_t length = size;
      if (j == count - 1) {
        is_done = true;
        length = runtime_profile.size() - size * j;
      }
      profile_packet->set_runtime_profile(runtime_profile.data() + size * j,
                                          length);
      profile_packet->set_is_done(is_done);
      packet->Finalize();
      ctx.Flush();
    }
  });
}

// Implementations of the definition of the
// "base/trace/native/trace_controller_impl.h"

TraceController* TraceController::Instance() {
  static TraceControllerImpl instance_;
  return &instance_;
}

TraceControllerImpl::TraceControllerImpl() : TraceController() {
  hook_systrace_ = std::make_unique<HookSystemTrace>();

  ::perfetto::TracingInitArgs args;
  // #if OS_ANDROID
  //   // only android support system backend
  //   args.backends |= ::perfetto::kSystemBackend;
  // #endif
  args.backends |= ::perfetto::kInProcessBackend;
  args.shmem_size_hint_kb = 1024;
  ::perfetto::Tracing::Initialize(args);
  TrackEvent::Register();
}

int TraceControllerImpl::StartTracing(
    const std::shared_ptr<TraceConfig>& config) {
  auto& session = CreateNewSession(config);
  session.config = config;

  // handle categories set
  ::perfetto::protos::gen::TrackEventConfig track_event_cfg;
  auto* enabled_categories = track_event_cfg.mutable_enabled_categories();
  auto* disabled_categories = track_event_cfg.mutable_disabled_categories();
  enabled_categories->insert(enabled_categories->begin(),
                             config->included_categories.begin(),
                             config->included_categories.end());
  disabled_categories->insert(disabled_categories->begin(),
                              config->excluded_categories.begin(),
                              config->excluded_categories.end());
  if (std::find(enabled_categories->begin(), enabled_categories->end(),
                INTERNAL_TRACE_CATEGORY_SCREENSHOTS) !=
      enabled_categories->end()) {
    track_event_cfg.add_enabled_tags("Screenshot");
  }

  // perfetto trace config
  ::perfetto::TraceConfig cfg;
  auto* ds_cfg = cfg.add_data_sources()->mutable_config();
  ds_cfg->set_name("track_event");
  ds_cfg->set_track_event_config_raw(track_event_cfg.SerializeAsString());
  // DCHECK(config->buffer_size > 0);
  cfg.set_flush_period_ms(1000);
  cfg.add_buffers()->set_size_kb(config->buffer_size);

  // file path
  if (config->file_path.empty() && delegate_) {
    if (trace_file_dir_.empty()) {
      trace_file_dir_ = delegate_->GenerateTracingFileDir();
    }
    config->file_path = GenerateTraceFilePath(trace_file_dir_);
  }

  // setup and start session
  if (config->record_mode == TraceConfig::RECORD_CONTINUOUSLY) {
    // write trace events from buffer to file every 3 seconds.
    // DCHECK(!config->file_path.empty());
    cfg.set_file_write_period_ms(3 * 1000);
    int fd = open(config->file_path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0600);
    session.opened_fds.push_back(fd);
    session.session_impl->Setup(cfg, fd);
  } else {
    session.session_impl->Setup(cfg);
  }

  for (auto& trace_plugin_pair : trace_plugins_) {
    if (trace_plugin_pair.second) {
      trace_plugin_pair.second->DispatchSetup(config);
    }
  }
  session.session_impl->StartBlocking();

  // plugin
  for (auto& trace_plugin_pair : trace_plugins_) {
    if (trace_plugin_pair.second) {
      trace_plugin_pair.second->DispatchBegin();
    }
  }
  if (config->enable_systrace) {
    hook_systrace_->Install();
  }

  // status
  session.started = true;
  is_tracing_started_ = true;
  LOGI("Tracing started, session id: " << session.id << " buffer size: "
                                       << config->buffer_size);

#ifdef OS_ANDROID
  if (delegate_) {
    delegate_->RefreshATraceTags();
  }
#endif
  return session.id;
}

bool TraceControllerImpl::StopTracing(int session_id) {
  // find session
  auto session_pair = tracing_sessions_.find(session_id);
  if (session_pair == tracing_sessions_.end()) {
    LOGE("Tracing session not found: " << session_id);
    return false;
  }

  // clean plugin
  for (auto& trace_plugin_pair : trace_plugins_) {
    if (trace_plugin_pair.second) {
      trace_plugin_pair.second->DispatchEnd();
    }
  }
  trace_plugins_.clear();

  auto& session = session_pair->second;
  session->session_impl->StopBlocking();
  session->started = false;
  is_tracing_started_ = false;
  if (session->config->is_startup_tracing) {
    startup_tracing_file_ = session->config->file_path;
  }
  LOGI("Tracing stopped, file path:" << session->config->file_path);
  // DCHECK(session->config != nullptr);

  if (session->config->record_mode == TraceConfig::RECORD_CONTINUOUSLY) {
    for (int& fd : session->opened_fds) {
#ifndef _WIN32
      fsync(fd);
#endif
      close(fd);
    }
    session->opened_fds.clear();
  } else {
    std::vector<char> trace_data(session->session_impl->ReadTraceBlocking());
    std::ofstream output(session->config->file_path,
                         std::ios::out | std::ios::binary);
    output.write(&trace_data[0], trace_data.size());
    output.flush();
  }

  if (session->config->enable_systrace) {
    hook_systrace_->Uninstall();
  }

  for (const auto& callback : session->complete_callbacks) {
    callback();
  }
  // register an empty backend to avoid using a lock
  tracing_sessions_.erase(session_id);
  LOGI("Tracing stopped, session id: " << session_id);

  return true;
}

void TraceControllerImpl::AddTracePlugin(TracePlugin* plugin) {
  if (plugin) {
    auto plugin_name = plugin->Name();
    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = trace_plugins_.find(plugin_name);
    if (iter != trace_plugins_.end()) {
      LOGI("The trace plugin is already set up.");
      return;
    }
    trace_plugins_.emplace(plugin_name, plugin);
  }
}

bool TraceControllerImpl::DeleteTracePlugin(const std::string& plugin_name) {
  auto iterator = trace_plugins_.find(plugin_name);
  if (iterator != trace_plugins_.end()) {
    trace_plugins_.erase(iterator);
    return true;
  }
  LOGI("There is no trace plugin that you want to remove.");
  return false;
}

void TraceControllerImpl::AddCompleteCallback(
    int session_id, const std::function<void()> callback) {
  auto session_pair = tracing_sessions_.find(session_id);
  if (session_pair == tracing_sessions_.end()) {
    LOGE("Tracing session not found: " << session_id);
    return;
  }
  auto& session = session_pair->second;
  session->complete_callbacks.push_back(callback);
}

void TraceControllerImpl::RemoveCompleteCallbacks(int session_id) {
  auto session_pair = tracing_sessions_.find(session_id);
  if (session_pair == tracing_sessions_.end()) {
    LOGE("Tracing session not found: " << session_id);
    return;
  }
  auto& session = session_pair->second;
  session->complete_callbacks.clear();
}

void TraceControllerImpl::StartStartupTracingIfNeeded() {
  static constexpr const char* const kStartupDuraion = "startup_duration";
  static constexpr const char* const kEnableSystrace = "enable_systrace";
  static constexpr const char* const kResultFile = "result_file";
  const auto startup_config = this->GetStartupTracingConfig();
  if (startup_config.empty()) {
    return;
  }
  rapidjson::Document doc;
  if (doc.Parse(startup_config.c_str()).HasParseError()) {
    return;
  }

  if (!doc.HasMember(kStartupDuraion) || !doc[kStartupDuraion].IsNumber()) {
    return;
  }
  // unit: seconds
  const int duration = doc[kStartupDuraion].GetInt();
  if (duration <= 0) {
    return;
  }

  bool enable_systrace = false;
  if (doc.HasMember(kEnableSystrace) && doc[kEnableSystrace].IsBool()) {
    enable_systrace = doc[kEnableSystrace].GetBool();
  }

  std::string result_file;
  if (doc.HasMember(kResultFile) && doc[kResultFile].IsString()) {
    result_file = doc[kResultFile].GetString();
  }

  static base::NoDestructor<fml::Thread> startup_trace_thread(
      "Lynx_Startup_Trace");
  auto trace_config = std::make_shared<lynx::trace::TraceConfig>();
  if (!result_file.empty()) {
    trace_config->file_path = result_file;
  }
  trace_config->included_categories = {"*"};
  trace_config->excluded_categories = {"*"};
  trace_config->enable_systrace = enable_systrace;
  trace_config->is_startup_tracing = true;
  const int session_id = this->StartTracing(trace_config);
  LOGD("Lynx Startup Trace started");
  auto stop_startup_tracing = [this, session_id]() {
    this->StopTracing(session_id);
    LOGD("Lynx Startup Trace stopped");
    const auto config_path = this->trace_file_dir_ + kStartupTracingFile;
    if (!remove(config_path.c_str())) {
      LOGD("Lynx Startup Trace config file removed");
    } else {
      LOGD("Lynx Startup Trace config file remove fail");
    }
  };
  startup_trace_thread->GetTaskRunner()->PostDelayedTask(
      std::move(stop_startup_tracing), fml::TimeDelta::FromSeconds(duration));
}

void TraceControllerImpl::SetStartupTracingConfig(std::string config) {
  if (trace_file_dir_.empty()) {
    trace_file_dir_ = delegate_->GenerateTracingFileDir();
    if (trace_file_dir_.empty()) {
      return;
    }
  }
  const std::string trace_config_path = trace_file_dir_ + kStartupTracingFile;
  std::ofstream output(trace_config_path, std::ios::out | std::ios::binary);
  if (output.is_open()) {
    output.write(config.data(), config.size());
    output.flush();
    output.close();
  } else {
    LOGE("Write trace_config.json failed!");
  }
}

std::string TraceControllerImpl::GetStartupTracingConfig() {
  if (trace_file_dir_.empty()) {
    trace_file_dir_ = delegate_->GenerateTracingFileDir();
    if (trace_file_dir_.empty()) {
      return "";
    }
  }
  const std::string trace_config_path = trace_file_dir_ + kStartupTracingFile;
  std::ifstream input(trace_config_path, std::ios::in | std::ios::binary);

  if (input.is_open()) {
    std::string config((std::istreambuf_iterator<char>(input)),
                       std::istreambuf_iterator<char>());
    input.close();
    return config;
  } else {
    LOGE("Read trace_config.json failed!");
    return "";
  }
}

std::string TraceControllerImpl::GetStartupTracingFilePath() {
  return startup_tracing_file_;
}

bool TraceControllerImpl::IsTracingStarted() { return is_tracing_started_; }

// private
TraceControllerImpl::TracingSession& TraceControllerImpl::CreateNewSession(
    const std::shared_ptr<TraceConfig> config) {
  static int next_session_id = 0;
  next_session_id++;
  auto new_session = new TracingSession;
  new_session->session_impl = ::perfetto::Tracing::NewTrace();
  new_session->id = next_session_id;
  new_session->config = nullptr;
  new_session->started = false;
  tracing_sessions_[next_session_id] =
      std::unique_ptr<TracingSession>(new_session);
  return *new_session;
}

std::string TraceControllerImpl::GenerateTraceFilePath(
    const std::string& file_dir) {
  std::string file_path = file_dir;
  if (file_path.back() != '/') {
    file_path.append("/");
  }

  thread_local std::thread::id thread_id = std::this_thread::get_id();
  static std::hash<std::thread::id> hasher;
  auto pthd_id = static_cast<unsigned int>(hasher(thread_id));

  time_t now = time(NULL);
  struct tm* tm = localtime(&now);
  std::ostringstream file_name;
  file_name << "lynx-profile-trace-" << pthd_id << "-" << tm->tm_year + 1900
            << "-" << tm->tm_mon + 1 << "-" << tm->tm_mday << "-" << tm->tm_hour
            << tm->tm_min << tm->tm_sec;

  file_path.append(file_name.str());
  return file_path;
}

}  // namespace trace
}  // namespace lynx
