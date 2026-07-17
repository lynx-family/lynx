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

#include <atomic>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#include "base/include/log/logging.h"
#include "base/include/notification_center.h"
#include "base/include/thread/timed_task.h"
#include "base/trace/native/trace_defines.h"
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

namespace {
std::atomic<bool> g_trace_event_runtime_enabled{false};

constexpr uint32_t kPostTraceMemorySampleIntervalMs = 100;
constexpr uint32_t kPostTraceMemorySampleDurationMs = 3000;
constexpr uint32_t kPostTraceMemoryPacketSequenceId = 0x6C796E78;  // "lynx"

using TracePacket = ::perfetto::protos::pbzero::TracePacket;
using TrackDescriptor = ::perfetto::protos::pbzero::TrackDescriptor;
using TracingServiceEvent = ::perfetto::protos::pbzero::TracingServiceEvent;

struct PostTraceMemorySample {
  uint64_t timestamp = 0;
  std::vector<std::pair<std::string, int64_t>> counters;
};

void AppendVarInt(std::vector<char>& data, uint64_t value) {
  while (value >= 0x80) {
    data.push_back(static_cast<char>(value | 0x80));
    value >>= 7;
  }
  data.push_back(static_cast<char>(value));
}

void AppendTracePacket(std::vector<char>& trace_data,
                       const std::string& packet) {
  AppendVarInt(trace_data, static_cast<uint64_t>(0x0A));
  AppendVarInt(trace_data, static_cast<uint64_t>(packet.size()));
  trace_data.insert(trace_data.end(), packet.begin(), packet.end());
}

std::string BuildIncrementalStateClearedPacket(uint64_t timestamp) {
  ::protozero::HeapBuffered<TracePacket> packet;
  packet->set_trusted_packet_sequence_id(kPostTraceMemoryPacketSequenceId);
  packet->set_first_packet_on_sequence(true);
  packet->set_timestamp(timestamp);
  packet->set_timestamp_clock_id(
      static_cast<uint32_t>(TrackEvent::GetTraceClockId()));
  packet->set_sequence_flags(TracePacket::SEQ_INCREMENTAL_STATE_CLEARED);
  return packet.SerializeAsString();
}

std::string BuildTrackDescriptorPacket(uint64_t timestamp,
                                       const ::perfetto::CounterTrack& track) {
  ::protozero::HeapBuffered<TracePacket> packet;
  packet->set_trusted_packet_sequence_id(kPostTraceMemoryPacketSequenceId);
  packet->set_timestamp(timestamp);
  packet->set_timestamp_clock_id(
      static_cast<uint32_t>(TrackEvent::GetTraceClockId()));
  packet->set_sequence_flags(TracePacket::SEQ_NEEDS_INCREMENTAL_STATE);
  track.Serialize(packet->set_track_descriptor<TrackDescriptor>());
  return packet.SerializeAsString();
}

std::string BuildCounterPacket(uint64_t timestamp, uint64_t track_uuid,
                               int64_t value) {
  ::protozero::HeapBuffered<TracePacket> packet;
  packet->set_trusted_packet_sequence_id(kPostTraceMemoryPacketSequenceId);
  packet->set_timestamp(timestamp);
  packet->set_timestamp_clock_id(
      static_cast<uint32_t>(TrackEvent::GetTraceClockId()));
  packet->set_sequence_flags(TracePacket::SEQ_NEEDS_INCREMENTAL_STATE);
  auto* event = packet->set_track_event<TrackEventMessage>();
  event->set_type(TrackEvent_Type::TYPE_COUNTER);
  event->set_track_uuid(track_uuid);
  event->set_double_counter_value(value);
  return packet.SerializeAsString();
}

std::string BuildTracingDisabledPacket(uint64_t timestamp) {
  ::protozero::HeapBuffered<TracePacket> packet;
  packet->set_timestamp(timestamp);
  packet->set_timestamp_clock_id(
      static_cast<uint32_t>(TrackEvent::GetTraceClockId()));
  packet->set_service_event<TracingServiceEvent>()->set_tracing_disabled(true);
  return packet.SerializeAsString();
}

std::vector<PostTraceMemorySample> CollectPostTraceMemorySamples(
    TraceController::Delegate* delegate) {
  std::vector<PostTraceMemorySample> samples;
  if (delegate == nullptr) {
    return samples;
  }
  constexpr uint32_t kSampleCount =
      kPostTraceMemorySampleDurationMs / kPostTraceMemorySampleIntervalMs;
  samples.reserve(kSampleCount);
  for (uint32_t sample_index = 0; sample_index < kSampleCount; ++sample_index) {
    if (sample_index > 0) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(kPostTraceMemorySampleIntervalMs));
    }
    auto data_array = delegate->GetMemoryStats();
    PostTraceMemorySample sample;
    sample.timestamp = GetTraceTimeNs();
    for (size_t i = 0; i + 1 < data_array.size(); i += 2) {
      sample.counters.emplace_back(data_array[i],
                                   std::stoll(data_array[i + 1]));
    }
    if (!sample.counters.empty()) {
      samples.emplace_back(std::move(sample));
    }
  }
  return samples;
}

void AppendPostTraceMemoryPackets(
    std::vector<char>& trace_data,
    const std::vector<PostTraceMemorySample>& samples) {
  if (samples.empty()) {
    return;
  }
  std::map<std::string, ::perfetto::CounterTrack> tracks;
  for (const auto& sample : samples) {
    for (const auto& counter : sample.counters) {
      if (tracks.find(counter.first) == tracks.end()) {
        tracks.emplace(counter.first,
                       ConvertToPerfCounterTrack(lynx::perfetto::CounterTrack(
                           counter.first.c_str())));
      }
    }
  }
  AppendTracePacket(trace_data, BuildIncrementalStateClearedPacket(
                                    samples.front().timestamp));
  for (const auto& track : tracks) {
    AppendTracePacket(trace_data, BuildTrackDescriptorPacket(
                                      samples.front().timestamp, track.second));
  }
  for (const auto& sample : samples) {
    for (const auto& counter : sample.counters) {
      AppendTracePacket(
          trace_data,
          BuildCounterPacket(sample.timestamp, tracks.at(counter.first).uuid,
                             counter.second));
    }
  }
  AppendTracePacket(trace_data,
                    BuildTracingDisabledPacket(samples.back().timestamp));
}

void AppendTraceDataToFile(const std::string& file_path,
                           const std::vector<char>& trace_data) {
  if (trace_data.empty()) {
    return;
  }
  std::ofstream output(file_path,
                       std::ios::out | std::ios::binary | std::ios::app);
  output.write(trace_data.data(), trace_data.size());
  output.flush();
}

void AppendPostTraceMemoryToTraceFile(const std::string& file_path,
                                      TraceController::Delegate* delegate) {
  if (file_path.empty() || delegate == nullptr) {
    return;
  }
  std::vector<PostTraceMemorySample> samples =
      CollectPostTraceMemorySamples(delegate);
  std::vector<char> post_trace_memory_data;
  AppendPostTraceMemoryPackets(post_trace_memory_data, samples);
  if (post_trace_memory_data.empty()) {
    return;
  }
  AppendTraceDataToFile(file_path, post_trace_memory_data);
}
}  // namespace

// Implementations of the definition of the
// "base/trace/native/trace_event_utils_perfetto.h"
constexpr ::perfetto::CounterTrack ConvertToPerfCounterTrack(
    const lynx::perfetto::CounterTrack& counter_track) {
  if (counter_track.is_global_) {
    if (counter_track.unit_name_ != nullptr) {
      return ::perfetto::CounterTrack::Global(
          ::perfetto::DynamicString(counter_track.name_),
          counter_track.unit_name_);
    } else {
      return ::perfetto::CounterTrack::Global(
          ::perfetto::DynamicString(counter_track.name_),
          static_cast<::perfetto::CounterTrack::Unit>(counter_track.unit_));
    }
  }
  auto track =
      ::perfetto::CounterTrack(::perfetto::DynamicString(counter_track.name_))
          .set_category(counter_track.category_)
          .set_unit_name(counter_track.unit_name_)
          .set_unit_multiplier(counter_track.unit_multiplier_)
          .set_is_incremental(counter_track.is_incremental_)
          .set_unit(
              static_cast<::perfetto::CounterTrack::Unit>(counter_track.unit_));
  return track;
}

uint64_t GetFlowId() {
  static std::atomic<uint64_t> sTraceEventFlowId = 0;
  return sTraceEventFlowId++;
}

uint64_t GetTraceTimeNs() { return TrackEventInternal::GetTimeNs(); }

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

    ::perfetto::internal::TrackEventTlsState& tls_state =
        *ctx.GetCustomTlsState();
    // Make sure incremental state is valid.
    ::perfetto::TraceWriterBase* trace_writer = ctx.getTraceWriter();
    ::perfetto::internal::TrackEventIncrementalState* incr_state =
        ctx.GetIncrementalState();

    TrackEventInternal::ResetIncrementalStateIfRequired(
        trace_writer, incr_state, tls_state, trace_timestamp);
    // Write the track descriptor before any event on the track.
    TrackEventInternal::WriteTrackDescriptorIfNeeded(
        track_id == nullptr ? TrackEventInternal::kDefaultTrack
                            : ::perfetto::Track(track_id->id()),
        trace_writer, incr_state, tls_state, trace_timestamp);

    // Write the event itself.
    {
      auto event_ctx = TrackEventInternal::WriteEvent(
          trace_writer, incr_state, tls_state, nullptr,
          static_cast<TrackEvent_Type>(phase), trace_timestamp, false);
      TrackEventInternal::WriteEventName(::perfetto::StaticString(name),
                                         event_ctx, tls_state);
      if (category_name != nullptr) {
        event_ctx.event()->add_categories(category_name, strlen(category_name));
      }
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
                              const uint64_t& counter,
                              const FuncType& callback) {
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
    ::perfetto::internal::TrackEventTlsState& tls_state =
        *ctx.GetCustomTlsState();
    ::perfetto::TraceWriterBase* trace_writer = ctx.getTraceWriter();
    ;
    ::perfetto::internal::TrackEventIncrementalState* incr_state =
        ctx.GetIncrementalState();

    TrackEventInternal::ResetIncrementalStateIfRequired(
        trace_writer, incr_state, tls_state, trace_timestamp);

    // Write the track descriptor before any event on the track.
    TrackEventInternal::WriteTrackDescriptorIfNeeded(
        track, trace_writer, incr_state, tls_state, trace_timestamp);

    // Write the event itself.
    {
      auto event_ctx = TrackEventInternal::WriteEvent(
          trace_writer, incr_state, tls_state, nullptr,
          static_cast<TrackEvent_Type>(phase), trace_timestamp, false);

      event_ctx.event()->set_track_uuid(track.uuid);
      event_ctx.event()->set_double_counter_value(counter);
      if (callback) {
        lynx::perfetto::TrackEvent event(&event_ctx);
        lynx::perfetto::EventContext out_ctx(&event);
        callback(std::move(out_ctx));
      }
    }  // event_ctx
  });
}

bool TraceEventCategoryEnabled(const char* category) {
  return TrackEvent::IsDynamicCategoryEnabled(
      ::perfetto::DynamicCategory(category));
}

bool TraceEventRuntimeEnabled() {
  return g_trace_event_runtime_enabled.load(std::memory_order_relaxed);
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
  last_session_ = &session;

  // handle categories set
  ::perfetto::protos::gen::TrackEventConfig track_event_cfg;
  auto* enabled_categories = track_event_cfg.mutable_enabled_categories();
  auto* disabled_categories = track_event_cfg.mutable_disabled_categories();
  track_event_cfg.set_disable_incremental_timestamps(true);
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
  cfg.set_flush_period_ms(1000);
  cfg.add_buffers()->set_size_kb(config->buffer_size);

  if (config->enable_compress) {
    cfg.set_compression_type(::perfetto::TraceConfig::COMPRESSION_TYPE_DEFLATE);
  }
  // file path
  if (config->file_path.empty() && delegate_) {
    if (trace_file_dir_.empty()) {
      trace_file_dir_ = delegate_->GenerateTracingFileDir();
    }
    config->file_path = GenerateTraceFilePath(trace_file_dir_);
  }

  // setup and start session
  if (config->record_mode == TraceConfig::RECORD_CONTINUOUSLY) {
    cfg.set_file_write_period_ms(config->file_write_period_ms);
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

#ifdef OS_ANDROID
  if (delegate_) {
    delegate_->RefreshATraceTags();
    delegate_->SetIsTracingStarted(true);
  }
#endif
  session.session_impl->StartBlocking();
  g_trace_event_runtime_enabled.store(true, std::memory_order_relaxed);

  // The first event after StartBlocking.
  TraceEventImplementation(
      INTERNAL_TRACE_CATEGORY, "TRACE_BEGIN", TraceEventType::TYPE_INSTANT,
      nullptr, 0, [&](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations(
            "enable_memory_trace", std::to_string(config->enable_memory_trace));
        ctx.event()->add_debug_annotations(
            "memory_trace_force_gc",
            std::to_string(config->memory_trace_force_gc));
      });

  // plugin
  for (auto& trace_plugin_pair : trace_plugins_) {
    if (trace_plugin_pair.second) {
      trace_plugin_pair.second->DispatchBegin();
    }
  }
  if (config->enable_systrace || config->enable_memory_trace) {
    if (!hook_systrace_) {
      hook_systrace_ = std::make_unique<HookSystemTrace>(*this);
    }
    HookSystemTrace::SetupConfig sys_config;
    if (config->enable_memory_trace) {
      // To reduce trace data volume, disable cpu trace when recording memory.
      sys_config.cpu_trace_enabled = false;
    }
    hook_systrace_->Install(sys_config);
  }

  // status
  session.started = true;
  is_tracing_started_ = true;

  // post trace begin notification after session started, plugins started and
  // is_tracing_started_ flag is set true.
  base::NotificationCallback::Notify(LYNX_ON_TRACE_BEGIN_NOTIFICATION, 0);

  LOGI("Tracing started, session id: " << session.id << " buffer size: "
                                       << config->buffer_size);

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
  const std::string trace_file_path = session->config->file_path;
  const bool enable_memory_trace = session->config->enable_memory_trace;
  const auto complete_callbacks = session->complete_callbacks;

  // The last event before StopBlocking.
  TraceEventImplementation(INTERNAL_TRACE_CATEGORY, "TRACE_END",
                           TraceEventType::TYPE_INSTANT, nullptr, 0, nullptr);

  session->session_impl->StopBlocking();
  session->started = false;
  is_tracing_started_ = false;
  g_trace_event_runtime_enabled.store(false, std::memory_order_relaxed);
#ifdef OS_ANDROID
  if (delegate_) {
    delegate_->SetIsTracingStarted(false);
  }
#endif
  if (session->config->is_startup_tracing) {
    startup_tracing_file_ = session->config->file_path;
  }
  LOGI("Tracing stopped, file path:" << session->config->file_path);

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

  if (session->config->enable_systrace && hook_systrace_) {
    hook_systrace_->Uninstall();
  }

  // Release the tracing session before collecting post-trace memory samples, so
  // the tail data reflects memory after Perfetto and trace buffers are freed.
  if (last_session_ && last_session_->id == session_id) {
    last_session_ = nullptr;
  }
  tracing_sessions_.erase(session_id);

  if (enable_memory_trace) {
    // Continuously record memory data for several seconds to ensure that the
    // trace itself has been released and will not affect the accuracy of the
    // data.
    AppendPostTraceMemoryToTraceFile(trace_file_path, delegate_.get());
  }

  for (const auto& callback : complete_callbacks) {
    callback();
  }
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

std::shared_ptr<TraceConfig> TraceControllerImpl::GetLastSessionTraceConfig() {
  if (tracing_sessions_.empty() || last_session_ == nullptr) {
    return nullptr;
  }
  return last_session_->config;
}

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
