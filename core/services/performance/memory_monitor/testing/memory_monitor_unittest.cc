// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/memory_monitor/memory_monitor.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <functional>
#include <limits>
#include <memory>
#include <tuple>
#include <unordered_set>
#include <utility>

#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/thread.h"
#if ENABLE_TRACE_PERFETTO
#include "base/trace/native/trace_controller.h"
#endif
#include "core/renderer/tasm/testing/event_tracker_mock.h"
#include "core/runtime/js/js_realm_manager.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/services/performance/memory_monitor/global_memory_monitor.h"
#include "core/services/performance/memory_monitor/testing/memory_simulation_test_support.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace performance {

namespace {

#if ENABLE_TRACE_PERFETTO
class TestTraceControllerDelegate : public trace::TraceController::Delegate {
 public:
  explicit TestTraceControllerDelegate(std::string directory)
      : directory_(std::move(directory)) {}

  std::string GenerateTracingFileDir() override { return directory_; }
#ifdef OS_ANDROID
  void RefreshATraceTags() override {}
  void SetIsTracingStarted(bool) override {}
#endif

 private:
  std::string directory_;
};
#endif

class MemoryMonitorTestEnvironment : public ::testing::Environment {
 public:
  void SetUp() override {
    MemoryMonitor::ForceEnableForTesting(
        MemoryMonitor::ForceEnableMode::kCurrentProcess);
  }
};

[[maybe_unused]] auto* const memory_monitor_test_environment =
    ::testing::AddGlobalTestEnvironment(new MemoryMonitorTestEnvironment);

class TestPerformanceSender : public PerformanceEventSender {
 public:
  TestPerformanceSender() : PerformanceEventSender(nullptr) {}
  void OnPerformanceEvent(std::unique_ptr<pub::Value>, EventType) override {
    ADD_FAILURE() << "Memory updates must not emit legacy observer events";
  }
};

class TestVM : public runtime::js::VMInstance {
 public:
  explicit TestVM(runtime::js::JSRuntimeType type) : type_(type) {}
  ~TestVM() override {
    if (on_destroy) on_destroy();
  }
  runtime::js::JSRuntimeType GetRuntimeType() const override { return type_; }
  std::string GetDebugDescription() const override { return "test"; }
  size_t GetHeapSize() const override {
    if (on_heap_read) on_heap_read();
    return heap_size;
  }

  size_t heap_size{500};
  std::function<void()> on_heap_read;
  std::function<void()> on_destroy;

 private:
  runtime::js::JSRuntimeType type_;
};

double Number(const report::MoveOnlyEvent& event, const char* key) {
  const auto props = event.GetPropsAsMap();
  auto it = props.find(key);
  if (it == props.end()) {
    ADD_FAILURE() << "Missing field " << key;
    return -1;
  }
  return it->second.GetType() == report::EventProp::Type::kInt32
             ? it->second.GetIntValue()
             : it->second.GetDoubleValue();
}

void UniqueKeys(const report::MoveOnlyEvent& event) {
  std::unordered_set<std::string> keys;
  for (const auto& prop : event.GetProps()) {
    EXPECT_TRUE(keys.insert(prop.GetKey()).second) << prop.GetKey();
  }
}

void CompactSchema(const report::MoveOnlyEvent& event) {
  EXPECT_EQ(Number(event, "schema_version"), 10);
  const auto props = event.GetPropsAsMap();
  for (const auto& [key, value] : props) {
    EXPECT_NE(key, "page_id");
    EXPECT_NE(key, "normalized_url");
    EXPECT_NE(key, "sampled_leak_page_url");
    EXPECT_NE(key.find("sampled_active_url"), 0u);
  }
  EXPECT_EQ(props.count("retained_snapshot_complete"), 0u);
  EXPECT_EQ(props.count("sampled_leak_page_exit_elapsed_ms"), 0u);
  for (const char* field : {"active_page_full_memory_count",
                            "lynx_full_memory_snapshot_valid",
                            "sampled_active_page_full_memory_valid",
                            "sampled_active_page_group_full_memory_valid",
                            "page_required_sources",
                            "source_tasm_element_complete",
                            "source_mts_vm_heap_complete",
                            "source_bts_vm_heap_complete",
                            "source_platform_ui_complete",
                            "current_page_full_bytes",
                            "current_page_core_bytes",
                            "early_full_profile_eligible",
                            "early_core_profile_eligible",
                            "early_usage_summary_full_valid",
                            "early_usage_summary_core_valid",
                            "early_sample_full_count",
                            "early_sample_core_count",
                            "early_sample_mean_full_bytes",
                            "early_sample_mean_core_bytes",
                            "early_sample_peak_full_bytes",
                            "early_sample_peak_core_bytes",
                            "sampling_policy_version",
                            "process_sample_rate",
                            "source_tasm_element_sample_rate",
                            "source_platform_ui_sample_rate",
                            "source_bts_slot_gc_sample_rate",
                            "source_bts_slot_rc_sample_rate",
                            "page_memory_policy_version",
                            "source_mts_vm_heap_enabled",
                            "source_bts_vm_heap_enabled",
                            "measurement_mode",
                            "url_normalization",
                            "anchor_type",
                            "page_snapshot_fresh",
                            "source_age_unknown",
                            "early_summary_method",
                            "early_usage_summary_fresh",
                            "early_30s_reference_expected_count",
                            "process_memory_kind",
                            "process_memory_metric",
                            "process_memory_bytes",
                            "process_rss_bytes",
                            "process_rss_at_trigger_bytes",
                            "trigger_rss_previous_milestone_bytes",
                            "trigger_rss_milestone_bytes",
                            "lynx_accounted_process_memory_ratio",
                            "lynx_shared_vm_process_memory_ratio",
                            "lynx_tracked_page_retained_process_memory_ratio",
                            "sampled_leak_page_attributed_process_memory_ratio",
                            "rss_poll_interval_ms",
                            "trigger_coalesced_count",
                            "lynx_snapshot_fresh",
                            "byte_time_policy",
                            "sampled_active_page_snapshot_fresh",
                            "sampled_active_url_snapshot_fresh",
                            "sampled_leak_page_scope",
                            "sampled_leak_page_measurement_kind",
                            "sampled_vm_selection_weight_type",
                            "trigger_vm_slot_page_capacity",
                            "source_tasm_element_enabled",
                            "source_platform_ui_enabled",
                            "source_bts_vm_slot_enabled",
                            "anchor_time_ms",
                            "actual_offset_ms",
                            "sample_status",
                            "current_page_accounted_bytes",
                            "page_accounted_source_mask",
                            "page_eligible_source_mask",
                            "page_full_memory_valid",
                            "page_core_memory_valid",
                            "page_required_source_mask",
                            "trigger_reason_mask",
                            "max_source_receipt_age_ms",
                            "early_scheduled_full_valid_mask",
                            "early_scheduled_core_valid_mask",
                            "early_usage_summary_close_reason",
                            "early_window_duration_bucket",
                            "early_required_scheduled_mask",
                            "has_early_30s_reference",
                            "early_30s_reference_full_valid",
                            "early_30s_reference_core_valid",
                            "early_scheduled_mean_full_bytes",
                            "early_scheduled_mean_core_bytes",
                            "snapshot_id",
                            "report_reason",
                            "trigger_timer_elapsed_minutes",
                            "process_pss_at_trigger_bytes",
                            "snapshot_partial",
                            "lynx_accounted_source_mask",
                            "lynx_enabled_source_mask",
                            "active_page_memory_bytes_sum",
                            "active_page_memory_bytes_max",
                            "shared_vm_heap_bytes_max",
                            "shared_vm_tracked_exited_page_retained_bytes_max",
                            "sampled_active_page_instance_id",
                            "sampled_active_page_url",
                            "sampled_active_page_source_mask",
                            "sampled_active_page_weight",
                            "sampled_active_url_present",
                            "sampled_active_url_collected_instance_count",
                            "sampled_active_url_source_mask",
                            "sampled_active_url_selection_probability",
                            "sampled_active_url_instance_count_weighted",
                            "sampled_leak_page_selection_probability",
                            "sampled_leak_page_instance_id",
                            "sampled_leak_page_vm_heap_bytes",
                            "sampled_leak_page_attributed_bytes",
                            "sampled_leak_page_attributed_process_pss_ratio",
                            "sampled_vm_scope",
                            "sampled_active_page_alive_10m_weighted",
                            "sampled_active_page_alive_20m_weighted",
                            "sampled_active_page_alive_30m_weighted",
                            "sampled_vm_selection_probability",
                            "sampled_vm_attributed_heap_bytes",
                            "sampled_vm_slot_tracking_enabled",
                            "sampled_vm_slot_tracking_supported",
                            "sampled_vm_slot_tracking_saturated",
                            "trigger_vm_slot_capacity_reached",
                            "trigger_vm_slot_milestone",
                            "trigger_vm_slot_overflowed",
                            "process_session_id",
                            "event_id",
                            "event_build_duration_us",
                            "shared_vm_tracked_exited_page_count",
                            "shared_vm_exited_page_pending_gc_count",
                            "shared_vm_slot_saturated_count",
                            "shared_vm_untracked_page_count_sum",
                            "sampled_leak_page_retained_bytes",
                            "sampled_leak_page_vm_name",
                            "sampled_leak_page_vm_tracking_saturated",
                            "sampled_leak_page_vm_untracked_page_count",
                            "instance_id",
                            "accounting_scope_changed",
                            "sample_lateness_ms",
                            "early_required_scheduled_count",
                            "early_sample_count",
                            "summary_end_time_known"}) {
    EXPECT_EQ(props.count(field), 0u) << field;
  }
  for (const char* scope : {"full", "core"}) {
    for (const char* suffix : {"attempted_mask", "coverage_ratio", "max_gap_ms",
                               "first_offset_ms", "last_offset_ms"}) {
      const auto field = std::string("early_sample_") + scope + "_" + suffix;
      EXPECT_EQ(props.count(field), 0u) << field;
    }
  }
  for (const char* source :
       {"tasm_element", "mts_vm_heap", "bts_vm_heap", "platform_ui"}) {
    for (const char* suffix :
         {"expected_count", "completed_count", "complete"}) {
      const auto field = std::string("source_") + source + "_" + suffix;
      EXPECT_EQ(props.count(field), 0u) << field;
    }
  }
}

void OnReporter(const std::function<void()>& task) {
  fml::AutoResetWaitableEvent done;
  report::EventTrackerPlatformImpl::GetReportTaskRunner()->PostTask([&] {
    task();
    done.Signal();
  });
  done.Wait();
}

PageMemoryUsage FullUsage(int64_t total, int64_t bts, bool shared) {
  PageMemoryUsage usage;
  usage.total_bytes = total;
  usage.bts_bytes = bts;
  usage.ui_bytes = 10;
  usage.element_bytes = total - bts - 20;
  usage.mts_bytes = 10;
  usage.bts_shared = shared;
  return usage;
}

}  // namespace

class MemoryMonitorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    OnReporter([] {
      report::EventTrackerWaitableEvent::stack_.clear();
      report::EventTrackerWaitableEvent::generic_info_by_instance_.clear();
      report::EventTrackerWaitableEvent::extra_params_by_instance_.clear();
      report::EventTrackerWaitableEvent::query_count_by_instance_.clear();
    });
  }
  void TearDown() override {
    // Producers have finished; drain delivery before the next case.
    OnReporter([] { report::EventTracker::Flush(1); });
    OnReporter([] {
      report::EventTrackerWaitableEvent::generic_info_by_instance_.clear();
      report::EventTrackerWaitableEvent::extra_params_by_instance_.clear();
      report::EventTrackerWaitableEvent::query_count_by_instance_.clear();
    });
  }

  static void Populate(GlobalMemoryMonitor& monitor) {
    InstanceState page;
    page.SetUrl("app/item");
    page.created_at_ms = 100;
    page.mts_known = page.bts_known = true;
    page.mts_context_type = runtime::LepusNGContextType;
    page.bts_runtime_type = runtime::js::JSRuntimeType::quickjs;
    monitor.instance_state_.emplace(1, std::move(page));
  }

  static BtsVmState& AddVM(GlobalMemoryMonitor& monitor) {
    monitor.bts_vm_state_.push_back(std::make_unique<BtsVmState>());
    return *monitor.bts_vm_state_.back();
  }

  static report::MoveOnlyEvent BuildEvent(
      GlobalMemoryMonitor& monitor, GlobalMemoryMonitor::Snapshot& snapshot,
      int64_t now, int64_t pss, const char* event_name = nullptr) {
    // Most focused tests predate event splitting and assert calculations that
    // span several new event schemas. Merge their properties only in the test
    // process; routing tests inspect BuildEvents directly.
    snapshot.trigger.reasons |= GlobalMemoryMonitor::kTimer;
    auto events = monitor.BuildEvents(snapshot, now, pss);
    if (event_name) {
      for (auto& event : events) {
        if (event.GetName() == event_name) return std::move(event);
      }
      return {};
    }
    report::MoveOnlyEvent merged;
    merged.SetName("memory_test_snapshot");
    merged.SetInstanceId(report::kUnknownInstanceId);
    std::unordered_set<std::string> keys;
    for (const auto& event : events) {
      for (const auto& prop : event.GetProps()) {
        if (!keys.insert(prop.GetKey()).second) continue;
        switch (prop.GetType()) {
          case report::EventProp::Type::kString:
            merged.SetProps(prop.GetKey().c_str(), prop.GetStringValue());
            break;
          case report::EventProp::Type::kInt32:
            merged.SetProps(prop.GetKey().c_str(), prop.GetIntValue());
            break;
          case report::EventProp::Type::kDouble:
            merged.SetProps(prop.GetKey().c_str(), prop.GetDoubleValue());
            break;
        }
      }
    }
    return merged;
  }

  static void FillRecords(MemoryMonitor& page) {
    page.UpdateMemoryUsage(MemoryRecord(kCategoryTasmElement, 20));
    page.UpdateMemoryUsage(MemoryRecord(kCategoryMTSEngine, 10));
    page.UpdateMemoryUsage(MemoryRecord(kCategoryBTSEngine, 40));
    page.UpdateMemoryUsage(MemoryRecord("view", 30));
    page.RefreshMemoryUsage();
  }

  void RunResidencyCohort() {
    OnReporter([] {
      GlobalMemoryMonitor global;
      auto create = [&](int id, const char* key, int64_t at) {
        global.OnInstanceCreated(id, at, [&](auto& page) { page.SetUrl(key); });
        global.poll_timer_->Stop();
      };
      auto load = [&](int id) { global.ResolveInstancePageKey(id); };
      create(1, "pinned", 100);
      create(2, "transient", 100);
      create(3, "not_loaded", 100);
      create(6, "exited_before_load", 100);
      load(1);
      load(2);
      global.OnInstanceDestroyed(2, 2000);
      global.OnInstanceDestroyed(6, 2000);
      EXPECT_EQ(global.instance_state_.count(2), 0u);
      // No failure is counted while the process has not completed the window.
      EXPECT_TRUE(global.TakeResidencyReports(600099).empty());
      load(1);
      create(1, "pinned", 2000);
      EXPECT_EQ(global.residency_observations_.size(), 4u);
      auto events = global.TakeResidencyReports(600100);
      ASSERT_EQ(events.size(), 4u);
      for (const auto& event : events) {
        UniqueKeys(event);
        CompactSchema(event);
        const simulation::Row row(event);
        EXPECT_EQ(event.GetName(), "lynxsdk_memory_page_residency");
        EXPECT_FALSE(row.Has("report_kind"));
        EXPECT_FALSE(row.Has("process_session_id"));
        EXPECT_FALSE(row.Has("snapshot_seq"));
        EXPECT_EQ(row.Number("residency_threshold_ms"), 600000);
        EXPECT_EQ(row.Number("evaluated_page_count"), 1);
        EXPECT_EQ(row.Number("alive_page_count"),
                  row.Text("page_key") == "pinned" ||
                          row.Text("page_key") == "not_loaded"
                      ? 1
                      : 0);
        EXPECT_FALSE(row.Has("active_page_count"));
      }
      EXPECT_TRUE(global.residency_observations_.empty());
      EXPECT_EQ(global.instance_state_.at(1).residency_observation, nullptr);
      EXPECT_TRUE(global.TakeResidencyReports(900000).empty());

      // Reused IDs cannot clear the new page's observation. Delayed delivery
      // uses the captured exit time, including the exact-deadline boundary.
      create(4, "after_deadline", 700000);
      global.OnInstanceDestroyed(4, 1300001);
      create(4, "reused_id", 1400000);
      auto* pending = global.instance_state_.at(4).residency_observation;
      events = global.TakeResidencyReports(1500000);
      ASSERT_EQ(events.size(), 1u);
      EXPECT_EQ(Number(events[0], "alive_page_count"), 1);
      EXPECT_EQ(global.instance_state_.at(4).residency_observation, pending);
      global.OnInstanceDestroyed(4, 2000000);
      events = global.TakeResidencyReports(2100000);
      ASSERT_EQ(events.size(), 1u);
      EXPECT_EQ(Number(events[0], "alive_page_count"), 0);
      EXPECT_TRUE(global.residency_observations_.empty());

      // First load can resolve the key but cannot reset the creation deadline.
      const auto created = MemoryNowMs() - 10000;
      create(5, "first_load_fallback", created);
      report::EventTrackerPlatformImpl::UpdateGenericInfo(
          5, "page_id", std::string("first_load_host"));
      MemoryMonitor page(nullptr, base::LogContext{}, 5);
      page.global_ = &global;
      page.OnFirstLoadComplete();
      page.sample_timer_->Stop();
      page.OnFirstLoadComplete();
      EXPECT_EQ(global.instance_state_.at(5).created_at_ms, created);
      EXPECT_EQ(global.residency_observations_.size(), 1u);
      events = global.TakeResidencyReports(created + 600000);
      ASSERT_EQ(events.size(), 1u);
      EXPECT_EQ(simulation::Row(events[0]).Text("page_key"), "first_load_host");
      EXPECT_EQ(Number(events[0], "alive_page_count"), 1);
      EXPECT_TRUE(global.residency_observations_.empty());
      EXPECT_EQ(global.next_residency_at_ms_, 0);

      // Interleaved deadlines cannot rely on insertion order. Vector growth
      // and moving pending entries must preserve pointees and exit identities.
      for (int id = 10; id < 210; ++id) {
        create(id, "batch", id % 2 ? 4000000 : 3000000);
      }
      auto* retained = global.instance_state_.at(209).residency_observation;
      EXPECT_EQ(global.next_residency_at_ms_, 3600000);
      events = global.TakeResidencyReports(3600000);
      ASSERT_EQ(events.size(), 1u);
      EXPECT_EQ(Number(events[0], "evaluated_page_count"), 100);
      EXPECT_EQ(Number(events[0], "alive_page_count"), 100);
      EXPECT_EQ(global.residency_observations_.size(), 100u);
      EXPECT_EQ(global.next_residency_at_ms_, 4600000);
      EXPECT_EQ(global.instance_state_.at(209).residency_observation, retained);
      global.WithInstance(209, [](auto& state) { state.SetUrl("late_url"); });
      global.OnInstanceDestroyed(209, 4500000);
      events = global.TakeResidencyReports(4600000);
      ASSERT_EQ(events.size(), 2u);
      for (const auto& event : events) {
        const simulation::Row row(event);
        const bool exited = row.Text("page_key") == "late_url";
        EXPECT_EQ(row.Number("evaluated_page_count"), exited ? 1 : 99);
        EXPECT_EQ(row.Number("alive_page_count"), exited ? 0 : 99);
      }
      EXPECT_TRUE(global.residency_observations_.empty());

      // The existing poll processes residency even without a snapshot trigger.
      create(300, "poll", MemoryNowMs() - 600001);
      global.next_timer_at_ms_ = std::numeric_limits<int64_t>::max();
      global.pss_high_water_bucket_ = std::numeric_limits<int64_t>::max();
      global.PollProcessMemory();
      EXPECT_TRUE(global.residency_observations_.empty());
      EXPECT_EQ(global.next_residency_at_ms_, 0);
      EXPECT_EQ(global.instance_state_.at(300).residency_observation, nullptr);
    });
  }

  void RunVmDetailAndResidual() {
    OnReporter([] {
      GlobalMemoryMonitor global;
      global.random_.seed(43);
      GlobalMemoryMonitor::Snapshot snapshot;
      snapshot.started_at_ms = 4000;
      snapshot.trigger.reasons = GlobalMemoryMonitor::kTimer;
      std::vector<std::shared_ptr<TestVM>> engines;
      for (const auto type :
           {runtime::js::JSRuntimeType::quickjs,
            runtime::js::JSRuntimeType::jsc, runtime::js::JSRuntimeType::v8,
            runtime::js::JSRuntimeType::jsvm}) {
        engines.push_back(std::make_shared<TestVM>(type));
        global.OnBtsVMCreate(GlobalMemoryMonitor::RuntimeName(type), "",
                             engines.back());
        global.poll_timer_->Stop();
        auto& vm = *global.bts_vm_state_.back();
        vm.created_at_ms = 100;
        vm.total_page_count = 1000 + engines.size();
        vm.heap_valid = type == runtime::js::JSRuntimeType::quickjs;
        vm.measured_at_ms = 3000;
      }
      std::unordered_set<std::string> selected;
      for (int i = 0; i < 128; ++i) {
        const simulation::Row row(BuildEvent(global, snapshot, 4000, 1000));
        const auto type = row.Text("vm_runtime_type");
        selected.insert(type);
        EXPECT_EQ(row.Number("vm_selection_weight"), 4);
        EXPECT_EQ(row.Number("vm_total_page_count_weighted"),
                  row.Number("vm_total_page_count") * 4);
        EXPECT_EQ(row.Has("vm_heap_bytes"), type == "quickjs");
        if (type == "quickjs") {
          EXPECT_EQ(row.Number("vm_heap_bytes_weighted"), 0);
          EXPECT_EQ(row.Number("vm_heap_source_age_ms"), 1000);
        }
      }
      EXPECT_EQ(selected.size(), 4u);
      global.bts_vm_state_.resize(1);
      auto& vm = *global.bts_vm_state_[0];
      auto create = [&](int id, int slot) {
        global.OnInstanceCreated(id, 100, [&](auto& page) {
          page.bts_vm = engines[0].get();
          page.slot = slot;
        });
      };
      create(1, 3);
      create(2, 4);
      create(3, 5);
      global.OnInstanceDestroyed(1, 1000);
      global.OnInstanceDestroyed(2, 2000);
      BtsMemorySample sample;
      sample.heap_valid = sample.slots_valid = sample.is_full_gc = true;
      sample.heap_size = 1000;
      sample.measured_at_ms = 1500;
      sample.slots[3] = 30;
      sample.slots[4] = 40;
      sample.slots[5] = 500;  // Active page is not part of the residual.
      global.OnBtsVMMemoryUpdate(engines[0].get(), sample);
      auto expect = [&](bool present, double bytes = 0) {
        const simulation::Row row(BuildEvent(global, snapshot, 4000, 2000,
                                             "lynxsdk_memory_vm_quickjs"));
        if (row.Has("report_kind")) {
          EXPECT_EQ(row.Text("report_kind"), "sample");
        }
        EXPECT_EQ(row.Has("vm_retained_bytes"), present);
        EXPECT_EQ(row.Has("vm_retained_bytes_weighted"), present);
        EXPECT_EQ(row.Has("vm_retained_selection_weight"), present);
        if (present) {
          EXPECT_EQ(row.Number("vm_retained_bytes"), bytes);
          EXPECT_EQ(row.Number("vm_retained_bytes_weighted"), bytes);
        }
      };
      expect(false);  // Page 2 still lacks a post-exit full GC.
      sample.measured_at_ms = 3000;
      global.OnBtsVMMemoryUpdate(engines[0].get(), sample);
      expect(true, 70);
      vm.allocated_slots = 253;
      expect(false);
      vm.allocated_slots = 252;
      expect(true, 70);
      vm.untracked_page_count = 1;
      expect(false);
      vm.untracked_page_count = 0;
      snapshot.partial = true;
      expect(true, 70);
      snapshot.partial = false;
      vm.has_per_instance_data = false;
      expect(false);
      vm.has_per_instance_data = true;
      sample.measured_at_ms = 3500;
      sample.slots[3] = sample.slots[4] = 0;
      global.OnBtsVMMemoryUpdate(engines[0].get(), sample);
      expect(true, 0);
      create(4, -1);
      global.OnInstanceDestroyed(4, 3600);
      EXPECT_EQ(global.instance_state_.count(4), 0u);
      EXPECT_TRUE(vm.has_untracked_exited_pages);
      sample.measured_at_ms = 3700;
      global.OnBtsVMMemoryUpdate(engines[0].get(), sample);
      expect(false);  // A later full GC cannot restore lost attribution.
    });
  }

  void RunSimulation(const char* platform) {
    using simulation::Integer;
    using simulation::Json;
    using simulation::kMiB;
    fml::Thread js_thread("memory-simulation-js");
    const auto js_runner = js_thread.GetTaskRunner();
    const std::string filename =
        std::string("simulation_") + platform + ".json";
    const std::string source_relative =
        "core/services/performance/memory_monitor/testing/" + filename;
    const std::string workspace_relative = "lynx/" + source_relative;
    std::ifstream input;
    // Prefer the GN-copied runtime resource when launched from the output
    // directory; keep public and internal source-tree lookup as fallbacks.
    for (const auto& path :
         {filename, source_relative, workspace_relative,
          "../../" + source_relative, "../../" + workspace_relative}) {
      input.open(path, std::ios::binary | std::ios::ate);
      if (input.good()) break;
      input.clear();
    }
    ASSERT_TRUE(input.good())
        << "Missing fixture; run git lfs pull: " << source_relative;
    const auto length = input.tellg();
    ASSERT_GT(length, 0);
    ASSERT_LT(length, 15000000);
    input.seekg(0);
    const std::string json((std::istreambuf_iterator<char>(input)), {});
    ASSERT_NE(json.find("\"format_version\""), std::string::npos)
        << "Fixture must be hydrated, not a Git LFS pointer: "
        << source_relative;
    rapidjson::Document data;
    data.Parse(json.c_str());
    ASSERT_FALSE(data.HasParseError());
    ASSERT_EQ(data["format_version"].GetInt(), 15);
    ASSERT_EQ(data["event_schema"].GetInt(), 10);
    ASSERT_EQ(data["platform"].GetString(), std::string(platform));
    ASSERT_EQ(data["stats"]["oom"].GetUint(),
              data["normal_users"].GetUint() / 100);
    ASSERT_EQ(data["stats"]["ram_8"].GetUint(),
              data["normal_users"].GetUint() / 2);
    ASSERT_EQ(data["stats"]["ram_12"].GetUint(),
              data["normal_users"].GetUint() * 3 / 10);
    ASSERT_EQ(data["stats"]["ram_16"].GetUint(),
              data["normal_users"].GetUint() / 5);
    simulation::Analytics analytics;
    const auto& oracle = data["oracle"];
    const auto& sessions = data["sessions"];
    size_t replayed_snapshots = 0;
    for (const auto& session : sessions.GetArray()) {
      SCOPED_TRACE(std::string(platform) + "/" +
                   std::to_string(session["id"].GetInt()));
      size_t session_snapshots = 0;
      OnReporter([&] {
        report::EventTrackerWaitableEvent::generic_info_by_instance_.clear();
        report::EventTrackerWaitableEvent::extra_params_by_instance_.clear();
        GlobalMemoryMonitor global;
        global.random_.seed(data["seed"].GetUint64() +
                            session["id"].GetUint64());
        const auto& pages = session["pages"];
        const auto& vms = session["vms"];
        const auto& pools = session["pools"];
        const auto& slots = session["slots"];
        const auto& checkpoints = session["checkpoints"];
        const int64_t stop = session["stop"].GetInt64();
        // Real runtime/timer threads cannot wait two simulated hours. Inject
        // virtual timestamps at the existing friend boundary, not into the
        // production API, and drive the exact production event builders.
        constexpr int64_t epoch = 10000000;
        std::vector<std::shared_ptr<TestVM>> engines(vms.Size());
        std::vector<int64_t> last_gc(vms.Size(), -1);
        std::vector<std::unique_ptr<MemoryMonitor>> monitors(pages.Size());
        std::vector<bool> active(pages.Size(), false);
        for (size_t i = 0; i < pools.Size(); ++i) {
          const auto& pool = pools[i];
          global.OnMTSRuntimePoolUpdate(
              static_cast<int32_t>(100000 + i),
              data["urls"][Integer(pool, 0)].GetString(),
              static_cast<runtime::ContextType>(Integer(pool, 1)),
              epoch + Integer(pool, 2), static_cast<uint64_t>(Integer(pool, 3)),
              Integer(pool, 4) * kMiB);
        }
        ASSERT_NE(global.poll_timer_, nullptr);
        global.poll_timer_->Stop();
        struct Action {
          int64_t time;
          int kind;
          size_t index;
          size_t sample;
          bool operator<(const Action& other) const {
            return std::tie(time, kind, index, sample) <
                   std::tie(other.time, other.kind, other.index, other.sample);
          }
        };
        // Exit before sample at the same boundary; VM destroy after last exit.
        std::vector<Action> actions;
        for (size_t i = 0; i < vms.Size(); ++i) {
          actions.push_back({Integer(vms[i], 2), 2, i, 0});
          if (Integer(vms[i], 3) < stop)
            actions.push_back({Integer(vms[i], 3), 1, i, 0});
          const auto interval = Integer(vms[i], 6);
          if (interval > 0) {
            for (auto at = Integer(vms[i], 2) + interval;
                 at < std::min(stop, Integer(vms[i], 3)); at += interval)
              actions.push_back({at, 4, i, 1});
          }
        }
        for (size_t i = 0; i < pages.Size(); ++i) {
          const auto& p = pages[i];
          if (Integer(p, 0) || Integer(p, 2) >= stop) continue;
          actions.push_back({Integer(p, 2), 3, i, 0});
          if (Integer(p, 2) + 600000 < stop)
            actions.push_back({Integer(p, 2) + 600000, 7, i, 0});
          if (Integer(p, 3) < stop) actions.push_back({Integer(p, 3), 0, i, 0});
          for (size_t offset = 1; offset < kPageMemoryOffsetsMs.size();
               ++offset) {
            auto at = Integer(p, 2) + kPageMemoryOffsetsMs[offset];
            if (kPageMemoryOffsetsMs[offset] == 5000 && (Integer(p, 8) & 16))
              at += 1001;
            if (at < stop && at < Integer(p, 3))
              actions.push_back({at, 5, i, offset});
          }
          if (Integer(p, 8) & 32)
            actions.push_back({Integer(p, 2) + 12000, 4, i, 0});
        }
        for (size_t i = 0; i < checkpoints.Size(); ++i)
          actions.push_back({Integer(checkpoints[i], 0), 6, i, 0});
        std::sort(actions.begin(), actions.end());

        auto update_heaps = [&](int64_t at, int64_t gc_vm = -1) {
          std::vector<BtsMemorySample> samples(vms.Size());
          for (size_t i = 0; i < vms.Size(); ++i) {
            if (!engines[i]) continue;
            samples[i].heap_size = Integer(vms[i], 4) * kMiB;
            samples[i].heap_valid = Integer(vms[i], 0) != 1;
            samples[i].slots_valid =
                Integer(vms[i], 0) == 0 && Integer(vms[i], 5);
            if (samples[i].slots_valid)
              samples[i].slots[fml::JSMemoryTrackSlotType::Common] =
                  samples[i].heap_size;
            samples[i].measured_at_ms = epoch + at;
            samples[i].is_full_gc = static_cast<int64_t>(i) == gc_vm;
          }
          for (size_t i = 0; i < pages.Size(); ++i) {
            const auto& p = pages[i];
            const auto vm = Integer(p, 4);
            if (Integer(p, 0) || Integer(p, 2) > at || vm < 0 || !engines[vm])
              continue;
            const auto bytes =
                at < Integer(p, 3)
                    ? (Integer(p, 8) & 2
                           ? 0
                           : simulation::PageBytes(p, at) * 4 / 10)
                : Integer(vms[vm], 0) == 0
                    ? simulation::ExitedBytes(p, last_gc[vm])
                    : Integer(p, 6) * kMiB;
            samples[vm].heap_size += bytes;
            const auto slot = slots[i].GetInt();
            if (slot >= 2) {
              const auto unknown = bytes * Integer(p, 9) / 100;
              samples[vm].slots[fml::JSMemoryTrackSlotType::Unknown] += unknown;
              // Overflow combines many instances; valid page slots are unique.
              samples[vm].slots[slot] += bytes - unknown;
            }
          }
          for (size_t i = 0; i < vms.Size(); ++i) {
            if (!engines[i]) continue;
            if (samples[i].slots_valid) {
              size_t total = 0;
              for (const auto bytes : samples[i].slots) total += bytes;
              EXPECT_EQ(total, samples[i].heap_size)
                  << "Reserved slots partition the heap; never add it twice";
            }
            engines[i]->heap_size = samples[i].heap_size;
            if (Integer(vms[i], 0) == 0)
              global.OnBtsVMMemoryUpdate(engines[i].get(),
                                         std::move(samples[i]));
          }
        };
        auto update_page = [&](size_t i, int64_t at) {
          const auto& p = pages[i];
          const int flags = Integer(p, 8);
          const int64_t total = simulation::PageBytes(p, at);
          const int64_t element = total / 10, mts = total / 5;
          const int64_t bts = flags & 2 ? 0 : total * 4 / 10;
          auto& page = *monitors[i];
          page.UpdateMemoryUsage(MemoryRecord(kCategoryTasmElement, element));
          if (!(flags & 4))
            page.UpdateMemoryUsage(MemoryRecord(kCategoryMTSEngine, mts));
          if (!(flags & 2))
            page.UpdateMemoryUsage(MemoryRecord(kCategoryBTSEngine, bts));
          if (!(flags & 1))
            page.UpdateMemoryUsage(
                MemoryRecord("view", total - element - mts - bts));
        };
        for (const auto& action : actions) {
          const auto at = action.time;
          if (at >= stop) break;
          const auto i = action.index;
          if (action.kind == 2) {
            static const runtime::js::JSRuntimeType types[] = {
                runtime::js::JSRuntimeType::quickjs,
                runtime::js::JSRuntimeType::jsc, runtime::js::JSRuntimeType::v8,
                runtime::js::JSRuntimeType::jsvm};
            engines[i] = std::make_shared<TestVM>(types[Integer(vms[i], 0)]);
            const auto name = data["vm_names"][Integer(vms[i], 1)].GetString();
            global.OnBtsVMCreate(name, "", engines[i]);
            global.poll_timer_->Stop();
            global.started_at_ms_ = epoch;
            auto* vm = global.FindVM(engines[i].get());
            ASSERT_NE(vm, nullptr);
            vm->js_thread_runner = js_runner;
            vm->created_at_ms = vm->idle_since_ms = epoch + at;
            vm->slot_tracking_enabled = Integer(vms[i], 5);
          } else if (action.kind == 1) {
            js_runner->PostSyncTask([&engines, i] { engines[i].reset(); });
          } else if (action.kind == 3) {
            const auto& p = pages[i];
            const int flags = Integer(p, 8), owner = Integer(p, 4);
            const auto slot = slots[i].GetInt();
            const int32_t id = static_cast<int32_t>(i + 1);
            const auto key_index = Integer(p, 1);
            const auto& host_id = data["page_ids"][key_index];
            report::EventTrackerPlatformImpl::UpdateGenericInfo(
                id, "page_id", std::string(host_id.GetString()));
            if (slot != -1) {
              global.OnBtsVMSlotAllocate(engines[owner].get(),
                                         slot == 2 ? -1 : slot);
              auto* vm = global.FindVM(engines[owner].get());
              EXPECT_EQ(vm->pending_overflow,
                        slot == 2 && vm->untracked_page_count == 1);
              // Capture trigger timing from the fixture and replay it below;
              // do not queue a real-clock RequestReport with host PSS.
              vm->pending_overflow = false;
            }
            global.OnInstanceCreated(id, epoch + at, [&](auto& state) {
              state.SetUrl(data["urls"][key_index].GetString());
              state.mts_known = state.bts_known = true;
              state.mts_context_type = flags & 4 ? runtime::VMContextType
                                                 : runtime::LepusNGContextType;
              state.bts_expected = !(flags & 2);
              state.bts_runtime_type =
                  owner >= 0 ? engines[owner]->GetRuntimeType()
                             : runtime::js::JSRuntimeType::quickjs;
              state.bts_vm =
                  owner >= 0 && !(flags & 2) ? engines[owner].get() : nullptr;
              state.slot =
                  slot >= 0 ? slot : fml::JSMemoryTrackSlotType::Unknown;
            });
            global.poll_timer_->Stop();
            global.started_at_ms_ = epoch;
            active[i] = true;
            monitors[i] = std::make_unique<MemoryMonitor>(
                nullptr, base::LogContext{}, id);
            monitors[i]->global_ = &global;
            global.instance_state_.at(id).memory_monitor = monitors[i].get();
            update_heaps(at);
            update_page(i, at);
            monitors[i]->OnFirstLoadComplete();
            monitors[i]->sample_timer_->Stop();
            monitors[i]->anchor_at_ms_ = epoch + at;
          } else if (action.kind == 7) {
            for (auto& event : global.TakeResidencyReports(epoch + at)) {
              report::EventTracker::OnGlobalEvent(
                  [event = std::move(event)](auto& target) mutable {
                    target = std::move(event);
                  });
            }
          } else if (action.kind == 0) {
            global.OnInstanceDestroyed(static_cast<int32_t>(i + 1), epoch + at);
            active[i] = false;
            update_heaps(at);
            monitors[i]->SetExitTime(epoch + at);
            monitors[i].reset();
          } else if (action.kind == 4) {
            if (action.sample == 1) {
              last_gc[i] = at;
              update_heaps(at, static_cast<int64_t>(i));
            }
          } else if (action.kind == 5) {
            update_heaps(at);
            update_page(i, at);
            monitors[i]->ReportScheduled(action.sample, epoch + at);
          } else {
            const auto& checkpoint = checkpoints[i];
            update_heaps(at);
            // Producer callbacks advance raw records. The global snapshot
            // refreshes every active page before reading its aggregate.
            for (size_t p = 0; p < monitors.size(); ++p)
              if (active[p]) update_page(p, at);
            GlobalMemoryMonitor::Trigger trigger;
            trigger.reasons = Integer(checkpoint, 1);
            trigger.pss_before = Integer(checkpoint, 3);
            trigger.pss_after = Integer(checkpoint, 4);
            if (Integer(checkpoint, 2) >= 0) {
              const size_t page = Integer(checkpoint, 2);
              const size_t owner = Integer(pages[page], 4);
              const auto* vm = global.FindVM(engines[owner].get());
              ASSERT_NE(vm, nullptr);
              trigger.overflow_vm = std::make_unique<BtsVmState>(*vm);
            }
            auto snapshot = global.CollectSnapshot(std::move(trigger));
            snapshot.started_at_ms = epoch + at;
            auto events = global.BuildEvents(snapshot, epoch + at,
                                             Integer(checkpoint, 5));
            global.previous_snapshot_at_ms_ = snapshot.started_at_ms;
            const report::MoveOnlyEvent* process = nullptr;
            for (const auto& event : events) {
              if (event.GetName() == "lynxsdk_memory_process") {
                process = &event;
                break;
              }
            }
            ASSERT_NE(process, nullptr);
            simulation::Row row(*process);
            EXPECT_EQ(row.Number("lynx_accounted_memory_bytes"),
                      Integer(checkpoint, 6));
            EXPECT_FALSE(row.Has("lynx_full_memory_snapshot_valid"));
            EXPECT_EQ(row.Number("shared_vm_heap_bytes_sum"),
                      Integer(checkpoint, 7));
            EXPECT_EQ(
                row.Number("shared_vm_tracked_exited_page_retained_bytes"),
                Integer(checkpoint, 8));
            EXPECT_EQ(row.Number("active_page_count"), Integer(checkpoint, 9));
            uint64_t pool_count = 0;
            uint64_t pool_runtime_count = 0;
            int64_t pool_heap_bytes = 0;
            for (const auto& pool : pools.GetArray()) {
              if (Integer(pool, 2) > at) continue;
              ++pool_count;
              pool_runtime_count += Integer(pool, 3);
              pool_heap_bytes += Integer(pool, 4) * kMiB;
            }
            EXPECT_EQ(row.Number("mts_pool_count"), pool_count);
            EXPECT_EQ(row.Number("mts_pool_runtime_count"), pool_runtime_count);
            EXPECT_EQ(row.Number("mts_pool_heap_bytes_sum"), pool_heap_bytes);
            for (const char* field :
                 {"shared_vm_alive_10m_heap_bytes_sum",
                  "shared_vm_alive_30m_heap_bytes_sum",
                  "shared_vm_exited_page_pending_gc_count",
                  "shared_vm_slot_saturated_count", "shared_vm_idle_count",
                  "shared_vm_heap_supported_count",
                  "shared_vm_heap_collected_count",
                  "shared_vm_tracked_exited_page_count",
                  "shared_vm_untracked_page_count_sum"}) {
              EXPECT_FALSE(row.Has(field)) << field;
            }
            EXPECT_FALSE(row.Has("retained_snapshot_complete"));
            EXPECT_FALSE(row.Has("sampled_leak_page_exit_elapsed_ms"));
            for (auto& event : events) {
              report::EventTracker::OnGlobalEvent(
                  [event = std::move(event)](auto& target) mutable {
                    target = std::move(event);
                  });
            }
            ++session_snapshots;
          }
        }
        for (size_t i = 0; i < monitors.size(); ++i) {
          if (!monitors[i]) continue;
          if (session["oom"].GetBool()) {
            // Process death cannot deliver an unfinished page's end summary.
            monitors[i]->summary_published_ = true;
          }
          global.OnInstanceDestroyed(static_cast<int32_t>(i + 1), epoch + stop);
          monitors[i]->SetExitTime(epoch + stop);
          monitors[i].reset();
        }
        js_runner->PostSyncTask([&engines] {
          for (auto& engine : engines) engine.reset();
        });
        for (size_t i = 0; i < pools.Size(); ++i) {
          global.OnMTSRuntimePoolDestroy(static_cast<int32_t>(100000 + i));
        }
        global.CollectSnapshot({});
        if (global.poll_timer_) global.poll_timer_->Stop();
        EXPECT_TRUE(global.instance_state_.empty());
        EXPECT_TRUE(global.bts_vm_state_.empty());
        EXPECT_TRUE(global.mts_runtime_pool_state_.empty());
        report::EventTracker::Flush(1);
      });
      OnReporter([] {});
      const auto id = session["id"].GetInt(), ram = session["ram"].GetInt();
      for (const auto& event : report::EventTrackerWaitableEvent::stack_)
        analytics.Consume(event, id, ram);
      replayed_snapshots += session_snapshots;
    }
    // Verify the Q1..Q11 query contract documented in memory_monitor.h.
    // These invoke ASSERT/EXPECT checks, not just XML output.
    analytics.VerifyPageMemory(oracle);
    analytics.VerifyFixedAgeMemory(oracle);
    // This verifier includes Q3, Q5.3 VM residuals and Q9.1 reuse.
    analytics.VerifyResidentPages(oracle);
    analytics.VerifyConcurrentPageKeys(oracle);
    analytics.VerifyOldVms(oracle);
    analytics.VerifyResidualPages(oracle);
    analytics.VerifyByteTime(oracle);
    analytics.VerifyPssRatios(oracle);
    analytics.VerifySlotOverflow(oracle);
    analytics.VerifyReservedSlots();
    analytics.VerifyMtsPools(oracle);
    // Coverage gates below only prove the fixture exercised these cohorts.
    // Their actual metric values are checked by the named verifiers above.
    EXPECT_EQ(replayed_snapshots, data["stats"]["snapshots"].GetUint64());
    EXPECT_GT(analytics.global_rows, 1000u);
    EXPECT_GT(analytics.short_valid, 0u);
    EXPECT_GT(analytics.overflow, 0u);
    if (std::string(platform) == "ios") {
      // Q2: unmeasurable JSC BTS must not hide the measurable MTS subset.
      ASSERT_TRUE(analytics.values["page_mts_5s"].count("app/shell|jsc"));
      EXPECT_GT(analytics.values["page_mts_5s"].at("app/shell|jsc").size(), 0u);
    }
    // Output only: RecordProperty is NOT an assertion. In particular Q8's
    // ratio P99 is checked by VerifyPssRatios -> VerifyDistribution above.
    // The generator never reads this XML to tune its truth or error bounds.
    for (const auto& metric : analytics.sums) {
      for (const auto& entry : metric.second) {
        auto key =
            std::string(platform) + "_" + metric.first + "_" + entry.first;
        std::replace(key.begin(), key.end(), '/', '_');
        RecordProperty(key, std::to_string(entry.second));
      }
    }
    for (const auto& entry : analytics.values["ratios"]) {
      const auto& values = entry.second;
      auto key = std::string(platform) + "_ratio_p99_" + entry.first;
      std::replace(key.begin(), key.end(), '/', '_');
      RecordProperty(key, std::to_string(simulation::Percentile(values, .99)));
    }
    RecordProperty(std::string(platform) + "_page_events",
                   std::to_string(analytics.page_rows));
    RecordProperty(std::string(platform) + "_global_events",
                   std::to_string(analytics.global_rows));
  }

  void RunReservedSlotQueries() {
    OnReporter([] {
      GlobalMemoryMonitor global;
      global.random_.seed(20260916);
      GlobalMemoryMonitor::Snapshot snapshot;
      snapshot.started_at_ms = 1000;
      snapshot.trigger.reasons = GlobalMemoryMonitor::kTimer;
      // Three different category leaders plus a large V8 competitor. All four
      // measurable heaps and JSC must enter the uniform VM draw.
      const size_t heaps[] = {100, 200, 400, 900, 0};
      const size_t reserved[][3] = {{30, 4, 0}, {1, 8, 0}, {2, 6, 300}};
      const char* names[] = {"unknown_owner", "common_owner", "overflow_owner",
                             "runtime_manager/v8", "jsc"};
      std::vector<std::shared_ptr<TestVM>> engines;
      for (size_t i = 0; i < 5; ++i) {
        const auto type = i < 3    ? runtime::js::JSRuntimeType::quickjs
                          : i == 3 ? runtime::js::JSRuntimeType::v8
                                   : runtime::js::JSRuntimeType::jsc;
        engines.push_back(std::make_shared<TestVM>(type));
        auto& vm = AddVM(global);
        vm.SetName(names[i]);
        vm.type = type;
        vm.vm_ptr = engines.back().get();
        vm.vm = engines.back();
        BtsMemorySample sample;
        sample.heap_valid = i != 4;
        sample.heap_size = heaps[i];
        sample.slots_valid = i < 3;
        if (i < 3) {
          for (size_t slot = 0; slot < 3; ++slot)
            sample.slots[slot] = reserved[i][slot];
          vm.allocated_slots = i == 2 ? 253 : 3;
          vm.overflowed = i == 2;
        }
        global.OnBtsVMMemoryUpdate(engines.back().get(), sample);
      }
      std::array<std::array<std::vector<double>, 3>, 3> distributions;
      size_t selected[5] = {};
      constexpr size_t draws = 8192;
      const char* fields[] = {"vm_unknown_bytes", "vm_common_bytes",
                              "vm_overflow_bytes"};
      for (size_t draw = 0; draw < draws; ++draw) {
        const auto event = BuildEvent(global, snapshot, 1000, 2000,
                                      "lynxsdk_memory_vm_quickjs");
        UniqueKeys(event);
        const simulation::Row row(event);
        EXPECT_EQ(row.Text("report_kind"), "sample");
        const auto name = row.Text("vm_name");
        size_t owner = 0;
        while (owner < 3 && name != names[owner]) ++owner;
        ASSERT_LT(owner, 3u);
        ++selected[owner];
        EXPECT_EQ(row.Number("vm_heap_bytes"), heaps[owner]);
        EXPECT_FALSE(row.Has("vm_slot_selection_weight"));
        for (size_t slot = 0; slot < 3; ++slot) {
          const std::string weighted = std::string(fields[slot]) + "_weighted";
          EXPECT_EQ(row.Number(fields[slot]), reserved[owner][slot]);
          EXPECT_FALSE(row.Has(weighted));
          distributions[owner][slot].push_back(row.Number(fields[slot]));
        }
      }
      for (size_t owner = 0; owner < 3; ++owner) {
        ASSERT_GT(selected[owner], 0u);
        for (size_t slot = 0; slot < 3; ++slot) {
          auto& samples = distributions[owner][slot];
          std::sort(samples.begin(), samples.end());
          for (double percentile : {.50, .90, .95, .99}) {
            EXPECT_EQ(simulation::Percentile(samples, percentile),
                      reserved[owner][slot]);
          }
        }
      }
      // Q10: each raw P95 leaderboard finds its own high-memory VM.
      for (size_t slot = 0; slot < 3; ++slot)
        for (size_t other = 0; other < 3; ++other)
          if (slot != other) {
            EXPECT_GT(simulation::Percentile(distributions[slot][slot], .95),
                      simulation::Percentile(distributions[other][slot], .95));
          }

      global.bts_vm_state_.resize(1);
      auto expect_fields = [&](bool present, double expected) {
        auto events = global.BuildEvents(snapshot, 1000, 2000);
        const report::MoveOnlyEvent* quickjs = nullptr;
        for (const auto& event : events) {
          if (event.GetName() == "lynxsdk_memory_vm_quickjs") {
            quickjs = &event;
          }
        }
        EXPECT_EQ(quickjs != nullptr, present);
        if (!quickjs) return;
        const simulation::Row row(*quickjs);
        EXPECT_FALSE(row.Has("vm_slot_selection_weight"));
        for (const auto* key : fields) {
          EXPECT_TRUE(row.Has(key));
          EXPECT_FALSE(row.Has(std::string(key) + "_weighted"));
          EXPECT_EQ(row.Number(key), expected);
        }
      };
      // A failed slot refresh invalidates old cached numbers instead of
      // re-emitting stale data or manufacturing zeroes. Valid zero is emitted.
      BtsMemorySample sample;
      sample.heap_valid = true;
      sample.heap_size = 100;
      global.OnBtsVMMemoryUpdate(engines[0].get(), sample);
      expect_fields(false, 0);
      sample.slots_valid = true;
      global.OnBtsVMMemoryUpdate(engines[0].get(), sample);
      expect_fields(true, 0);
      global.bts_vm_state_[0]->slot_tracking_enabled = false;
      expect_fields(false, 0);
      global.bts_vm_state_[0]->slot_tracking_enabled = true;
      global.bts_vm_state_[0]->type = runtime::js::JSRuntimeType::v8;
      expect_fields(false, 0);
      global.bts_vm_state_[0]->type = runtime::js::JSRuntimeType::quickjs;
      global.bts_vm_state_[0]->heap_size = 0;
      expect_fields(true, 0);  // Valid zero heaps remain in the VM draw.
      global.OnBtsVMDestroy(engines[0].get());
      expect_fields(false, 0);
    });
  }

  void RunInstanceCreation(bool same_thread) {
    GlobalMemoryMonitor monitor;
    auto engine = std::make_shared<TestVM>(runtime::js::JSRuntimeType::quickjs);
    int callback_count = 0;
    OnReporter([&] {
      auto& vm = AddVM(monitor);
      vm.vm = engine;
      vm.vm_ptr = engine.get();
      vm.type = runtime::js::JSRuntimeType::quickjs;
      vm.slot_tracking_enabled = true;
    });
    auto create = [&] {
      monitor.OnInstanceCreated(
          1, 100, [&, expected = std::make_unique<bool>(false)](auto& state) {
            EXPECT_TRUE(report::EventTrackerPlatformImpl::GetReportTaskRunner()
                            ->RunsTasksOnCurrentThread());
            EXPECT_EQ(monitor.GetInstanceState(1), &state);
            EXPECT_EQ(state.created_at_ms, 100);
            ++callback_count;
            state.bts_expected = *expected;
            state.SetUrl("app/item?query=1#fragment");
            state.bts_vm = engine.get();
            state.slot = 3;
          });
    };
    if (same_thread) {
      OnReporter([&] {
        create();
        EXPECT_EQ(callback_count, 1);
      });
    } else {
      create();
    }
    OnReporter([&] {
      ASSERT_EQ(callback_count, 1);
      ASSERT_EQ(monitor.instance_state_.size(), 1u);
      const auto& state = monitor.instance_state_.at(1);
      const auto& vm = *monitor.bts_vm_state_.front();
      EXPECT_FALSE(state.bts_expected);
      EXPECT_EQ(state.url(), "app/item?query=1#fragment");
      EXPECT_EQ(vm.active_page_count, 1u);
      EXPECT_EQ(vm.total_page_count, 1u);

      monitor.OnInstanceCreated(1, 200, [&](auto& existing) {
        ++callback_count;
        existing.bts_expected = true;
        existing.bts_vm = engine.get();
      });
      EXPECT_EQ(callback_count, 2);
      EXPECT_EQ(state.created_at_ms, 100);
      EXPECT_TRUE(state.bts_expected);
      EXPECT_EQ(vm.active_page_count, 1u);
      EXPECT_EQ(vm.total_page_count, 1u);

      monitor.OnInstanceCreated(-1, 200, [](auto&) {
        ADD_FAILURE() << "Invalid instance ID must not invoke the callback";
      });
      EXPECT_EQ(monitor.instance_state_.size(), 1u);
      monitor.OnInstanceDestroyed(1, 300);
      ASSERT_TRUE(state.destroyed);
      monitor.OnInstanceCreated(1, 400, [](auto&) {
        ADD_FAILURE() << "Destroyed instance must not invoke the callback";
      });
      EXPECT_EQ(state.created_at_ms, 100);
      EXPECT_EQ(state.exited_at_ms, 300);
      EXPECT_EQ(vm.active_page_count, 0u);
      EXPECT_EQ(vm.total_page_count, 1u);
      monitor.OnBtsVMDestroy(engine.get());
      EXPECT_TRUE(monitor.instance_state_.empty());
      monitor.poll_timer_->Stop();
    });
  }

  void RunPageCache() {
    OnReporter([] {
      GlobalMemoryMonitor monitor;
      Populate(monitor);
      MemoryMonitor page(nullptr, base::LogContext{}, 1);
      page.global_ = &monitor;
      FillRecords(page);
      monitor.instance_state_.at(1).memory_monitor = &page;
      EXPECT_EQ(page.last_usage_.total_bytes, 100);
      EXPECT_EQ(monitor.instance_state_.at(1).memory_usage, 100u);
      EXPECT_EQ(monitor.GetInstanceBtsMemoryUsage(1),
                std::numeric_limits<size_t>::max());
      page.AllocateMemory(MemoryRecord("view", 5));
      EXPECT_EQ(page.last_usage_.total_bytes, 100);
      EXPECT_EQ(monitor.instance_state_.at(1).memory_usage, 100u);
      EXPECT_EQ(page.memory_records_.at("view").instance_count_, 2);
      monitor.CollectSnapshot({});
      EXPECT_EQ(page.last_usage_.total_bytes, 105);
      EXPECT_EQ(monitor.instance_state_.at(1).memory_usage, 105u);
      page.DeallocateMemory(MemoryRecord("view", 5));
      EXPECT_EQ(page.last_usage_.total_bytes, 105);
      EXPECT_EQ(page.memory_records_.at("view").instance_count_, 1);
      EXPECT_EQ(page.RefreshMemoryUsage().total_bytes, 100);
      page.UpdateMemoryUsage(MemoryRecord("view", 0), true);
      EXPECT_EQ(page.last_usage_.total_bytes, 100);
      EXPECT_EQ(page.RefreshMemoryUsage().total_bytes, 70);

      page.UpdateMemoryUsage(MemoryRecord("view", 30));
      const auto result = monitor.GetInstanceMemoryUsage(1);
      EXPECT_EQ(result.status, MemoryUsageQueryStatus::kOk);
      EXPECT_EQ(result.page.total_bytes, 100);
      EXPECT_EQ(result.page.element_bytes, 20);
      EXPECT_EQ(result.page.element_count, 1);
      EXPECT_EQ(result.page.mts_bytes, 10);
      EXPECT_EQ(result.page.bts_bytes, 40);
      EXPECT_EQ(result.page.ui_bytes, 30);
      EXPECT_FALSE(result.page.bts_shared);
      EXPECT_EQ(result.bts_heap_bytes, 40);

      auto expect_status = [&](int32_t id, MemoryUsageQueryStatus status) {
        const auto invalid_result = monitor.GetInstanceMemoryUsage(id);
        EXPECT_EQ(invalid_result.status, status);
        EXPECT_EQ(invalid_result.page.total_bytes, 0);
        EXPECT_EQ(invalid_result.bts_heap_bytes, 0);
      };
      expect_status(-1, MemoryUsageQueryStatus::kInvalidInstance);
      expect_status(99, MemoryUsageQueryStatus::kInvalidInstance);
      monitor.instance_state_.at(1).destroyed = true;
      expect_status(1, MemoryUsageQueryStatus::kInvalidInstance);
      monitor.instance_state_.at(1).destroyed = false;

      auto& settings =
          const_cast<MemoryMonitor::Settings&>(MemoryMonitor::GetSettings());
      const auto global_enabled = settings.global.enabled;
      settings.global.enabled = 0;
      expect_status(1, MemoryUsageQueryStatus::kMonitoringDisabled);
      settings.global.enabled = global_enabled;

      auto detail =
          std::make_unique<std::unordered_map<std::string, std::string>>();
      detail->emplace("item", "allocation");
      page.AllocateMemory(MemoryRecord("view", 5, std::move(detail)));
      EXPECT_EQ(page.memory_records_.at("view").detail_->at("item"),
                "allocation");
      detail = std::make_unique<std::unordered_map<std::string, std::string>>();
      detail->emplace("item", "allocation");
      page.DeallocateMemory(MemoryRecord("view", 5, std::move(detail)));
      EXPECT_TRUE(page.memory_records_.at("view").detail_->empty());
    });
  }

  void RunSharedCache() {
    OnReporter([] {
      GlobalMemoryMonitor monitor;
      Populate(monitor);
      auto engine =
          std::make_shared<TestVM>(runtime::js::JSRuntimeType::quickjs);
      auto& vm = AddVM(monitor);
      vm.vm_ptr = engine.get();
      vm.vm = engine;
      vm.type = runtime::js::JSRuntimeType::quickjs;
      vm.slot_tracking_enabled = vm.has_per_instance_data = true;
      vm.memory_slots[3] = 15;
      auto& state = monitor.instance_state_.at(1);
      state.bts_vm = engine.get();
      state.slot = 3;
      MemoryMonitor page(nullptr, base::LogContext{}, 1);
      page.global_ = &monitor;
      FillRecords(page);
      EXPECT_EQ(page.last_usage_.total_bytes, 75);
      EXPECT_EQ(page.last_usage_.bts_bytes, 15);
      EXPECT_EQ(page.memory_records_.at(kCategoryBTSEngine).size_bytes_, 15);
      vm.memory_slots[3] = 25;
      monitor.RefreshPageBts(state, vm);
      EXPECT_EQ(state.memory_usage, 85u);
      EXPECT_EQ(page.memory_records_.at(kCategoryBTSEngine).size_bytes_, 15);
      vm.memory_slots[3] = 0;
      monitor.RefreshPageBts(state, vm);
      EXPECT_EQ(state.memory_usage, 60u);
      state.slot = fml::JSMemoryTrackSlotType::Overflow;
      page.RefreshMemoryUsage();
      EXPECT_EQ(page.last_usage_.total_bytes, 60);
      EXPECT_EQ(page.memory_records_.at(kCategoryBTSEngine).size_bytes_, 0);
      vm.type = runtime::js::JSRuntimeType::v8;
      vm.slot_tracking_enabled = vm.has_per_instance_data = false;
      page.RefreshMemoryUsage();
      EXPECT_EQ(page.last_usage_.total_bytes, 60);
      EXPECT_EQ(page.memory_records_.at(kCategoryBTSEngine).size_bytes_, 0);
    });
  }

  void RunShortSummary() {
    OnReporter([] {
      GlobalMemoryMonitor monitor;
      Populate(monitor);
      MemoryMonitor page(nullptr, base::LogContext{}, 1);
      page.global_ = &monitor;
      FillRecords(page);
      page.anchor_at_ms_ = 1000;
      for (int i = 0; i < 6; ++i) {
        page.ObserveEarly(FullUsage((i + 1) * 100, 40, false), i);
      }
      report::MoveOnlyEvent event;
      page.AddEarlySummary(event, 8500);
      EXPECT_DOUBLE_EQ(Number(event, "early_sample_mean_memory_bytes"), 350);
      EXPECT_EQ(Number(event, "early_sample_peak_memory_bytes"), 600);
      EXPECT_DOUBLE_EQ(Number(event, "early_sample_mean_bts_attributed_bytes"),
                       40);
      EXPECT_EQ(Number(event, "early_sample_peak_bts_attributed_bytes"), 40);
      EXPECT_DOUBLE_EQ(Number(event, "early_sample_mean_mts_heap_bytes"), 10);
      EXPECT_EQ(Number(event, "early_sample_peak_mts_heap_bytes"), 10);
      EXPECT_EQ(event.GetPropsAsMap().count("early_usage_mean_full_bytes"), 0u);
      UniqueKeys(event);
      report::MoveOnlyEvent second;
      page.AddEarlySummary(second, 8500);
      EXPECT_TRUE(second.GetProps().empty());
    });
  }

  void RunEndSummary(bool runtime_first) {
    fml::Thread js_thread("memory-summary-js");
    GlobalMemoryMonitor monitor;
    std::unique_ptr<MemoryMonitor> page;
    auto engine = std::make_shared<TestVM>(runtime::js::JSRuntimeType::quickjs);
    int64_t anchor = 0;
    OnReporter([&] {
      Populate(monitor);
      auto& vm = AddVM(monitor);
      vm.vm_ptr = engine.get();
      vm.vm = engine;
      vm.type = runtime::js::JSRuntimeType::quickjs;
      vm.slot_tracking_enabled = vm.has_per_instance_data = true;
      vm.active_page_count = 1;
      vm.memory_slots[3] = 40;
      auto& state = monitor.instance_state_.at(1);
      state.bts_vm = engine.get();
      state.slot = 3;
      page = std::make_unique<MemoryMonitor>(nullptr, base::LogContext{}, 1);
      page->global_ = &monitor;
      FillRecords(*page);
      page->OnFirstLoadComplete();
      anchor = page->anchor_at_ms_;
      page->sample_timer_->Stop();
      vm.memory_slots[3] = 140;
      page->ReportScheduled(1, anchor + 1000);
      EXPECT_EQ(page->last_usage_.total_bytes, 200);
      monitor.OnInstanceDestroyed(1, anchor + 1500);
    });
    auto destroy_runtime = [&] {
      js_thread.GetTaskRunner()->PostSyncTask([&] {
        // Simulate a teardown GC notification followed by VM destruction.
        BtsMemorySample sample;
        sample.heap_valid = sample.slots_valid = true;
        sample.measured_at_ms = MemoryNowMs();
        monitor.OnBtsVMMemoryUpdate(engine.get(), sample);
        monitor.OnBtsVMDestroy(engine.get());
        engine.reset();
      });
    };
    auto destroy_monitor = [&] {
      OnReporter([&] {
        if (runtime_first) EXPECT_TRUE(monitor.instance_state_.empty());
        // Late producer values and even cleared records cannot alter the two
        // stored observations. A null global pointer proves no destructor read.
        page->UpdateMemoryUsage(MemoryRecord(kCategoryBTSEngine, 0));
        page->memory_records_.clear();
        page->SetExitTime(anchor + 1500);
        page->global_ = nullptr;
        page.reset();
        report::EventTracker::Flush(1);
      });
    };
    if (runtime_first) {
      destroy_runtime();
      destroy_monitor();
    } else {
      destroy_monitor();
      destroy_runtime();
    }
    OnReporter([&] {
      EXPECT_TRUE(monitor.instance_state_.empty());
      EXPECT_TRUE(monitor.bts_vm_state_.empty());
    });
    const auto& events = report::EventTrackerWaitableEvent::stack_;
    ASSERT_EQ(events.size(), 3u);
    EXPECT_EQ(Number(events[0], "current_page_memory_bytes"), 100);
    EXPECT_EQ(Number(events[1], "current_page_memory_bytes"), 200);
    const auto& event = events.back();
    const auto props = event.GetPropsAsMap();
    EXPECT_EQ(event.GetName(), "lynxsdk_performance_entry_memory_v2");
    EXPECT_EQ(event.GetInstanceId(), 1);
    EXPECT_EQ(props.at("page_key").GetStringValue(), "app/item");
    EXPECT_EQ(props.at("bts_runtime_type").GetStringValue(), "quickjs");
    EXPECT_EQ(props.at("report_kind").GetStringValue(), "early_summary");
    CompactSchema(event);
    EXPECT_EQ(Number(event, "has_early_usage_summary"), 1);
    EXPECT_EQ(Number(event, "early_sample_mean_memory_bytes"), 150);
    EXPECT_EQ(Number(event, "early_sample_mean_bts_attributed_bytes"), 90);
    EXPECT_EQ(Number(event, "early_sample_peak_bts_attributed_bytes"), 140);
    EXPECT_EQ(Number(event, "early_sample_mean_mts_heap_bytes"), 10);
    EXPECT_EQ(Number(event, "early_sample_peak_mts_heap_bytes"), 10);
    EXPECT_EQ(Number(event, "early_window_target_duration_ms"), 1500);
    for (const char* absent :
         {"current_page_accounted_bytes", "current_bts_attributed_bytes",
          "scheduled_offset_ms", "actual_offset_ms", "sample_status",
          "exit_snapshot_kind", "early_usage_mean_full_bytes"}) {
      EXPECT_EQ(props.count(absent), 0u) << absent;
    }
    for (const auto& item : events) {
      UniqueKeys(item);
      CompactSchema(item);
    }
  }

  void RunSingleSampleSummary() {
    for (int64_t age : {0, 999, 1000, 1500, 3000, 8000}) {
      SCOPED_TRACE(age);
      OnReporter([age] {
        GlobalMemoryMonitor monitor;
        Populate(monitor);
        MemoryMonitor page(nullptr, base::LogContext{}, 1);
        page.global_ = &monitor;
        FillRecords(page);
        page.OnFirstLoadComplete();
        page.sample_timer_->Stop();
        page.SetExitTime(page.anchor_at_ms_ + age);
        monitor.OnInstanceDestroyed(1, page.exit_at_ms_);
        EXPECT_TRUE(monitor.instance_state_.empty());
        // A queued timer after cleanup must neither sample nor reschedule.
        page.ReportScheduled(1, page.anchor_at_ms_ + 1000);
        EXPECT_EQ(page.next_sample_, kPageMemoryOffsetsMs.size());
        page.global_ = nullptr;
      });
      OnReporter([] { report::EventTracker::Flush(1); });
      OnReporter([] {});
      const auto& events = report::EventTrackerWaitableEvent::stack_;
      ASSERT_EQ(events.size(), 2u);
      const auto& summary = events.back();
      const bool valid = age < 1000;
      if (valid) {
        EXPECT_EQ(Number(summary, "early_sample_mean_memory_bytes"), 100);
        EXPECT_EQ(Number(summary, "early_sample_peak_memory_bytes"), 100);
        EXPECT_EQ(Number(summary, "early_sample_mean_bts_attributed_bytes"),
                  40);
        EXPECT_EQ(Number(summary, "early_sample_peak_bts_attributed_bytes"),
                  40);
        EXPECT_EQ(Number(summary, "early_sample_mean_mts_heap_bytes"), 10);
        EXPECT_EQ(Number(summary, "early_sample_peak_mts_heap_bytes"), 10);
      } else {
        EXPECT_EQ(
            summary.GetPropsAsMap().count("early_sample_mean_memory_bytes"),
            0u);
        EXPECT_EQ(
            summary.GetPropsAsMap().count("early_sample_peak_memory_bytes"),
            0u);
        EXPECT_EQ(summary.GetPropsAsMap().count(
                      "early_sample_mean_bts_attributed_bytes"),
                  0u);
        EXPECT_EQ(summary.GetPropsAsMap().count(
                      "early_sample_peak_bts_attributed_bytes"),
                  0u);
        EXPECT_EQ(
            summary.GetPropsAsMap().count("early_sample_mean_mts_heap_bytes"),
            0u);
        EXPECT_EQ(
            summary.GetPropsAsMap().count("early_sample_peak_mts_heap_bytes"),
            0u);
      }
    }
  }

  void RunNoAnchor() {
    OnReporter([] {
      GlobalMemoryMonitor monitor;
      Populate(monitor);
      MemoryMonitor page(nullptr, base::LogContext{}, 1);
      page.global_ = &monitor;
      FillRecords(page);
      monitor.OnInstanceDestroyed(1, MemoryNowMs());
      page.OnFirstLoadComplete();
      EXPECT_EQ(page.anchor_at_ms_, -1);
      page.global_ = nullptr;
    });
    OnReporter([] { report::EventTracker::Flush(1); });
    OnReporter([] {});
    EXPECT_TRUE(report::EventTrackerWaitableEvent::stack_.empty());
  }

  void RunPartialRecordSummary() {
    OnReporter([] {
      GlobalMemoryMonitor monitor;
      Populate(monitor);
      MemoryMonitor page(nullptr, base::LogContext{}, 1);
      page.global_ = &monitor;
      page.UpdateMemoryUsage(MemoryRecord(kCategoryTasmElement, 20));
      page.UpdateMemoryUsage(MemoryRecord(kCategoryMTSEngine, 10));
      page.UpdateMemoryUsage(MemoryRecord(kCategoryBTSEngine, 40));
      page.OnFirstLoadComplete();
      page.sample_timer_->Stop();
      page.UpdateMemoryUsage(MemoryRecord("view", 30));
      page.ReportScheduled(1, page.anchor_at_ms_ + 1000);
      page.SetExitTime(page.anchor_at_ms_ + 1500);
      monitor.OnInstanceDestroyed(1, page.exit_at_ms_);
    });
    OnReporter([] { report::EventTracker::Flush(1); });
    OnReporter([] {});
    const auto& events = report::EventTrackerWaitableEvent::stack_;
    ASSERT_EQ(events.size(), 3u);
    const auto& summary = events.back();
    EXPECT_EQ(Number(summary, "early_sample_mean_memory_bytes"), 85);
    EXPECT_EQ(Number(summary, "early_sample_peak_memory_bytes"), 100);
    EXPECT_EQ(Number(summary, "early_sample_mean_bts_attributed_bytes"), 40);
    EXPECT_EQ(Number(summary, "early_sample_peak_bts_attributed_bytes"), 40);
    EXPECT_EQ(Number(summary, "early_sample_mean_mts_heap_bytes"), 10);
    EXPECT_EQ(Number(summary, "early_sample_peak_mts_heap_bytes"), 10);
  }

  void RunScheduledAndExit() {
    OnReporter([] {
      GlobalMemoryMonitor monitor;
      Populate(monitor);
      auto page =
          std::make_unique<MemoryMonitor>(nullptr, base::LogContext{}, 1);
      page->global_ = &monitor;
      FillRecords(*page);
      page->OnFirstLoadComplete();
      const auto anchor = page->anchor_at_ms_;
      page->OnFirstLoadComplete();
      EXPECT_EQ(page->anchor_at_ms_, anchor);
      page->sample_timer_->Stop();
      for (size_t i = 1; i < kPageMemoryOffsetsMs.size(); ++i) {
        // A 1s spike belongs to Q1.2; larger late values must not enter early.
        page->UpdateMemoryUsage(
            MemoryRecord("view", i >= kEarlyMemorySampleCount ? 1000
                                 : i == 1                     ? 390
                                                              : 30));
        page->ReportScheduled(i, anchor + kPageMemoryOffsetsMs[i]);
        EXPECT_EQ(page->summary_published_, i >= kEarlyMemorySampleCount - 1);
      }
      EXPECT_TRUE(page->summary_published_);
      // Publication at 20s consumes the budget; destruction adds no event.
      monitor.OnInstanceDestroyed(1, anchor + kPageMemoryOffsetsMs.back() + 1);
      page->SetExitTime(anchor + kPageMemoryOffsetsMs.back() + 1);
      page.reset();
      EXPECT_TRUE(monitor.instance_state_.empty());
      report::EventTracker::Flush(1);
    });
    OnReporter([] {});
    const auto& events = report::EventTrackerWaitableEvent::stack_;
    ASSERT_EQ(events.size(), kPageMemoryOffsetsMs.size());
    size_t summaries = 0;
    for (const auto& event : events) {
      EXPECT_EQ(event.GetName(), "lynxsdk_performance_entry_memory_v2");
      EXPECT_EQ(event.GetInstanceId(), 1);
      summaries += Number(event, "has_early_usage_summary") == 1;
      EXPECT_EQ(event.GetPropsAsMap().count("app_foreground"), 0u);
      EXPECT_EQ(event.GetPropsAsMap().count("early_window_had_background"), 0u);
      EXPECT_EQ(event.GetPropsAsMap().count("exit_snapshot_kind"), 0u);
      CompactSchema(event);
      EXPECT_EQ(event.GetPropsAsMap().at("report_kind").GetStringValue(),
                "scheduled");
      UniqueKeys(event);
    }
    EXPECT_EQ(summaries, 1u);
    EXPECT_EQ(Number(events.back(), "current_page_memory_bytes"), 1070);
    const auto& summary = events[kEarlyMemorySampleCount - 1];
    EXPECT_EQ(summary.GetProps().size(), 22u);
    EXPECT_EQ(Number(summary, "viewSizeBytes"), 30);
    EXPECT_EQ(Number(summary, "early_window_target_duration_ms"), 20000);
    EXPECT_EQ(Number(summary, "early_sample_mean_memory_bytes"), 140);
    EXPECT_EQ(Number(summary, "early_sample_peak_memory_bytes"), 460);
    EXPECT_EQ(Number(summary, "early_sample_mean_bts_attributed_bytes"), 40);
    EXPECT_EQ(Number(summary, "early_sample_peak_bts_attributed_bytes"), 40);
    EXPECT_EQ(Number(summary, "early_sample_mean_mts_heap_bytes"), 10);
    EXPECT_EQ(Number(summary, "early_sample_peak_mts_heap_bytes"), 10);
    const std::array<int64_t, 15> expected_offsets = {
        0,     1000,  2000,  3000,   5000,   8000,   12000,  16000,
        20000, 40000, 60000, 120000, 300000, 600000, 1800000};
    ASSERT_EQ(events.size(), expected_offsets.size());
    for (size_t i = 0; i < events.size(); ++i) {
      EXPECT_EQ(Number(events[i], "scheduled_offset_ms"), expected_offsets[i]);
      EXPECT_EQ(
          events[i].GetPropsAsMap().count("early_sample_peak_memory_bytes"),
          i == kEarlyMemorySampleCount - 1 ? 1u : 0u);
    }
  }

  void RunDelayedSamples() {
    for (int64_t lateness : {1000, 1001}) {
      SCOPED_TRACE(lateness);
      OnReporter([lateness] {
        GlobalMemoryMonitor monitor;
        Populate(monitor);
        MemoryMonitor page(nullptr, base::LogContext{}, 1);
        page.global_ = &monitor;
        FillRecords(page);
        page.anchor_at_ms_ = 1000;
        for (size_t i = 0; i < kEarlyMemorySampleCount; ++i) {
          page.ReportScheduled(
              i, page.anchor_at_ms_ + kPageMemoryOffsetsMs[i] +
                     (kPageMemoryOffsetsMs[i] == 5000 ? lateness : 0));
        }
        EXPECT_EQ(page.early_.scheduled_mask,
                  (1u << kEarlyMemorySampleCount) - 1);
        page.SetExitTime(page.anchor_at_ms_ + kEarlyMemoryWindowMs + 1000);
      });
      OnReporter([] { report::EventTracker::Flush(1); });
      OnReporter([] {});
      const auto& events = report::EventTrackerWaitableEvent::stack_;
      ASSERT_EQ(events.size(), kEarlyMemorySampleCount);
      EXPECT_EQ(events[1].GetPropsAsMap().count("current_page_memory_bytes"),
                1u);
      const auto& summary = events.back();
      EXPECT_EQ(Number(summary, "early_sample_mean_memory_bytes"), 100);
      EXPECT_EQ(Number(summary, "early_sample_mean_bts_attributed_bytes"), 40);
      EXPECT_EQ(Number(summary, "early_sample_peak_bts_attributed_bytes"), 40);
      EXPECT_EQ(Number(summary, "early_sample_mean_mts_heap_bytes"), 10);
      EXPECT_EQ(Number(summary, "early_sample_peak_mts_heap_bytes"), 10);
      CompactSchema(summary);
      EXPECT_EQ(Number(events.back(), "has_early_usage_summary"), 1);
    }
  }

  void RunPageKeys() {
    OnReporter([] {
      GlobalMemoryMonitor monitor;
      const std::string raw_a =
          "demo://"
          "open?actual_url=https%3A%2F%2Fexample.test%2Fa&from=home#part";
      const std::string raw_b =
          "demo://"
          "open?actual_url=https%3A%2F%2Fexample.test%2Fb&from=search#part";
      const std::string shared_key = "feature/detail?variant=1#content";
      const std::string urls[] = {raw_a, raw_b, raw_a, raw_a, raw_b, "", raw_b};
      auto& extras =
          report::EventTrackerWaitableEvent::extra_params_by_instance_;
      extras[1]["page_id"] = "lower_priority";
      extras[4]["page_id"] = "ignored_by_empty_generic";
      extras[5]["page_id"] = "extra_only";
      std::vector<std::unique_ptr<MemoryMonitor>> pages(8);
      for (int32_t id = 1; id <= 7; ++id) {
        if (id <= 4) {
          const auto key = id <= 2 ? shared_key : id == 3 ? "different" : "";
          report::EventTrackerPlatformImpl::UpdateGenericInfo(id, "page_id",
                                                              key);
        } else if (id == 7) {
          report::EventTrackerPlatformImpl::UpdateGenericInfo(
              id, "page_id", std::string("unknown"));
        }
        monitor.OnInstanceCreated(id, 1000, [&](auto& state) {
          state.SetUrl(urls[id - 1]);
          state.mts_known = state.bts_known = true;
          state.mts_context_type = runtime::LepusNGContextType;
          state.bts_runtime_type = runtime::js::JSRuntimeType::quickjs;
          state.usage = FullUsage(100, 40, false);
        });
        pages[id] =
            std::make_unique<MemoryMonitor>(nullptr, base::LogContext{}, id);
        pages[id]->global_ = &monitor;
        FillRecords(*pages[id]);
        pages[id]->OnFirstLoadComplete();
        pages[id]->sample_timer_->Stop();
        if (id != 1) pages[id]->summary_published_ = true;
        EXPECT_EQ(
            report::EventTrackerWaitableEvent::query_count_by_instance_[id],
            1u);
      }
      monitor.poll_timer_->Stop();
      monitor.random_.seed(42);
      GlobalMemoryMonitor::Snapshot snapshot;
      snapshot.started_at_ms = 4000;
      const std::map<std::string, int> expected = {
          {shared_key, 2},   {"different", 1}, {raw_a, 1},
          {"extra_only", 1}, {"", 1},          {raw_b, 1}};
      std::set<std::string> selected;
      for (int i = 0; i < 128; ++i) {
        const auto event = BuildEvent(monitor, snapshot, 4000, 1000);
        CompactSchema(event);
        const simulation::Row row(event);
        const auto key = row.Text("page_key");
        ASSERT_TRUE(expected.count(key));
        selected.insert(key);
        const int n = expected.at(key);
        EXPECT_EQ(row.Number("page_group_instance_count"), n);
        EXPECT_EQ(row.Number("page_group_memory_bytes"), n * 100);
        EXPECT_DOUBLE_EQ(row.Number("page_group_selection_weight"), 7.0 / n);
        EXPECT_EQ(row.Number("active_page_count"), 7);
      }
      EXPECT_EQ(selected.size(), expected.size());
      const std::string keys[] = {shared_key,   shared_key, "different", raw_a,
                                  "extra_only", "",         raw_b};
      for (int32_t id = 1; id <= 7; ++id) {
        EXPECT_EQ(monitor.instance_state_.at(id).url(), urls[id - 1]);
        EXPECT_TRUE(monitor.instance_state_.at(id).page_key_resolved);
        MemoryMonitor observation(nullptr, base::LogContext{}, id);
        const auto event =
            observation.BuildIdentityEvent(monitor.instance_state_.at(id));
        CompactSchema(event);
        EXPECT_EQ(simulation::Row(event).Text("page_key"), keys[id - 1]);
        EXPECT_EQ(
            report::EventTrackerWaitableEvent::query_count_by_instance_[id],
            1u);
      }

      auto& page = *pages[1];
      const auto anchor = page.anchor_at_ms_;
      page.OnFirstLoadComplete();
      EXPECT_EQ(report::EventTrackerWaitableEvent::query_count_by_instance_[1],
                1u);
      EXPECT_EQ(simulation::Row(page.summary_identity_).Text("page_key"),
                shared_key);
      // The first-load identity is immutable. Later URL or generic-info changes
      // do not trigger another platform query or change the summary cohort.
      monitor.WithInstance(1, [&](auto& state) { state.SetUrl(raw_b); });
      page.ReportScheduled(1, anchor + 1000);
      report::EventTrackerPlatformImpl::UpdateGenericInfo(
          1, "page_id", std::string("late_identity"));
      page.ReportScheduled(2, anchor + 2000);
      EXPECT_EQ(monitor.instance_state_.at(1).page_key, shared_key);
      EXPECT_EQ(report::EventTrackerWaitableEvent::query_count_by_instance_[1],
                1u);
      EXPECT_EQ(simulation::Row(page.summary_identity_).Text("page_key"),
                shared_key);
      page.SetExitTime(anchor + 2500);

      // A confirmed residual keeps the last observed identity after the host
      // clears its metadata; neither grouping nor teardown may reread it.
      auto engine =
          std::make_shared<TestVM>(runtime::js::JSRuntimeType::quickjs);
      monitor.OnBtsVMCreate("group?name=original#suffix", "", engine);
      EXPECT_EQ(monitor.FindVM(engine.get())->vm_name(),
                "group?name=original#suffix");
      monitor.WithInstance(3, [&](auto& state) {
        state.bts_vm = engine.get();
        state.slot = 3;
      });
      report::EventTrackerPlatformImpl::ClearCache(3);
      monitor.OnInstanceDestroyed(3, 2000);
      BtsMemorySample sample;
      sample.heap_valid = sample.slots_valid = sample.is_full_gc = true;
      sample.heap_size = sample.slots[3] = 12;
      sample.measured_at_ms = 2001;
      monitor.OnBtsVMMemoryUpdate(engine.get(), sample);
      const auto event = BuildEvent(monitor, snapshot, 4000, 1000);
      CompactSchema(event);
      EXPECT_EQ(simulation::Row(event).Text("leak_page_key"), "different");
      EXPECT_EQ(report::EventTrackerWaitableEvent::query_count_by_instance_[3],
                1u);
      monitor.OnBtsVMDestroy(engine.get());
    });
  }

  void RunExitPageKeyBeforeFirstReport() {
    OnReporter([] {
      GlobalMemoryMonitor monitor;
      auto engine =
          std::make_shared<TestVM>(runtime::js::JSRuntimeType::quickjs);
      monitor.OnBtsVMCreate("shared", "", engine);
      monitor.poll_timer_->Stop();
      monitor.OnInstanceCreated(1, 1000, [&](auto& state) {
        state.SetUrl(
            "demo://open?url=https://example.test/kept?secret=1&outer=2");
        state.bts_vm = engine.get();
        state.slot = 3;
      });
      report::EventTrackerPlatformImpl::UpdateGenericInfo(
          1, "page_id", std::string("exit_before_report"));
      GlobalMemoryMonitor::Snapshot snapshot;
      snapshot.started_at_ms = 1500;
      const auto event = BuildEvent(monitor, snapshot, 1500, 1000);
      EXPECT_EQ(simulation::Row(event).Text("page_key"),
                "https://example.test/kept");
      EXPECT_EQ(report::EventTrackerWaitableEvent::query_count_by_instance_[1],
                0u);
      monitor.OnInstanceDestroyed(1, 2000);
      report::EventTrackerPlatformImpl::ClearCache(1);
      ASSERT_NE(monitor.GetInstanceState(1), nullptr);
      EXPECT_EQ(monitor.GetInstanceState(1)->page_key,
                "https://example.test/kept");
      EXPECT_FALSE(monitor.GetInstanceState(1)->page_key_resolved);
      EXPECT_EQ(report::EventTrackerWaitableEvent::query_count_by_instance_[1],
                0u);
      EXPECT_EQ(monitor.ResolveInstancePageKey(999), nullptr);
      EXPECT_EQ(monitor.ResolveInstancePageKey(-1), nullptr);
      monitor.OnBtsVMDestroy(engine.get());
    });
  }

  void RunGlobalEvent() {
    GlobalMemoryMonitor monitor;
    GlobalMemoryMonitor::Snapshot snapshot;
    snapshot.started_at_ms = 4000000;
    snapshot.trigger.reasons = GlobalMemoryMonitor::kTimer;
    auto engine = std::make_shared<TestVM>(runtime::js::JSRuntimeType::quickjs);
    BtsVmState vm;
    vm.vm_ptr = engine.get();
    vm.vm = engine;
    vm.SetName("shared");
    vm.type = runtime::js::JSRuntimeType::quickjs;
    vm.created_at_ms = 100;
    vm.heap_valid = true;
    vm.heap_size = 200;
    vm.has_per_instance_data = vm.slot_tracking_enabled = true;
    vm.memory_slots[3] = 40;
    vm.memory_slots[4] = 20;
    vm.full_gc_slots = vm.memory_slots;
    vm.full_gc_at_ms = 3995000;
    monitor.bts_vm_state_.push_back(std::make_unique<BtsVmState>(vm));
    InstanceState active;
    active.created_at_ms = 100;
    active.SetUrl("app/item");
    active.bts_vm = engine.get();
    active.slot = 3;
    active.usage = FullUsage(100, 40, true);
    active.memory_usage = 100;
    monitor.instance_state_.emplace(1, active);
    active.destroyed = true;
    active.slot = 4;
    active.exited_at_ms = 3990000;
    monitor.instance_state_.emplace(2, active);
    monitor.previous_snapshot_at_ms_ = 3900000;
    auto event = BuildEvent(monitor, snapshot, 4000100, 500);
    EXPECT_EQ(event.GetName(), "memory_test_snapshot");
    EXPECT_EQ(event.GetInstanceId(), report::kUnknownInstanceId);
    EXPECT_EQ(Number(event, "lynx_accounted_memory_bytes"), 260);
    EXPECT_EQ(Number(event, "active_page_count"), 1);
    EXPECT_EQ(Number(event, "shared_vm_tracked_exited_page_retained_bytes"),
              20);
    EXPECT_EQ(Number(event, "process_pss_bytes"), 500);
    EXPECT_DOUBLE_EQ(Number(event, "lynx_accounted_process_pss_ratio"), 0.52);
    EXPECT_DOUBLE_EQ(Number(event, "lynx_shared_vm_process_pss_ratio"), 0.4);
    EXPECT_DOUBLE_EQ(
        Number(event, "lynx_tracked_page_retained_process_pss_ratio"), 0.04);
    for (const auto* key : {"lynx_accounted_process_rss_ratio",
                            "lynx_shared_vm_process_rss_ratio",
                            "lynx_tracked_page_retained_process_rss_ratio",
                            "sampled_leak_page_attributed_rss_ratio"}) {
      EXPECT_EQ(event.GetPropsAsMap().count(key), 0u);
    }
    EXPECT_EQ(Number(event, "leak_page_selection_weight"), 1);
    EXPECT_EQ(Number(event, "leak_page_attributed_byte_ms"), 200000);
    EXPECT_EQ(Number(event, "shared_vm_heap_bytes_sum"), 200);
    EXPECT_EQ(Number(event, "vm_selection_weight"), 1);
    EXPECT_EQ(event.GetPropsAsMap().count("lynx_full_memory_snapshot_valid"),
              0u);
    EXPECT_GT(Number(event, "vm_age_ms"), 1800000);
    EXPECT_EQ(event.GetPropsAsMap().count("trigger_vm_present"), 0u);
    UniqueKeys(event);
    CompactSchema(event);
    const auto split_events = monitor.BuildEvents(snapshot, 4000100, 500);
    const report::MoveOnlyEvent* residual_event = nullptr;
    for (const auto& candidate : split_events) {
      for (const auto& prop : candidate.GetProps()) {
        EXPECT_NE(prop.GetKey().find("sampled_"), 0u) << candidate.GetName();
      }
      if (candidate.GetName() == "lynxsdk_memory_page_residual") {
        residual_event = &candidate;
      }
    }
    ASSERT_NE(residual_event, nullptr);
    const auto residual_props = residual_event->GetPropsAsMap();
    EXPECT_EQ(residual_props.size(), 4u);
    for (const char* field : {"schema_version", "leak_page_key",
                              "shared_vm_tracked_exited_page_retained_bytes",
                              "leak_page_selection_weight"}) {
      EXPECT_EQ(residual_props.count(field), 1u) << field;
    }
    for (const auto invalid : {int64_t{-1}, int64_t{0}}) {
      auto events = monitor.BuildEvents(snapshot, 4000100, invalid);
      for (const auto& candidate : events) {
        EXPECT_NE(candidate.GetName(), "lynxsdk_memory_process");
      }
    }
    auto over_one = BuildEvent(monitor, snapshot, 4000100, 100);
    EXPECT_DOUBLE_EQ(Number(over_one, "lynx_accounted_process_pss_ratio"), 2.6);
  }

  void RunPageKeyGroup() {
    GlobalMemoryMonitor monitor;
    GlobalMemoryMonitor::Snapshot snapshot;
    snapshot.started_at_ms = 1000;
    for (int i = 0; i < 3; ++i) {
      InstanceState page;
      page.SetUrl(i < 2 ? "app/items" : "app/other");
      page.usage = FullUsage(i < 2 ? 100 : 300, 40, false);
      monitor.instance_state_.emplace(i, page);
    }
    for (int i = 0; i < 20; ++i) {
      auto event = BuildEvent(monitor, snapshot, 1000, 1000);
      const auto n = Number(event, "page_group_instance_count");
      const auto sum = Number(event, "page_group_memory_bytes");
      EXPECT_DOUBLE_EQ(Number(event, "page_group_selection_weight"), 3 / n);
      EXPECT_DOUBLE_EQ(Number(event, "page_group_memory_bytes_weighted"),
                       sum * 3 / n);
      EXPECT_EQ(Number(event, "active_page_count"), 3);
      EXPECT_DOUBLE_EQ(n * Number(event, "page_group_selection_weight"),
                       Number(event, "active_page_count"));
      EXPECT_DOUBLE_EQ(Number(event, "page_memory_bytes_weighted"),
                       Number(event, "page_memory_bytes") * 3);
      UniqueKeys(event);
    }
  }

  void RunTriggerEventRouting() {
    OnReporter([] {
      GlobalMemoryMonitor monitor;
      Populate(monitor);
      auto engine =
          std::make_shared<TestVM>(runtime::js::JSRuntimeType::quickjs);
      auto& vm = AddVM(monitor);
      vm.vm = engine;
      vm.vm_ptr = engine.get();
      vm.type = runtime::js::JSRuntimeType::quickjs;
      vm.heap_valid = true;
      vm.slot_tracking_enabled = vm.has_per_instance_data = true;
      vm.SetName("quickjs");
      GlobalMemoryMonitor::Snapshot snapshot;
      snapshot.started_at_ms = 1000;
      snapshot.trigger.reasons = GlobalMemoryMonitor::kTimer |
                                 GlobalMemoryMonitor::kPss |
                                 GlobalMemoryMonitor::kOverflow;
      snapshot.trigger.pss_before = 256;
      snapshot.trigger.pss_after = 1024;
      snapshot.trigger.overflow_vm = std::make_unique<BtsVmState>();
      auto& overflow = *snapshot.trigger.overflow_vm;
      overflow.SetName("overflow");
      overflow.created_at_ms = 100;
      overflow.heap_size = 200;
      overflow.total_page_count = 254;
      overflow.untracked_page_count = 1;
      overflow.allocated_slots = 253;
      auto events = monitor.BuildEvents(snapshot, 1000, 1000);
      std::set<std::string> names;
      std::set<std::string> quickjs_kinds;
      for (const auto& event : events) {
        names.insert(event.GetName());
        EXPECT_EQ(event.GetPropsAsMap().count("trigger_reasons"), 0u);
        if (event.GetName() == "lynxsdk_memory_vm_quickjs") {
          const simulation::Row row(event);
          const auto kind = row.Text("report_kind");
          quickjs_kinds.insert(kind);
          EXPECT_TRUE(row.Has("vm_name"));
          EXPECT_TRUE(row.Has("vm_age_ms"));
          EXPECT_TRUE(row.Has("vm_total_page_count"));
          EXPECT_TRUE(row.Has("vm_slot_allocated_count"));
          EXPECT_TRUE(row.Has("vm_untracked_page_count"));
          EXPECT_TRUE(row.Has("vm_heap_bytes"));
          EXPECT_EQ(event.GetProps().size(), kind == "sample" ? 11u : 8u);
          EXPECT_FALSE(row.Has("vm_slot_selection_weight"));
          for (const auto& prop : event.GetProps()) {
            EXPECT_NE(prop.GetKey().find("sampled_"), 0u);
            EXPECT_NE(prop.GetKey().find("trigger_"), 0u);
          }
        }
      }
      EXPECT_EQ(names,
                (std::set<std::string>{
                    "lynxsdk_memory_process", "lynxsdk_memory_page",
                    "lynxsdk_memory_vm_stat", "lynxsdk_memory_vm_quickjs"}));
      EXPECT_EQ(quickjs_kinds,
                (std::set<std::string>{"sample", "slot_overflow"}));
      EXPECT_EQ(events.size(), 5u);
      for (const auto& event : events) {
        if (event.GetName() == "lynxsdk_memory_process") {
          EXPECT_EQ(Number(event, "trigger_pss_previous_milestone_bytes"), 256);
          EXPECT_EQ(Number(event, "trigger_pss_milestone_bytes"), 1024);
        } else {
          EXPECT_EQ(event.GetPropsAsMap().count("trigger_pss_milestone_bytes"),
                    0u);
        }
      }
    });
  }

  void RunPpsCensusReuse() {
    OnReporter([] {
      GlobalMemoryMonitor monitor;
      monitor.random_.seed(42);
      monitor.previous_snapshot_at_ms_ = 3900000;
      GlobalMemoryMonitor::Snapshot snapshot;
      snapshot.started_at_ms = 4000000;
      snapshot.trigger.reasons = GlobalMemoryMonitor::kTimer;
      std::vector<std::shared_ptr<TestVM>> engines;
      for (int i = 0; i < 2; ++i) {
        auto engine = std::make_shared<TestVM>(
            i == 0 ? runtime::js::JSRuntimeType::quickjs
                   : runtime::js::JSRuntimeType::v8);
        auto& vm = AddVM(monitor);
        vm.vm_ptr = engine.get();
        vm.vm = engine;
        vm.type = engine->GetRuntimeType();
        vm.SetName(i == 0 ? "old" : "young");
        vm.heap_valid = true;
        vm.heap_size = i == 0 ? 900 : 100;
        vm.created_at_ms = i == 0 ? 100 : 3950000;
        if (i == 0) {
          vm.slot_tracking_enabled = vm.has_per_instance_data = true;
          vm.allocated_slots = 253;
          vm.untracked_page_count = 2;
          vm.memory_slots[3] = 90;
          vm.memory_slots[4] = 10;
          vm.full_gc_slots = vm.memory_slots;
          vm.full_gc_at_ms = 3995000;
          for (int id = 0; id < 2; ++id) {
            auto& page = monitor.instance_state_[id];
            page.SetUrl(id == 0 ? "old/page" : "young/page");
            page.destroyed = true;
            page.bts_vm = engine.get();
            page.slot = 3 + id;
            page.exited_at_ms = id == 0 ? 3000000 : 3990000;
          }
        }
        engines.push_back(std::move(engine));
      }
      std::unordered_set<std::string> vm_names, urls;
      double expected_old_heap = 0;
      double expected_residual = 0, expected_residual_weight = 0;
      for (int i = 0; i < 100; ++i) {
        const auto event = BuildEvent(monitor, snapshot, 4000000, 2000);
        CompactSchema(event);
        UniqueKeys(event);
        const auto props = event.GetPropsAsMap();
        const auto name = props.at("vm_name").GetStringValue();
        const auto url = props.at("leak_page_key").GetStringValue();
        const double heap = Number(event, "vm_heap_bytes_weighted");
        const double retained =
            Number(event, "shared_vm_tracked_exited_page_retained_bytes");
        const double vm_probability = 1 / Number(event, "vm_selection_weight");
        const double slot_probability = url == "old/page" ? 0.9 : 0.1;
        const double slot_weight = Number(event, "leak_page_selection_weight");
        EXPECT_EQ(Number(event, "shared_vm_heap_bytes_sum"), 1000);
        EXPECT_EQ(heap, name == "old" ? 1800 : 200);
        EXPECT_EQ(retained, 100);
        EXPECT_DOUBLE_EQ(slot_weight, url == "old/page" ? 100.0 / 90 : 10);
        EXPECT_DOUBLE_EQ(Number(event, "vm_heap_bytes") / vm_probability, heap);
        EXPECT_DOUBLE_EQ(retained / slot_weight, url == "old/page" ? 90 : 10);
        EXPECT_EQ(Number(event, "vm_attributed_heap_byte_ms"),
                  heap * (name == "old" ? 100000 : 50000));
        EXPECT_EQ(Number(event, "leak_page_attributed_byte_ms"),
                  retained * (url == "old/page" ? 100000 : 10000));
        if (vm_names.insert(name).second && name == "old")
          expected_old_heap += vm_probability * heap;
        if (urls.insert(url).second) {
          // Evaluate each possible draw once with its analytical probability.
          expected_residual += slot_probability * retained;
          expected_residual_weight += slot_probability * slot_weight;
        }
      }
      ASSERT_EQ(vm_names.size(), 2u);
      ASSERT_EQ(urls.size(), 2u);
      EXPECT_DOUBLE_EQ(expected_old_heap, 900);
      EXPECT_DOUBLE_EQ(expected_residual_weight, 2);
      EXPECT_DOUBLE_EQ(expected_residual / expected_residual_weight, 50);
    });
  }

  void RunEngineCapabilities() {
    GlobalMemoryMonitor monitor;
    GlobalMemoryMonitor::Snapshot snapshot;
    snapshot.started_at_ms = 2000000;
    for (auto type :
         {runtime::js::JSRuntimeType::v8, runtime::js::JSRuntimeType::jsvm,
          runtime::js::JSRuntimeType::jsc}) {
      BtsVmState vm;
      vm.type = type;
      vm.SetName(GlobalMemoryMonitor::RuntimeName(type));
      vm.created_at_ms = 1;
      vm.heap_size = 500;
      vm.heap_valid = type != runtime::js::JSRuntimeType::jsc;
      monitor.bts_vm_state_.push_back(std::make_unique<BtsVmState>(vm));
    }
    auto event = BuildEvent(monitor, snapshot, 2000000, 2000);
    EXPECT_EQ(Number(event, "shared_vm_count"), 3);
    EXPECT_EQ(event.GetPropsAsMap().count("shared_vm_heap_supported_count"),
              0u);
    EXPECT_EQ(Number(event, "shared_vm_heap_bytes_sum"), 1000);
    EXPECT_EQ(event.GetPropsAsMap().count("lynx_full_memory_snapshot_valid"),
              0u);
    EXPECT_EQ(event.GetPropsAsMap().count("vm_heap_snapshot_complete"), 0u);
    EXPECT_EQ(Number(event, "vm_selection_weight"), 3);
    const simulation::Row row(event);
    if (row.Text("vm_runtime_type") == "jsc") {
      EXPECT_FALSE(row.Has("vm_heap_bytes"));
    } else {
      EXPECT_DOUBLE_EQ(Number(event, "vm_heap_bytes_weighted"), 1500);
      EXPECT_EQ(Number(event, "vm_heap_selection_weight"), 3);
    }
  }

  void RunPostExitFullGcResiduals() {
    OnReporter([] {
      // Delivery order can differ across the shell and JS threads. Run both
      // orders using the same capture/exit times, including a same-ms tie.
      for (bool gc_delivered_first : {false, true}) {
        for (int64_t captured : {1999, 2000, 2001}) {
          SCOPED_TRACE(gc_delivered_first);
          SCOPED_TRACE(captured);
          GlobalMemoryMonitor monitor;
          auto engine =
              std::make_shared<TestVM>(runtime::js::JSRuntimeType::quickjs);
          monitor.OnBtsVMCreate("shared", "", engine);
          monitor.poll_timer_->Stop();
          monitor.OnInstanceCreated(1, 1000, [&](auto& page) {
            page.SetUrl("exited");
            page.bts_vm = engine.get();
            page.slot = 3;
          });
          monitor.OnInstanceCreated(2, 1000, [&](auto& page) {
            page.SetUrl("active");
            page.bts_vm = engine.get();
            page.slot = 4;
          });
          BtsMemorySample sample;
          sample.heap_valid = sample.slots_valid = true;
          sample.heap_size = 200;
          sample.slots[3] = 90;
          sample.slots[4] = 60;
          sample.slots[1] = 50;
          sample.measured_at_ms = 1500;
          monitor.OnBtsVMMemoryUpdate(engine.get(), sample);
          auto check = [&](double bytes, bool area_expected) {
            GlobalMemoryMonitor::Snapshot snapshot;
            snapshot.started_at_ms = 4000;
            monitor.previous_snapshot_at_ms_ = 3000;
            const simulation::Row row(BuildEvent(monitor, snapshot, 4000, 500));
            EXPECT_EQ(
                row.Number("shared_vm_tracked_exited_page_retained_bytes"),
                bytes);
            EXPECT_FALSE(row.Has("retained_snapshot_complete"));
            EXPECT_FALSE(row.Has("sampled_leak_page_exit_elapsed_ms"));
            EXPECT_EQ(row.Has("leak_page_key"), bytes > 0);
            EXPECT_EQ(row.Has("leak_page_selection_weight"), bytes > 0);
            EXPECT_FALSE(row.Has("sampled_leak_page_selection_probability"));
            if (bytes > 0) {
              EXPECT_EQ(row.Text("leak_page_key"), "exited");
              EXPECT_EQ(row.Number("leak_page_selection_weight"), 1);
              EXPECT_EQ(row.Has("leak_page_attributed_byte_ms"), area_expected);
            }
          };
          if (!gc_delivered_first) {
            monitor.OnInstanceDestroyed(1, 2000);
            check(0, false);
          }
          sample.is_full_gc = true;
          sample.measured_at_ms = captured;
          sample.slots[3] = 12;
          monitor.OnBtsVMMemoryUpdate(engine.get(), sample);
          if (gc_delivered_first) monitor.OnInstanceDestroyed(1, 2000);
          const bool eligible = captured > 2000;
          check(eligible ? 12 : 0, eligible);

          // Ordinary and invalid full-GC callbacks cannot certify or overwrite
          // residuals, while current active-page and reserved values advance.
          sample.is_full_gc = false;
          sample.measured_at_ms = 2100;
          sample.slots[3] = 120;
          sample.slots[4] = 70;
          sample.slots[1] = 10;
          monitor.OnBtsVMMemoryUpdate(engine.get(), sample);
          check(eligible ? 12 : 0, eligible);
          EXPECT_EQ(monitor.GetInstanceBtsMemoryUsage(2), 70u);
          EXPECT_EQ(monitor.FindVM(engine.get())->memory_slots[1], 10u);
          sample.is_full_gc = true;
          sample.slots_valid = false;
          sample.measured_at_ms = 2200;
          monitor.OnBtsVMMemoryUpdate(engine.get(), sample);
          check(eligible ? 12 : 0, false);
          sample.slots_valid = true;
          sample.heap_valid = false;
          monitor.OnBtsVMMemoryUpdate(engine.get(), sample);
          check(eligible ? 12 : 0, eligible);

          // A later valid full GC replaces the residual; a delayed older GC
          // cannot roll it back. Zero is confirmed data, not pending/unknown.
          sample.heap_valid = true;
          sample.measured_at_ms = 2500;
          sample.slots[3] = 4;
          monitor.OnBtsVMMemoryUpdate(engine.get(), sample);
          check(4, true);
          sample.measured_at_ms = 2300;
          sample.slots[3] = 80;
          monitor.OnBtsVMMemoryUpdate(engine.get(), sample);
          check(4, true);
          sample.measured_at_ms = 2600;
          sample.slots[3] = 0;
          monitor.OnBtsVMMemoryUpdate(engine.get(), sample);
          check(0, true);
          sample.measured_at_ms = 2700;
          sample.slots[3] = 6;
          monitor.OnBtsVMMemoryUpdate(engine.get(), sample);
          // Active slot 4 was seen by this GC but exits later: it must wait.
          // Only slot 3 may be selected, even though pending slot 4 is larger.
          monitor.OnInstanceDestroyed(2, 2800);
          check(6, false);
          monitor.OnBtsVMDestroy(engine.get());
          check(0, true);
          EXPECT_TRUE(monitor.instance_state_.empty());
        }
      }
    });
  }

  void RunLifetimeAndSlots() {
    OnReporter([] {
      GlobalMemoryMonitor monitor;
      Populate(monitor);
      auto engine =
          std::make_shared<TestVM>(runtime::js::JSRuntimeType::quickjs);
      auto& vm = AddVM(monitor);
      vm.vm_ptr = engine.get();
      vm.vm = engine;
      vm.type = runtime::js::JSRuntimeType::quickjs;
      for (int slot = 3; slot < 256; ++slot)
        monitor.OnBtsVMSlotAllocate(engine.get(), slot);
      EXPECT_EQ(vm.allocated_slots, 253u);
      EXPECT_FALSE(vm.overflowed);
      monitor.OnBtsVMSlotAllocate(engine.get(), -1);
      EXPECT_TRUE(vm.overflowed);
      EXPECT_EQ(vm.untracked_page_count, 1u);
      auto& page = monitor.instance_state_.at(1);
      page.bts_vm = engine.get();
      page.slot = 3;
      page.usage = FullUsage(100, 40, true);
      vm.active_page_count = 1;
      monitor.OnInstanceDestroyed(1, 2000);
      monitor.OnInstanceDestroyed(1, 3000);
      EXPECT_EQ(vm.active_page_count, 0u);
      EXPECT_EQ(vm.idle_since_ms, 2000);
      EXPECT_EQ(monitor.instance_state_.size(), 1u);
      EXPECT_EQ(monitor.instance_state_.at(1).exited_at_ms, 2000);
      // Exit only changes lifecycle metadata; it does not sample the slots.
      EXPECT_EQ(monitor.instance_state_.at(1).usage.total_bytes, 100);
      BtsMemorySample sample;
      sample.slots_valid = true;
      sample.slots[3] = 12;
      monitor.OnBtsVMMemoryUpdate(engine.get(), sample);
      EXPECT_EQ(monitor.GetInstanceBtsMemoryUsage(1), 12u);
      monitor.OnBtsVMDestroy(engine.get());
      EXPECT_TRUE(monitor.instance_state_.empty());
      bool called = false;
      monitor.WithInstance(1, [&called](auto&) { called = true; });
      EXPECT_FALSE(called);
      EXPECT_TRUE(monitor.instance_state_.empty());
    });
  }

  void RunSynchronousHeapCollection() {
    fml::Thread js_thread("memory-monitor-js");
    const auto runner = js_thread.GetTaskRunner();
    OnReporter([&] {
      GlobalMemoryMonitor monitor;
      int reads = 0;
      std::vector<std::shared_ptr<TestVM>> engines;
      const runtime::js::JSRuntimeType types[] = {
          runtime::js::JSRuntimeType::v8, runtime::js::JSRuntimeType::jsvm,
          runtime::js::JSRuntimeType::quickjs, runtime::js::JSRuntimeType::jsc};
      for (auto type : types) {
        auto engine = std::make_shared<TestVM>(type);
        if (type == runtime::js::JSRuntimeType::jsvm) engine->heap_size = 0;
        engine->on_heap_read = [&, type] {
          EXPECT_TRUE(runner->RunsTasksOnCurrentThread());
          EXPECT_TRUE(type == runtime::js::JSRuntimeType::v8 ||
                      type == runtime::js::JSRuntimeType::jsvm);
          ++reads;
        };
        auto& vm = AddVM(monitor);
        vm.vm = engine;
        vm.vm_ptr = engine.get();
        vm.type = type;
        vm.js_thread_runner = runner;
        vm.heap_size = 125;
        vm.heap_valid = type != runtime::js::JSRuntimeType::jsc;
        vm.measured_at_ms = 1;
        engines.push_back(std::move(engine));
      }
      Populate(monitor);
      auto snapshot = monitor.CollectSnapshot({});
      const auto& vms = monitor.bts_vm_state_;
      ASSERT_EQ(vms.size(), 4u);
      EXPECT_FALSE(snapshot.partial);
      EXPECT_EQ(reads, 2);
      EXPECT_EQ(vms[0]->heap_size, 500u);
      EXPECT_EQ(vms[1]->heap_size, 0u);
      EXPECT_TRUE(vms[1]->heap_valid);
      EXPECT_GT(vms[0]->measured_at_ms, 1);
      EXPECT_EQ(vms[2]->heap_size, 125u);
      EXPECT_EQ(vms[2]->measured_at_ms, 1);
      EXPECT_FALSE(vms[3]->heap_valid);
      auto event = BuildEvent(monitor, snapshot, MemoryNowMs(), 1000);
      EXPECT_EQ(Number(event, "shared_vm_heap_bytes_sum"), 625);
      EXPECT_EQ(event.GetPropsAsMap().count("snapshot_membership_changed"), 0u);
      EXPECT_EQ(
          event.GetPropsAsMap().count("sampled_active_url_membership_changed"),
          0u);
      UniqueKeys(event);
      // Test engine ownership follows the same owner-thread constraint.
      runner->PostSyncTask(
          [engines = std::move(engines)]() mutable { engines.clear(); });
    });
  }

  void RunHeapCollectionTimeout() {
    fml::Thread js_thread("memory-monitor-blocked-js");
    const auto runner = js_thread.GetTaskRunner();
    fml::AutoResetWaitableEvent task_started;
    fml::AutoResetWaitableEvent unblock_task;
    runner->PostTask([&] {
      task_started.Signal();
      unblock_task.Wait();
    });
    task_started.Wait();

    GlobalMemoryMonitor monitor;
    auto engine = std::make_shared<TestVM>(runtime::js::JSRuntimeType::v8);
    OnReporter([&] {
      auto& vm = AddVM(monitor);
      vm.vm = engine;
      vm.vm_ptr = engine.get();
      vm.type = runtime::js::JSRuntimeType::v8;
      vm.js_thread_runner = runner;
      vm.heap_valid = true;
      vm.heap_size = 125;
      vm.measured_at_ms = 1;

      auto snapshot = monitor.CollectSnapshot({});

      EXPECT_TRUE(snapshot.partial);
      EXPECT_EQ(snapshot.failed_heap_vms,
                std::vector<runtime::js::VMInstance*>({engine.get()}));
      EXPECT_EQ(vm.heap_size, 125u);
      EXPECT_EQ(vm.measured_at_ms, 1);
    });

    unblock_task.Signal();
    runner->PostSyncTask([] {});
    OnReporter([&] {
      const auto& vm = *monitor.bts_vm_state_[0];
      EXPECT_EQ(vm.heap_size, 125u);
      EXPECT_EQ(vm.measured_at_ms, 1);
      monitor.bts_vm_state_.clear();
    });
    runner->PostSyncTask(
        [engine = std::move(engine)]() mutable { engine.reset(); });
  }

  void RunReporterUpdatesWait() {
    fml::Thread js_thread("memory-monitor-js");
    const auto runner = js_thread.GetTaskRunner();
    GlobalMemoryMonitor monitor;
    std::atomic<bool> updated{false};
    std::atomic<bool> destroyed_on_js{false};
    report::MoveOnlyEvent event;
    OnReporter([&] {
      Populate(monitor);
      auto engine = std::make_shared<TestVM>(runtime::js::JSRuntimeType::v8);
      engine->on_destroy = [&] {
        destroyed_on_js = runner->RunsTasksOnCurrentThread();
      };
      engine->on_heap_read = [&] {
        EXPECT_TRUE(runner->RunsTasksOnCurrentThread());
        monitor.WithInstance(1, [&](auto& page) {
          page.SetUrl("app/updated");
          updated = true;
        });
        EXPECT_FALSE(updated);
        // The temporary weak_ptr lock must keep the VM alive until the query
        // completes, and must release its last strong reference on JS.
        engine.reset();
        EXPECT_FALSE(destroyed_on_js);
      };
      auto& vm = AddVM(monitor);
      vm.vm = engine;
      vm.vm_ptr = engine.get();
      vm.type = runtime::js::JSRuntimeType::v8;
      vm.js_thread_runner = runner;
      auto snapshot = monitor.CollectSnapshot({});
      EXPECT_FALSE(snapshot.partial);
      EXPECT_EQ(monitor.bts_vm_state_[0]->heap_size, 500u);
      EXPECT_TRUE(destroyed_on_js);
      EXPECT_FALSE(updated);
      EXPECT_EQ(monitor.instance_state_.at(1).url(), "app/item");
      event = BuildEvent(monitor, snapshot, MemoryNowMs(), 1000);
      EXPECT_EQ(event.GetPropsAsMap().at("page_key").GetStringValue(),
                "app/item");
    });
    // The queued mutation executes only after the entire reporter task above.
    OnReporter([&] {
      EXPECT_TRUE(updated);
      EXPECT_EQ(monitor.instance_state_.at(1).url(), "app/updated");
      EXPECT_EQ(event.GetPropsAsMap().at("page_key").GetStringValue(),
                "app/item");
    });
  }

  void RunMissingHeapRunner() {
    OnReporter([] {
      for (bool cached : {false, true}) {
        GlobalMemoryMonitor monitor;
        auto engine = std::make_shared<TestVM>(runtime::js::JSRuntimeType::v8);
        engine->on_heap_read = [] {
          ADD_FAILURE() << "Missing runner must not query";
        };
        auto& vm = AddVM(monitor);
        vm.vm = engine;
        vm.vm_ptr = engine.get();
        vm.type = runtime::js::JSRuntimeType::v8;
        vm.heap_valid = cached;
        vm.heap_size = cached ? 75 : 0;
        vm.measured_at_ms = 1;
        auto snapshot = monitor.CollectSnapshot({});
        ASSERT_EQ(monitor.bts_vm_state_.size(), 1u);
        EXPECT_TRUE(snapshot.partial);
        EXPECT_EQ(vm.heap_valid, cached);
        EXPECT_EQ(vm.heap_size, cached ? 75u : 0u);
        EXPECT_EQ(vm.measured_at_ms, 1);
        auto event = BuildEvent(monitor, snapshot, MemoryNowMs(), 1000,
                                "lynxsdk_memory_vm_stat");
        EXPECT_EQ(Number(event, "vm_selection_weight"), 1);
        EXPECT_FALSE(simulation::Row(event).Has("vm_heap_selection_weight"));
        const auto events = monitor.BuildEvents(snapshot, MemoryNowMs(), 1000);
        for (const auto& candidate : events) {
          EXPECT_NE(candidate.GetName(), "lynxsdk_memory_process");
        }
      }
    });
  }

  void RunExpiredHeapWhileWaiting() {
    fml::Thread js_thread("memory-monitor-js");
    const auto runner = js_thread.GetTaskRunner();
    OnReporter([&] {
      GlobalMemoryMonitor monitor;
      auto first = std::make_shared<TestVM>(runtime::js::JSRuntimeType::v8);
      auto second = std::make_shared<TestVM>(runtime::js::JSRuntimeType::jsvm);
      first->on_heap_read = [&] { second.reset(); };
      for (const auto& engine : {first, second}) {
        auto& vm = AddVM(monitor);
        vm.vm = engine;
        vm.vm_ptr = engine.get();
        vm.type = engine->GetRuntimeType();
        vm.js_thread_runner = runner;
        vm.heap_valid = true;
        vm.heap_size = 75;
        vm.measured_at_ms = 1;
      }
      // The second VM was live at snapshot start, but dies before its query.
      auto snapshot = monitor.CollectSnapshot({});
      ASSERT_EQ(monitor.bts_vm_state_.size(), 2u);
      EXPECT_TRUE(snapshot.partial);
      EXPECT_EQ(monitor.bts_vm_state_[0]->heap_size, 500u);
      EXPECT_EQ(monitor.bts_vm_state_[1]->heap_size, 75u);
      EXPECT_EQ(monitor.bts_vm_state_[1]->measured_at_ms, 1);
      auto event = BuildEvent(monitor, snapshot, MemoryNowMs(), 1000,
                              "lynxsdk_memory_vm_stat");
      const simulation::Row row(event);
      EXPECT_EQ(Number(event, "vm_selection_weight"), 2);
      if (row.Text("vm_runtime_type") == "v8") {
        EXPECT_EQ(Number(event, "vm_heap_bytes"), 500);
      } else {
        EXPECT_FALSE(row.Has("vm_heap_bytes"));
      }
      const auto events = monitor.BuildEvents(snapshot, MemoryNowMs(), 1000);
      for (const auto& candidate : events) {
        EXPECT_NE(candidate.GetName(), "lynxsdk_memory_process");
      }
      // On the next collection it is already expired, so normal cleanup runs.
      auto next = monitor.CollectSnapshot({});
      EXPECT_EQ(monitor.bts_vm_state_.size(), 1u);
      EXPECT_FALSE(next.partial);
      runner->PostSyncTask(
          [engine = std::move(first)]() mutable { engine.reset(); });
    });
  }

  void RunExpiredVmCleanup() {
    OnReporter([] {
      GlobalMemoryMonitor monitor;
      std::vector<std::shared_ptr<TestVM>> engines;
      for (int i = 0; i < 20; ++i) {
        auto engine =
            std::make_shared<TestVM>(runtime::js::JSRuntimeType::quickjs);
        auto& vm = AddVM(monitor);
        vm.vm = engine;
        vm.vm_ptr = engine.get();
        vm.type = runtime::js::JSRuntimeType::quickjs;
        vm.heap_valid = true;
        vm.heap_size = 125;
        vm.has_per_instance_data = vm.slot_tracking_enabled = true;
        vm.memory_slots[3] = 10;
        vm.full_gc_slots = vm.memory_slots;
        vm.full_gc_at_ms = 1;
        auto& page = monitor.instance_state_[i];
        page.bts_vm = engine.get();
        page.slot = 3;
        page.destroyed = true;
        engines.push_back(std::move(engine));
      }
      // Eighteen expired entries exceed inline capacity, with live entries
      // mixed in.
      for (int i = 0; i < 20; ++i) {
        if (i != 2 && i != 7) engines[i].reset();
      }
      auto snapshot = monitor.CollectSnapshot({});
      EXPECT_FALSE(snapshot.partial);
      ASSERT_EQ(monitor.bts_vm_state_.size(), 2u);
      EXPECT_EQ(monitor.bts_vm_state_[0]->vm_ptr, engines[2].get());
      EXPECT_EQ(monitor.bts_vm_state_[1]->vm_ptr, engines[7].get());
      EXPECT_EQ(monitor.instance_state_.size(), 2u);
      EXPECT_EQ(monitor.instance_state_.count(2), 1u);
      EXPECT_EQ(monitor.instance_state_.count(7), 1u);
      auto event = BuildEvent(monitor, snapshot, MemoryNowMs(), 1000);
      EXPECT_EQ(Number(event, "shared_vm_heap_bytes_sum"), 250);
      EXPECT_EQ(Number(event, "shared_vm_tracked_exited_page_retained_bytes"),
                20);
      engines.clear();
      snapshot = monitor.CollectSnapshot({});
      EXPECT_FALSE(snapshot.partial);
      EXPECT_TRUE(monitor.bts_vm_state_.empty());
      EXPECT_TRUE(monitor.instance_state_.empty());
      event = BuildEvent(monitor, snapshot, MemoryNowMs(), 1000);
      EXPECT_EQ(Number(event, "shared_vm_count"), 0);
      EXPECT_EQ(event.GetPropsAsMap().count("vm_name"), 0u);
      EXPECT_EQ(event.GetPropsAsMap().count("leak_page_key"), 0u);
    });
  }

  void RunQueuedSlotOverflowTrigger() {
    GlobalMemoryMonitor monitor;
    OnReporter([&] {
      auto engine =
          std::make_shared<TestVM>(runtime::js::JSRuntimeType::quickjs);
      auto& vm = AddVM(monitor);
      vm.vm = engine;
      vm.vm_ptr = engine.get();
      vm.SetName("original");
      vm.created_at_ms = MemoryNowMs() - 1000;
      vm.heap_size = 123;
      vm.heap_valid = true;
      vm.allocated_slots = 253;
      vm.pending_overflow = vm.overflowed = true;
      vm.untracked_page_count = 1;
      vm.total_page_count = 254;
      monitor.EmitSlotOverflowTrigger(vm);
      EXPECT_FALSE(vm.pending_overflow);
      monitor.EmitSlotOverflowTrigger(vm);  // No new pending transition.
      vm.SetName("changed");
      vm.heap_size = 999;
      vm.total_page_count = 300;
      monitor.OnBtsVMDestroy(engine.get());
    });
    OnReporter([&] {
      ASSERT_NE(monitor.poll_timer_, nullptr);
      monitor.poll_timer_->Stop();
      report::EventTracker::Flush(1);
    });
    OnReporter([] {});
    const auto& events = report::EventTrackerWaitableEvent::stack_;
    const report::MoveOnlyEvent* event = nullptr;
    for (const auto& candidate : events) {
      if (candidate.GetName() == "lynxsdk_memory_vm_quickjs") {
        ASSERT_EQ(event, nullptr);
        event = &candidate;
      }
    }
    ASSERT_NE(event, nullptr);
    CompactSchema(*event);
    UniqueKeys(*event);
    EXPECT_EQ(event->GetPropsAsMap().at("report_kind").GetStringValue(),
              "slot_overflow");
    EXPECT_EQ(event->GetPropsAsMap().count("trigger_vm_present"), 0u);
    EXPECT_EQ(event->GetPropsAsMap().count("trigger_reasons"), 0u);
    EXPECT_EQ(event->GetPropsAsMap().count("trigger_vm_slot_milestone"), 0u);
    EXPECT_EQ(event->GetPropsAsMap().at("vm_name").GetStringValue(),
              "original");
    EXPECT_EQ(Number(*event, "vm_slot_allocated_count"), 253);
    EXPECT_EQ(Number(*event, "vm_total_page_count"), 254);
    EXPECT_EQ(Number(*event, "vm_untracked_page_count"), 1);
    EXPECT_GE(Number(*event, "vm_age_ms"), 1000);
    EXPECT_EQ(Number(*event, "vm_heap_bytes"), 123);
  }

  void RunGlobalQueueAfterSync() {
    fml::Thread js_thread("memory-monitor-js");
    const auto runner = js_thread.GetTaskRunner();
    OnReporter([&] {
      GlobalMemoryMonitor monitor;
      Populate(monitor);
      monitor.UpdatePageMemory(1, FullUsage(100, 40, false));
      auto engine = std::make_shared<TestVM>(runtime::js::JSRuntimeType::v8);
      auto& vm = AddVM(monitor);
      vm.vm = engine;
      vm.vm_ptr = engine.get();
      vm.type = runtime::js::JSRuntimeType::v8;
      vm.js_thread_runner = runner;
      GlobalMemoryMonitor::Trigger trigger;
      trigger.reasons = GlobalMemoryMonitor::kTimer | GlobalMemoryMonitor::kPss;
      trigger.pss_before = 256;
      trigger.pss_after = 1024;
      monitor.RequestReport(std::move(trigger));
      runner->PostSyncTask([&] { engine->heap_size = 900; });
      monitor.UpdatePageMemory(1, FullUsage(200, 40, false));
      monitor.WithInstance(1, [](auto& page) { page.SetUrl("app/updated"); });
      GlobalMemoryMonitor::Trigger warning;
      warning.reasons = GlobalMemoryMonitor::kWarning;
      monitor.RequestReport(std::move(warning));
      monitor.poll_timer_->Stop();
      monitor.bts_vm_state_.clear();
      monitor.instance_state_.clear();
      runner->PostSyncTask(
          [engine = std::move(engine)]() mutable { engine.reset(); });
      report::EventTracker::Flush(1);
    });
    OnReporter([] {});
    const auto& events = report::EventTrackerWaitableEvent::stack_;
    for (const auto& event : events) {
      EXPECT_EQ(event.GetInstanceId(), report::kUnknownInstanceId);
      EXPECT_EQ(event.GetPropsAsMap().count("lynx_full_memory_snapshot_valid"),
                0u);
      EXPECT_EQ(event.GetPropsAsMap().count("snapshot_membership_changed"), 0u);
      EXPECT_EQ(event.GetPropsAsMap().count("process_session_id"), 0u);
      EXPECT_EQ(event.GetPropsAsMap().count("snapshot_seq"), 0u);
      UniqueKeys(event);
      CompactSchema(event);
    }
    const report::MoveOnlyEvent* first_page = nullptr;
    const report::MoveOnlyEvent* first_vm = nullptr;
    const report::MoveOnlyEvent* second_process = nullptr;
    for (const auto& event : events) {
      if (event.GetName() == "lynxsdk_memory_page") {
        first_page = &event;
      } else if (event.GetName() == "lynxsdk_memory_vm_stat") {
        first_vm = &event;
      } else if (event.GetName() == "lynxsdk_memory_process" &&
                 Number(event, "shared_vm_heap_bytes_sum") == 900) {
        second_process = &event;
      }
    }
    ASSERT_NE(first_page, nullptr);
    ASSERT_NE(first_vm, nullptr);
    EXPECT_EQ(Number(*first_page, "page_memory_bytes"), 100);
    EXPECT_EQ(first_page->GetPropsAsMap().at("page_key").GetStringValue(),
              "app/item");
    EXPECT_EQ(Number(*first_vm, "vm_heap_bytes"), 500);
    if (second_process) {
      EXPECT_EQ(Number(*second_process, "shared_vm_heap_bytes_sum"), 900);
      const auto pss = Number(*second_process, "process_pss_bytes");
      EXPECT_DOUBLE_EQ(
          Number(*second_process, "lynx_accounted_process_pss_ratio"),
          Number(*second_process, "lynx_accounted_memory_bytes") / pss);
    }
  }

  void RunUntrackedCleanup() {
    OnReporter([] {
      for (auto type : {runtime::js::JSRuntimeType::v8,
                        runtime::js::JSRuntimeType::quickjs}) {
        GlobalMemoryMonitor monitor;
        Populate(monitor);
        auto engine = std::make_shared<TestVM>(type);
        auto& vm = AddVM(monitor);
        vm.vm_ptr = engine.get();
        vm.vm = engine;
        vm.type = type;
        monitor.instance_state_.at(1).bts_vm = engine.get();
        monitor.OnInstanceDestroyed(1, 2000);
        monitor.OnInstanceDestroyed(1, 3000);
        EXPECT_TRUE(monitor.instance_state_.empty());
      }
      GlobalMemoryMonitor standalone;
      Populate(standalone);
      standalone.instance_state_.at(1).slot = 3;
      standalone.OnInstanceDestroyed(1, 2000);
      EXPECT_TRUE(standalone.instance_state_.empty());
    });
  }

  void RunVMSharingPolicy() {
    OnReporter([] {
      GlobalMemoryMonitor monitor;
      const struct {
        runtime::js::JSRuntimeType type;
        bool across_groups;
        const char* name;
      } cases[] = {
          {runtime::js::JSRuntimeType::v8, true, "runtime_manager/v8"},
          {runtime::js::JSRuntimeType::jsvm, true, "runtime_manager/jsvm"},
          {runtime::js::JSRuntimeType::quickjs, false, "app/group?version=1"},
          {runtime::js::JSRuntimeType::jsc, false, "app/group?version=1"},
      };
      for (const auto& test : cases) {
        SCOPED_TRACE(test.name);
        EXPECT_EQ(runtime::JSRealmManager::IsVMSharedAcrossGroups(test.type),
                  test.across_groups);
        auto engine = std::make_shared<TestVM>(test.type);
        monitor.OnBtsVMCreate("app/group?version=1", "", engine);
        ASSERT_EQ(monitor.bts_vm_state_.size(), 1u);
        const auto& vm = *monitor.bts_vm_state_.front();
        EXPECT_EQ(vm.runtime_manager_scope, test.across_groups);
        EXPECT_EQ(vm.vm_name(), test.name);
        monitor.OnBtsVMDestroy(engine.get());
      }
      monitor.poll_timer_->Stop();
    });
  }

  void RunScheduledTimer() {
    OnReporter([] {
      GlobalMemoryMonitor monitor;
      Populate(monitor);
      MemoryMonitor page(nullptr, base::LogContext{}, 1);
      page.global_ = &monitor;
      FillRecords(page);
      page.OnFirstLoadComplete();
      ASSERT_NE(page.sample_timer_, nullptr);
      EXPECT_EQ(page.early_.attempted_mask, 1u);
      EXPECT_TRUE(page.sample_timer_->FireImmediately());
      EXPECT_EQ(page.early_.attempted_mask, 3u);
      page.sample_timer_->Stop();
      page.SetExitTime(page.anchor_at_ms_ + 1500);
    });
    OnReporter([] { report::EventTracker::Flush(1); });
    OnReporter([] {});
    const auto& events = report::EventTrackerWaitableEvent::stack_;
    ASSERT_EQ(events.size(), 3u);
    EXPECT_EQ(events.front().GetName(), "lynxsdk_performance_entry_memory_v2");
    EXPECT_EQ(events.front().GetPropsAsMap().at("report_kind").GetStringValue(),
              "scheduled");
    EXPECT_EQ(Number(events.front(), "scheduled_offset_ms"), 0);
    EXPECT_EQ(Number(events[1], "scheduled_offset_ms"), 1000);
    EXPECT_EQ(Number(events.back(), "has_early_usage_summary"), 1);
  }

  void RunScheduledDeliverySampling(bool scheduled_enabled, bool short_page,
                                    bool miss_early_points) {
    SCOPED_TRACE(scheduled_enabled);
    OnReporter([=] {
      report::EventTrackerWaitableEvent::stack_.clear();
      auto& settings =
          const_cast<MemoryMonitor::Settings&>(MemoryMonitor::GetSettings());
      const auto saved = settings.page_scheduled.enabled;
      const auto ui_enabled = settings.lynx_ui.enabled;
      settings.page_scheduled.enabled = scheduled_enabled;
      {
        GlobalMemoryMonitor global;
        Populate(global);
        MemoryMonitor page(nullptr, base::LogContext{}, 1);
        page.global_ = &global;
        global.instance_state_.at(1).memory_monitor = &page;
        FillRecords(page);
        page.OnFirstLoadComplete();
        page.sample_timer_->Stop();
        const auto anchor = page.anchor_at_ms_;
        const int64_t duration =
            short_page ? 1500 : kPageMemoryOffsetsMs.back() + 1;
        for (size_t i = 1; i < kPageMemoryOffsetsMs.size(); ++i) {
          if (kPageMemoryOffsetsMs[i] >= duration) break;
          // The first callback after 0s can be delayed past the early window.
          if (miss_early_points && i < kEarlyMemorySampleCount) continue;
          page.UpdateMemoryUsage(MemoryRecord(kCategoryMTSEngine, 10 + i * 10));
          page.UpdateMemoryUsage(MemoryRecord(kCategoryBTSEngine, 40 + i * 20));
          page.ReportScheduled(i, anchor + kPageMemoryOffsetsMs[i]);
          EXPECT_EQ(page.summary_published_, i >= kEarlyMemorySampleCount - 1);
        }
        EXPECT_EQ(page.summary_published_, !short_page);
        const auto sample_count = miss_early_points ? 1 : short_page ? 2 : 9;
        EXPECT_EQ(page.early_.count, sample_count);
        EXPECT_EQ(page.early_bts_.count, sample_count);
        EXPECT_EQ(page.early_mts_.count, sample_count);
        EXPECT_EQ(page.early_.scheduled_mask, (1u << sample_count) - 1);
        EXPECT_TRUE(MemoryMonitor::Enable());
        EXPECT_EQ(settings.lynx_ui.enabled, ui_enabled);
        // The delivery gate must not disable collection or global accounting.
        global.GetGlobalMemoryUsage([=](GlobalMemoryUsage usage) {
          EXPECT_EQ(usage.total.total_bytes, short_page ? 130 : 520);
          EXPECT_EQ(usage.total.mts_bytes, short_page ? 20 : 150);
          EXPECT_EQ(usage.total.bts_bytes, short_page ? 60 : 320);
        });
        page.SetExitTime(anchor + duration);
        global.OnInstanceDestroyed(1, anchor + duration);
        page.ReportScheduled(1, anchor + duration + 1000);
        EXPECT_EQ(page.next_sample_, kPageMemoryOffsetsMs.size());
      }
      settings.page_scheduled.enabled = saved;
    });
    OnReporter([] { report::EventTracker::Flush(1); });
    OnReporter([] {});
    const auto& events = report::EventTrackerWaitableEvent::stack_;
    const size_t expected_count = !scheduled_enabled  ? 1
                                  : short_page        ? 3
                                  : miss_early_points ? 7
                                                      : 15;
    ASSERT_EQ(events.size(), expected_count);
    size_t summaries = 0;
    for (const auto& event : events) {
      UniqueKeys(event);
      EXPECT_EQ(event.GetName(), "lynxsdk_performance_entry_memory_v2");
      if (!scheduled_enabled) {
        EXPECT_EQ(event.GetPropsAsMap().at("report_kind").GetStringValue(),
                  "early_summary");
        EXPECT_EQ(event.GetPropsAsMap().count("scheduled_offset_ms"), 0u);
        EXPECT_EQ(event.GetPropsAsMap().count("current_page_memory_bytes"), 0u);
      }
      if (!Number(event, "has_early_usage_summary")) continue;
      ++summaries;
      EXPECT_EQ(Number(event, "early_window_target_duration_ms"),
                short_page ? 1500 : 20000);
      const struct {
        const char* mean;
        const char* peak;
        double mean_bytes;
        double peak_bytes;
      } expected[] = {
          {"early_sample_mean_memory_bytes", "early_sample_peak_memory_bytes",
           short_page ? 115.0 : 220.0, short_page ? 130.0 : 340.0},
          {"early_sample_mean_bts_attributed_bytes",
           "early_sample_peak_bts_attributed_bytes", short_page ? 50.0 : 120.0,
           short_page ? 60.0 : 200.0},
          {"early_sample_mean_mts_heap_bytes",
           "early_sample_peak_mts_heap_bytes", short_page ? 15.0 : 50.0,
           short_page ? 20.0 : 90.0},
      };
      for (const auto& metric : expected) {
        if (miss_early_points) {
          EXPECT_EQ(event.GetPropsAsMap().count(metric.mean), 0u);
          EXPECT_EQ(event.GetPropsAsMap().count(metric.peak), 0u);
        } else {
          EXPECT_DOUBLE_EQ(Number(event, metric.mean), metric.mean_bytes);
          EXPECT_DOUBLE_EQ(Number(event, metric.peak), metric.peak_bytes);
        }
      }
    }
    EXPECT_EQ(summaries, 1u);
  }

  void RunSamplingSettings() {
    EXPECT_TRUE(MemoryMonitor::Enable());
    const auto& settings = MemoryMonitor::GetSettings();
    for (const auto* entry :
         {static_cast<const MemoryMonitor::Settings::Entry*>(&settings.global),
          &settings.lynx_ui, &settings.page_scheduled}) {
      EXPECT_EQ(entry->enabled, 1u);
      EXPECT_EQ(entry->sample_rate, 1.0f);
    }
    EXPECT_EQ(MemoryMonitor::ScriptingEngineMode(true),
              std::min(255u, settings.global.mts_threshold_mb) << 24);
    EXPECT_EQ(MemoryMonitor::ScriptingEngineMode(false),
              std::min(255u, settings.global.bts_threshold_mb) << 24);
    report::MoveOnlyEvent event;
    MemoryMonitor::AddCommonProps(event);
    EXPECT_EQ(event.GetProps().size(), 2u);
    EXPECT_EQ(Number(event, "lynx_ui_memory_enabled"), 1);
    UniqueKeys(event);
    CompactSchema(event);
  }

  void RunConfigParsing() {
    MemoryMonitor::Settings settings;
    EXPECT_FLOAT_EQ(settings.page_scheduled.sample_rate, 0.1f);
    settings.ApplyConfig(
        "global_sample_rate,0.25,lynx_ui_sample_rate,0.75,"
        "page_scheduled_sample_rate,0.125,"
        "time_interval_sec,10,pss_threshold_mb,2,mts_threshold_mb,3,"
        "bts_threshold_mb,4");
    EXPECT_FLOAT_EQ(settings.global.sample_rate, 0.25f);
    EXPECT_FLOAT_EQ(settings.lynx_ui.sample_rate, 0.75f);
    EXPECT_FLOAT_EQ(settings.page_scheduled.sample_rate, 0.125f);
    EXPECT_EQ(settings.global.time_interval_sec, 10u);
    EXPECT_EQ(settings.global.pss_threshold_mb, 2u);
    EXPECT_EQ(settings.global.mts_threshold_mb, 3u);
    EXPECT_EQ(settings.global.bts_threshold_mb, 4u);

    settings.ApplyConfig("page_scheduled_sample_rate,0");
    EXPECT_FLOAT_EQ(settings.page_scheduled.sample_rate, 0.0f);
    EXPECT_FLOAT_EQ(settings.global.sample_rate, 0.25f);
    EXPECT_FLOAT_EQ(settings.lynx_ui.sample_rate, 0.75f);
    settings.ApplyConfig(
        "page_scheduled_sample_rate,0.5,page_scheduled_sample_rate,1,"
        "page_scheduled_sample_rate,invalid,page_scheduled_sample_rate");
    EXPECT_FLOAT_EQ(settings.page_scheduled.sample_rate, 1.0f);

    settings.ApplyConfig(
        "global_sample_rate,2,lynx_ui_sample_rate,-1,time_interval_sec,-1,"
        "pss_threshold_mb,invalid,unknown_key,9,global.sample_rate,1,"
        "global_sample_rate");
    EXPECT_FLOAT_EQ(settings.global.sample_rate, 2.0f);
    EXPECT_FLOAT_EQ(settings.lynx_ui.sample_rate, -1.0f);
    EXPECT_EQ(settings.global.time_interval_sec,
              std::numeric_limits<uint32_t>::max());
    EXPECT_EQ(settings.global.pss_threshold_mb, 2u);

    settings.ApplyConfig("global_sample_rate,0.5,global_sample_rate,1");
    EXPECT_FLOAT_EQ(settings.global.sample_rate, 1.0f);
  }

#if ENABLE_TRACE_PERFETTO
  void RunPersistentForceEnable() {
    constexpr char kMarkerPath[] = "./.lynx-memory-monitor-force-enabled";
    std::remove(kMarkerPath);
    auto* trace_controller = trace::GetTraceControllerInstance();
    trace_controller->SetDelegate(
        std::make_unique<TestTraceControllerDelegate>("."));

    MemoryMonitor::ForceEnableForTesting(
        MemoryMonitor::ForceEnableMode::kCurrentProcess);
    auto* marker = std::fopen(kMarkerPath, "rb");
    EXPECT_EQ(marker, nullptr);
    if (marker) std::fclose(marker);

    MemoryMonitor::ForceEnableForTesting(
        MemoryMonitor::ForceEnableMode::kPersistent);
    marker = std::fopen(kMarkerPath, "rb");
    EXPECT_NE(marker, nullptr);
    if (marker) std::fclose(marker);

    MemoryMonitor::Settings settings;
    settings.ApplyConfig(
        "global_sample_rate,0,lynx_ui_sample_rate,0,page_scheduled_sample_rate,"
        "0");
    EXPECT_EQ(settings.global.enabled, 1u);
    EXPECT_FLOAT_EQ(settings.global.sample_rate, 1.0f);
    EXPECT_EQ(settings.lynx_ui.enabled, 1u);
    EXPECT_FLOAT_EQ(settings.lynx_ui.sample_rate, 1.0f);
    EXPECT_EQ(settings.page_scheduled.enabled, 1u);
    EXPECT_FLOAT_EQ(settings.page_scheduled.sample_rate, 1.0f);

    std::remove(kMarkerPath);
  }
#endif

  void RunLogContext() {
    MemoryMonitor monitor(nullptr, base::LogContext{});
    EXPECT_EQ(monitor.log_context_.view_id, base::kUnavailableLynxEntityId);
    EXPECT_EQ(monitor.log_context_.engine_id, base::kUnavailableLynxEntityId);
    EXPECT_EQ(monitor.log_context_.runtime_id, base::kUnavailableLynxEntityId);
    monitor.SetLogContext({1, 2, 3});
    EXPECT_EQ(monitor.log_context_.view_id, 1);
    EXPECT_EQ(monitor.log_context_.engine_id, 2);
    EXPECT_EQ(monitor.log_context_.runtime_id, 3);
  }

  void RunRecordUpdates() {
    OnReporter([] {
      TestPerformanceSender sender;
      MemoryMonitor monitor(&sender, base::LogContext{});
      monitor.AllocateMemory(MemoryRecord(kCategoryMTSEngine, 1024));
      monitor.AllocateMemory(MemoryRecord(kCategoryMTSEngine, 512));
      EXPECT_EQ(monitor.memory_records_.at(kCategoryMTSEngine).size_bytes_,
                1536);
      monitor.DeallocateMemory(MemoryRecord(kCategoryMTSEngine, 256));
      EXPECT_EQ(monitor.memory_records_.at(kCategoryMTSEngine).size_bytes_,
                1280);
      monitor.DeallocateMemory(MemoryRecord(kCategoryBTSEngine, 512));
      EXPECT_EQ(monitor.memory_records_.count(kCategoryBTSEngine), 0u);
      monitor.UpdateMemoryUsage(MemoryRecord(kCategoryMTSEngine, 512));
      EXPECT_EQ(monitor.memory_records_.at(kCategoryMTSEngine).size_bytes_,
                512);
      auto details =
          std::make_unique<std::unordered_map<std::string, std::string>>();
      details->emplace("url", "test.com");
      details->emplace("id", "123");
      monitor.UpdateMemoryUsage(
          MemoryRecord(kCategoryBTSEngine, 2048, std::move(details)));
      // No refresh has occurred, so this remains the producer's VM heap value.
      const auto& record = monitor.memory_records_.at(kCategoryBTSEngine);
      EXPECT_EQ(record.size_bytes_, 2048);
      ASSERT_NE(record.detail_, nullptr);
      EXPECT_EQ(record.detail_->at("url"), "test.com");
      EXPECT_EQ(record.detail_->at("id"), "123");
    });
    OnReporter([] { report::EventTracker::Flush(1); });
    OnReporter([] {});
    EXPECT_TRUE(report::EventTrackerWaitableEvent::stack_.empty());
  }

  void RunMissedDeadlineAndExit() {
    OnReporter([] {
      GlobalMemoryMonitor monitor;
      Populate(monitor);
      MemoryMonitor page(nullptr, base::LogContext{}, 1);
      page.global_ = &monitor;
      FillRecords(page);
      page.anchor_at_ms_ = 1000;
      page.ReportScheduled(0, 1000);
      page.ReportScheduled(1, 22000);
      EXPECT_TRUE(page.summary_published_);
      monitor.OnInstanceDestroyed(1, 23000);
      page.ReportScheduled(kEarlyMemorySampleCount, 41000);
      page.SetExitTime(23000);
    });
    OnReporter([] { report::EventTracker::Flush(1); });
    OnReporter([] {});
    const auto& events = report::EventTrackerWaitableEvent::stack_;
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(Number(events[1], "has_early_usage_summary"), 1);
    EXPECT_EQ(events[1].GetPropsAsMap().count("early_sample_mean_memory_bytes"),
              0u);
    EXPECT_EQ(events[1].GetPropsAsMap().count("early_sample_peak_memory_bytes"),
              0u);
    EXPECT_EQ(events[1].GetPropsAsMap().count(
                  "early_sample_mean_bts_attributed_bytes"),
              0u);
    EXPECT_EQ(events[1].GetPropsAsMap().count(
                  "early_sample_peak_bts_attributed_bytes"),
              0u);
    EXPECT_EQ(
        events[1].GetPropsAsMap().count("early_sample_mean_mts_heap_bytes"),
        0u);
    EXPECT_EQ(
        events[1].GetPropsAsMap().count("early_sample_peak_mts_heap_bytes"),
        0u);
    EXPECT_EQ(Number(events[1], "early_window_target_duration_ms"), 20000);
  }

  void RunCommonSamplingProperties() {
    report::MoveOnlyEvent event;
    MemoryMonitor::AddCommonProps(event);
    const auto props = event.GetPropsAsMap();
    ASSERT_EQ(props.count("lynx_ui_memory_enabled"), 1u);
    EXPECT_EQ(Number(event, "lynx_ui_memory_enabled"),
              MemoryMonitor::GetSettings().lynx_ui.enabled);
  }

  void RunSinglePageMemoryScope() {
    OnReporter([] {
      GlobalMemoryMonitor monitor;
      Populate(monitor);
      MemoryMonitor page(nullptr, base::LogContext{}, 1);
      page.global_ = &monitor;
      auto& settings =
          const_cast<MemoryMonitor::Settings&>(MemoryMonitor::GetSettings());
      const auto ui_enabled = settings.lynx_ui.enabled;
      settings.lynx_ui.enabled = 0;
      FillRecords(page);
      page.UpdateMemoryUsage(MemoryRecord("x-viewpager-ng", 0));
      EXPECT_EQ(page.last_usage_.total_bytes, 100);
      EXPECT_EQ(page.last_usage_.ui_bytes, 30);
      const auto disabled = page.BuildPageEvent(
          page.last_usage_, monitor.instance_state_.at(1), 0);
      EXPECT_EQ(Number(disabled, "lynx_ui_memory_enabled"), 0);
      EXPECT_EQ(disabled.GetPropsAsMap().count("viewSizeBytes"), 0u);
      EXPECT_EQ(disabled.GetPropsAsMap().count("x_viewpager_ngSizeBytes"), 0u);
      settings.lynx_ui.enabled = ui_enabled;
      EXPECT_EQ(page.RefreshMemoryUsage().total_bytes, 100);

      auto first = FullUsage(100, 40, false);
      const auto event =
          page.BuildPageEvent(first, monitor.instance_state_.at(1), 0);
      EXPECT_EQ(Number(event, "current_page_memory_bytes"), 100);
      EXPECT_EQ(Number(event, "viewSizeBytes"), 30);
      EXPECT_EQ(Number(event, "x_viewpager_ngSizeBytes"), 0);
      EXPECT_EQ(page.memory_records_.count("x-viewpager-ng"), 1u);
      EXPECT_EQ(page.memory_records_.count("x_viewpager_ng"), 0u);
      UniqueKeys(event);
      for (const char* field :
           {"page_required_sources", "source_tasm_element_complete",
            "source_mts_vm_heap_complete", "source_bts_vm_heap_complete",
            "source_platform_ui_complete", "current_page_full_bytes",
            "current_page_core_bytes", "lynxTasmElementSizeBytes",
            "mainThreadScriptingEngineSizeBytes",
            "backgroundThreadScriptingEngineSizeBytes"}) {
        EXPECT_EQ(event.GetPropsAsMap().count(field), 0u) << field;
      }

      page.anchor_at_ms_ = 1000;
      page.ObserveEarly(first, 0);
      auto second = first;
      second.total_bytes = 200;
      second.element_bytes += 100;
      page.ObserveEarly(second, 1);
      report::MoveOnlyEvent summary;
      page.AddEarlySummary(summary, 1000);
      EXPECT_DOUBLE_EQ(Number(summary, "early_sample_mean_memory_bytes"), 150);
      EXPECT_EQ(Number(summary, "early_sample_peak_memory_bytes"), 200);
      EXPECT_DOUBLE_EQ(
          Number(summary, "early_sample_mean_bts_attributed_bytes"), 40);
      EXPECT_EQ(Number(summary, "early_sample_peak_bts_attributed_bytes"), 40);
      EXPECT_DOUBLE_EQ(Number(summary, "early_sample_mean_mts_heap_bytes"), 10);
      EXPECT_EQ(Number(summary, "early_sample_peak_mts_heap_bytes"), 10);
      for (const char* field :
           {"early_full_profile_eligible", "early_core_profile_eligible",
            "early_usage_summary_full_valid", "early_usage_summary_core_valid",
            "early_sample_full_count", "early_sample_core_count",
            "early_sample_mean_full_bytes", "early_sample_mean_core_bytes",
            "early_sample_peak_full_bytes", "early_sample_peak_core_bytes"}) {
        EXPECT_EQ(summary.GetPropsAsMap().count(field), 0u) << field;
      }
    });
  }

  void RunGlobalPageMemoryWithoutValidity() {
    OnReporter([] {
      GlobalMemoryMonitor monitor;
      GlobalMemoryMonitor::Snapshot snapshot;
      snapshot.started_at_ms = 1000;
      InstanceState page;
      page.SetUrl("app/page");
      page.usage = FullUsage(100, 40, false);
      page.memory_usage = 100;
      monitor.instance_state_.emplace(1, page);

      const auto event = BuildEvent(monitor, snapshot, 1000, 1000);
      EXPECT_EQ(Number(event, "lynx_accounted_memory_bytes"), 100);
      EXPECT_EQ(Number(event, "page_memory_bytes"), 100);
      EXPECT_EQ(Number(event, "page_group_memory_bytes"), 100);
      for (const char* field :
           {"active_page_full_memory_count", "lynx_full_memory_snapshot_valid",
            "sampled_active_page_full_memory_valid",
            "sampled_active_page_group_full_memory_valid"}) {
        EXPECT_EQ(event.GetPropsAsMap().count(field), 0u) << field;
      }
    });
  }

  void RunGlobalQueryUsesVmIdentity() {
    fml::Thread js_thread("memory-global-query-js");
    const auto js_runner = js_thread.GetTaskRunner();
    OnReporter([&] {
      GlobalMemoryMonitor monitor;
      std::array<std::shared_ptr<TestVM>, 2> engines = {
          std::make_shared<TestVM>(runtime::js::JSRuntimeType::quickjs),
          std::make_shared<TestVM>(runtime::js::JSRuntimeType::quickjs)};
      for (size_t i = 0; i < engines.size(); ++i) {
        engines[i]->heap_size = (i + 1) * 100;
        auto& vm = AddVM(monitor);
        vm.SetName("same-group");
        vm.vm = engines[i];
        vm.vm_ptr = engines[i].get();
        vm.type = runtime::js::JSRuntimeType::quickjs;
        vm.js_thread_runner = js_runner;
        vm.heap_valid = true;
        vm.heap_size = (i + 1) * 100;
        vm.slot_tracking_enabled = vm.has_per_instance_data = true;
        vm.memory_slots[3 + i] = (i + 1) * 10;

        InstanceState page;
        page.SetUrl(i == 0 ? "app/first" : "app/second");
        page.page_key = i == 0 ? "first" : "second";
        page.group_id = "same-group";
        page.bts_vm = engines[i].get();
        page.slot = 3 + i;
        page.usage = FullUsage(i == 0 ? 60 : 100, (i + 1) * 10, true);
        page.usage.element_bytes = i == 0 ? 20 : 30;
        page.usage.element_count = i == 0 ? 2 : 3;
        page.usage.mts_bytes = i == 0 ? 10 : 20;
        page.usage.ui_bytes = i == 0 ? 20 : 30;
        monitor.instance_state_.emplace(static_cast<int32_t>(i + 1),
                                        std::move(page));
      }
      auto idle_engine =
          std::make_shared<TestVM>(runtime::js::JSRuntimeType::quickjs);
      auto& idle_vm = AddVM(monitor);
      idle_vm.SetName("same-group");
      idle_vm.vm = idle_engine;
      idle_vm.vm_ptr = idle_engine.get();
      idle_vm.type = runtime::js::JSRuntimeType::quickjs;
      idle_vm.js_thread_runner = js_runner;
      idle_vm.heap_valid = true;
      idle_vm.heap_size = 500;

      bool callback_invoked = false;
      monitor.GetGlobalMemoryUsage([&](GlobalMemoryUsage usage) {
        callback_invoked = true;
        EXPECT_EQ(usage.total.total_bytes, 430);
        EXPECT_EQ(usage.total.element_bytes, 50);
        EXPECT_EQ(usage.total.element_count, 5);
        EXPECT_EQ(usage.total.mts_bytes, 30);
        EXPECT_EQ(usage.total.ui_bytes, 50);
        EXPECT_EQ(usage.total.bts_bytes, 300);
        ASSERT_EQ(usage.instances.size(), 2u);
        for (const auto& instance : usage.instances) {
          ASSERT_TRUE(instance.instance_id == 1 || instance.instance_id == 2);
          EXPECT_EQ(instance.page.bts_bytes, instance.instance_id * 10);
          EXPECT_EQ(instance.bts_heap_bytes, instance.instance_id * 100);
          EXPECT_EQ(instance.bts_runtime_group_id, "same-group");
        }
      });
      EXPECT_TRUE(callback_invoked);
    });
  }

  void RunMTSRuntimePoolAccountingAndSampling() {
    OnReporter([] {
      GlobalMemoryMonitor monitor;
      report::EventTrackerPlatformImpl::UpdateGenericInfo(
          7, "page_id", std::string("pool-page"));
      InstanceState page;
      page.SetUrl("app/pool");
      page.usage = FullUsage(100, 40, false);
      page.memory_usage = 100;
      monitor.instance_state_.emplace(7, std::move(page));

      const runtime::ContextType types[] = {
          runtime::VMContextType, runtime::LepusNGContextType,
          runtime::RTSContextType, runtime::RTSNativeContextType};
      for (size_t i = 0; i < std::size(types); ++i) {
        monitor.OnMTSRuntimePoolUpdate(static_cast<int32_t>(10 + i), "app/pool",
                                       types[i], 100, i, i * 10);
      }
      ASSERT_NE(monitor.poll_timer_, nullptr);
      monitor.poll_timer_->Stop();
      ASSERT_EQ(monitor.mts_runtime_pool_state_.size(), 4u);

      GlobalMemoryMonitor::Trigger trigger;
      trigger.reasons = GlobalMemoryMonitor::kTimer;
      auto snapshot = monitor.CollectSnapshot(std::move(trigger));
      snapshot.started_at_ms = 1000;
      for (const auto& item : monitor.mts_runtime_pool_state_) {
        EXPECT_TRUE(item.second.page_key_resolved);
        EXPECT_EQ(item.second.page_key, "pool-page");
      }

      const auto global_usage = monitor.BuildGlobalMemoryUsage(snapshot);
      EXPECT_EQ(global_usage.total.total_bytes, 100);
      EXPECT_EQ(global_usage.total.mts_bytes, 10);

      auto events = monitor.BuildEvents(snapshot, 1000, 1000);
      const report::MoveOnlyEvent* process = nullptr;
      for (const auto& event : events) {
        if (event.GetName() == "lynxsdk_memory_process") process = &event;
      }
      ASSERT_NE(process, nullptr);
      UniqueKeys(*process);
      CompactSchema(*process);
      EXPECT_EQ(Number(*process, "lynx_accounted_memory_bytes"), 160);
      EXPECT_EQ(Number(*process, "mts_pool_count"), 4);
      EXPECT_EQ(Number(*process, "mts_pool_runtime_count"), 6);
      EXPECT_EQ(Number(*process, "mts_pool_heap_bytes_sum"), 60);
      EXPECT_DOUBLE_EQ(Number(*process, "lynx_mts_pool_process_pss_ratio"),
                       0.06);

      std::array<bool, 4> selected{};
      constexpr size_t kDrawCount = 1024;
      for (size_t draw = 0; draw < kDrawCount; ++draw) {
        events = monitor.BuildEvents(snapshot, 1000, 1000);
        const report::MoveOnlyEvent* pool_event = nullptr;
        for (const auto& event : events) {
          if (event.GetName() == "lynxsdk_memory_mts_pool") {
            pool_event = &event;
          }
        }
        ASSERT_NE(pool_event, nullptr);
        UniqueKeys(*pool_event);
        CompactSchema(*pool_event);
        const simulation::Row row(*pool_event);
        const auto type = static_cast<size_t>(row.Number("mts_context_type"));
        ASSERT_LT(type, selected.size());
        selected[type] = true;
        EXPECT_EQ(row.Text("page_key"), "pool-page");
        EXPECT_EQ(row.Number("mts_pool_age_ms"), 900);
        EXPECT_EQ(row.Number("mts_pool_runtime_count"), type);
        EXPECT_EQ(row.Number("mts_pool_runtime_count_weighted"), type * 4);
        EXPECT_EQ(row.Number("mts_pool_selection_weight"), 4);
        EXPECT_EQ(row.Number("mts_pool_heap_bytes"), type * 10);
        EXPECT_EQ(row.Number("mts_pool_heap_bytes_weighted"), type * 40);
        EXPECT_EQ(row.Number("mts_pool_heap_selection_weight"), 4);
      }
      EXPECT_TRUE(std::all_of(selected.begin(), selected.end(),
                              [](bool value) { return value; }));

      monitor.OnMTSRuntimePoolDestroy(10);
      EXPECT_EQ(monitor.mts_runtime_pool_state_.count(10), 0u);
    });
  }
};

TEST_F(MemoryMonitorTest, LogContextIsAReplaceableSnapshot) { RunLogContext(); }
TEST_F(MemoryMonitorTest, RecordUpdatesAndDestructionDoNotEmitObserverEvents) {
  RunRecordUpdates();
}
TEST_F(MemoryMonitorTest,
       PageKeysUseHostIdentityAndPreserveOriginalUrlFallback) {
  RunPageKeys();
}
TEST_F(MemoryMonitorTest, ExitBeforeFirstReportPreservesPageKey) {
  RunExitPageKeyBeforeFirstReport();
}
TEST_F(MemoryMonitorTest, InstanceCreationInitializesInlineOnReporter) {
  RunInstanceCreation(true);
}
TEST_F(MemoryMonitorTest,
       InstanceCreationInitializesOnReporterFromAnotherThread) {
  RunInstanceCreation(false);
}
TEST_F(MemoryMonitorTest, StandaloneCacheAndRecordArithmetic) {
  RunPageCache();
}
TEST_F(MemoryMonitorTest, SharedSlotReplacementAndMissingAttribution) {
  RunSharedCache();
}
TEST_F(MemoryMonitorTest, ShortPageSampleMeanAndUniqueSummary) {
  RunShortSummary();
}
TEST_F(MemoryMonitorTest, SavedSummarySurvivesRuntimeTeardownBeforeMonitor) {
  RunEndSummary(true);
}
TEST_F(MemoryMonitorTest, SavedSummaryDoesNotWaitForRuntimeTeardown) {
  RunEndSummary(false);
}
TEST_F(MemoryMonitorTest, SingleSampleSummaryRequiresEveryDuePoint) {
  RunSingleSampleSummary();
}
TEST_F(MemoryMonitorTest, DestructionWithoutAnchorDoesNotEmitSummary) {
  RunNoAnchor();
}
TEST_F(MemoryMonitorTest, PartialRecordsRemainInPageSummary) {
  RunPartialRecordSummary();
}
TEST_F(MemoryMonitorTest, CompletedSummaryDoesNotEmitOnDestruction) {
  RunScheduledAndExit();
}
TEST_F(MemoryMonitorTest, DelayedSamplesRemainValid) { RunDelayedSamples(); }
TEST_F(MemoryMonitorTest, GlobalDedupAndPssRatio) {
  OnReporter([this] { RunGlobalEvent(); });
}
TEST_F(MemoryMonitorTest, ConcurrentPageKeyGroupWeightsAndTotals) {
  OnReporter([this] { RunPageKeyGroup(); });
}
TEST_F(MemoryMonitorTest, TriggerTypesRouteToDedicatedEvents) {
  RunTriggerEventRouting();
}
TEST_F(MemoryMonitorTest, ReservedSlotsSupportVmNamePercentiles) {
  RunReservedSlotQueries();
}
TEST(MemorySimulationAnalyticsTest, NearestRankPercentilesAtExactBoundaries) {
  // Q1 and Q8 must share ceil(p*N)-1; N*p itself is off by one for N=20/100.
  for (size_t count : {1, 20, 100}) {
    std::vector<double> sorted;
    for (size_t i = 1; i <= count; ++i) sorted.push_back(i);
    EXPECT_EQ(simulation::Percentile(sorted, .95), std::ceil(count * .95));
    EXPECT_EQ(simulation::Percentile(sorted, .99), std::ceil(count * .99));
    EXPECT_EQ(simulation::Percentile(sorted, 1), count);
  }
}

TEST(MemorySimulationAnalyticsTest, FixedAgeQueryIncludesZeroValues) {
  simulation::Analytics analytics;
  for (int64_t bytes : {0, 100}) {
    report::MoveOnlyEvent event;
    event.SetName("lynxsdk_performance_entry_memory_v2");
    event.SetProps("schema_version", 10);
    event.SetProps("page_key", "zero-or-missing");
    event.SetProps("mts_context_type", -1);
    event.SetProps("report_kind", "scheduled");
    event.SetProps("scheduled_offset_ms", 5000);
    event.SetProps("current_page_memory_bytes", bytes);
    analytics.Consume(event, 0, 8);
  }
  EXPECT_EQ(analytics.page_rows, 2u);
  ASSERT_EQ(analytics.values.count("page_5s"), 1u);
  const auto& groups = analytics.values.at("page_5s");
  ASSERT_EQ(groups.count("zero-or-missing"), 1u);
  EXPECT_EQ(groups.at("zero-or-missing"), (std::vector<double>{0, 100}));
}

TEST(MemorySimulationAnalyticsTest, ResidualMeanUsesPresentPositiveWeights) {
  simulation::Analytics analytics;
  const struct {
    bool timer;
    bool present;
    double weight;
    double retained;
  } rows[] = {
      {true, true, 10, 100},    {true, true, 2, 100},
      {true, true, -1, 10000},  // Missing weight is unusable.
      {true, true, 0, 10000},   {true, true, 10, 10000},
      {false, true, 10, 10000}, {true, false, 10, 10000},
  };
  for (const auto& row : rows) {
    if (!row.timer || !row.present) continue;
    report::MoveOnlyEvent event;
    event.SetName("lynxsdk_memory_page_residual");
    event.SetProps("schema_version", 10);
    event.SetProps("leak_page_key", "exited/page");
    event.SetProps("shared_vm_tracked_exited_page_retained_bytes",
                   row.retained);
    if (row.weight >= 0)
      event.SetProps("leak_page_selection_weight", row.weight);
    analytics.Consume(event, 0, 8);
  }
  const auto& numerator = analytics.sums.at("residual");
  const auto& denominator = analytics.sums.at("residual_weight");
  ASSERT_EQ(numerator.size(), 1u);
  ASSERT_EQ(denominator.size(), 1u);
  EXPECT_DOUBLE_EQ(numerator.at("exited/page"), 10200);
  EXPECT_DOUBLE_EQ(denominator.at("exited/page"), 22);
  EXPECT_DOUBLE_EQ(numerator.at("exited/page") / denominator.at("exited/page"),
                   10200.0 / 22);
}

TEST(MemorySimulationAnalyticsTest,
     ResidualAreaUsesFieldPresenceAsCompletenessGate) {
  simulation::Analytics analytics;
  report::MoveOnlyEvent event;
  event.SetName("lynxsdk_memory_persistence");
  event.SetProps("schema_version", 10);
  event.SetProps("leak_page_key", "exited/page");
  event.SetProps("leak_page_attributed_byte_ms", 123);

  analytics.Consume(event, 0, 8);

  ASSERT_EQ(analytics.sums.at("residual_area").size(), 1u);
  EXPECT_DOUBLE_EQ(analytics.sums.at("residual_area").at("exited/page"), 123);
}

TEST_F(MemoryMonitorTest, SimulatedAndroidWorkloadMeetsMonitoringGoals) {
  RunSimulation("android");
}
TEST_F(MemoryMonitorTest, SimulatedIosWorkloadRespectsUnsupportedJsc) {
  RunSimulation("ios");
}
TEST_F(MemoryMonitorTest, SimulatedHarmonyWorkloadAccountsSharedJsvm) {
  RunSimulation("harmony");
}
TEST_F(MemoryMonitorTest, SimulatedPcWorkloadMeetsMonitoringGoals) {
  RunSimulation("pc");
}
TEST_F(MemoryMonitorTest, PpsCensusReusePreservesAttributionAndClippedAreas) {
  RunPpsCensusReuse();
}
TEST_F(MemoryMonitorTest, HeapOnlyAndUnsupportedEngines) {
  OnReporter([this] { RunEngineCapabilities(); });
}
TEST_F(MemoryMonitorTest, SlotsCapacityOverflowAndResidualCleanup) {
  RunLifetimeAndSlots();
}
TEST_F(MemoryMonitorTest, ResidualsRequireValidFullGcStrictlyAfterExit) {
  RunPostExitFullGcResiduals();
}
TEST_F(MemoryMonitorTest, SynchronousHeapQueryRunsOnOwnerThread) {
  RunSynchronousHeapCollection();
}
TEST_F(MemoryMonitorTest, HeapQueryTimeoutKeepsCachedValueAndMarksPartial) {
  RunHeapCollectionTimeout();
}
TEST_F(MemoryMonitorTest, ReporterUpdatesWaitAndVmReferenceIsReleasedOnJs) {
  RunReporterUpdatesWait();
}
TEST_F(MemoryMonitorTest, MissingHeapRunnerKeepsCandidateAndMarksPartial) {
  RunMissingHeapRunner();
}
TEST_F(MemoryMonitorTest, ExpiredVmDuringSyncQueryKeepsCachedCandidate) {
  RunExpiredHeapWhileWaiting();
}
TEST_F(MemoryMonitorTest,
       ExpiredVmCleanupPreservesLiveEntriesBeyondInlineCapacity) {
  RunExpiredVmCleanup();
}
TEST_F(MemoryMonitorTest, QueuedSlotOverflowKeepsValuesAfterVmDestruction) {
  RunQueuedSlotOverflowTrigger();
}
TEST_F(MemoryMonitorTest, SynchronousGlobalEventsKeepValuesUntilFlush) {
  RunGlobalQueueAfterSync();
}
TEST_F(MemoryMonitorTest, UntrackedVmPagesDoNotAccumulateAfterExit) {
  RunUntrackedCleanup();
}
TEST_F(MemoryMonitorTest, VMSharingPolicyMatchesRegistration) {
  RunVMSharingPolicy();
}
TEST_F(MemoryMonitorTest, ScheduledTimerSamplesWithoutChangingSettings) {
  RunScheduledTimer();
}
TEST_F(MemoryMonitorTest, ScheduledSamplingPreservesFullEarlySummary) {
  for (bool enabled : {false, true}) {
    RunScheduledDeliverySampling(enabled, false, false);
  }
}
TEST_F(MemoryMonitorTest, ScheduledSamplingPreservesShortEarlySummary) {
  for (bool enabled : {false, true}) {
    RunScheduledDeliverySampling(enabled, true, false);
  }
}
TEST_F(MemoryMonitorTest, ScheduledSamplingPreservesMissedPointSummary) {
  for (bool enabled : {false, true}) {
    RunScheduledDeliverySampling(enabled, false, true);
  }
}
TEST_F(MemoryMonitorTest, TestStartupEnablesAllMemorySources) {
  RunSamplingSettings();
}
TEST_F(MemoryMonitorTest, ConfigUsesCommaSeparatedIndependentKeyValuePairs) {
  RunConfigParsing();
}
#if ENABLE_TRACE_PERFETTO
TEST_F(MemoryMonitorTest, PersistentForceEnableOverridesSamplingConfig) {
  RunPersistentForceEnable();
}
#endif
TEST_F(MemoryMonitorTest, CommonPropertiesExposeUiMemoryConfiguration) {
  RunCommonSamplingProperties();
}
TEST_F(MemoryMonitorTest, PageEventsUseOneMemoryScopeForPartialRecords) {
  RunSinglePageMemoryScope();
}
TEST_F(MemoryMonitorTest, GlobalPageMemoryDoesNotRequireSourceValidity) {
  RunGlobalPageMemoryWithoutValidity();
}
TEST_F(MemoryMonitorTest, GlobalQueryCountsSameNameVmsByIdentity) {
  RunGlobalQueryUsesVmIdentity();
}
TEST_F(MemoryMonitorTest,
       MTSRuntimePoolsContributeOnlyToProcessAccountingAndPoolSamples) {
  RunMTSRuntimePoolAccountingAndSampling();
}
TEST_F(MemoryMonitorTest,
       MissedDeadlinePublishesOnceAndExitStopsScheduledEvents) {
  RunMissedDeadlineAndExit();
}

TEST_F(MemoryMonitorTest, ResidencyRequiresCompleteProcessObservationWindow) {
  RunResidencyCohort();
}

TEST_F(MemoryMonitorTest, UniformVmDetailsAndCompleteResiduals) {
  RunVmDetailAndResidual();
}

TEST(MemorySamplesTest, FullWindowEqualsNinePointReference) {
  EarlyMemorySamples total_samples;
  EarlyMemorySamples bts_samples;
  EarlyMemorySamples mts_samples;
  const std::array<int64_t, kEarlyMemorySampleCount> values = {
      80, 620, 80, 80, 80, 80, 80, 80, 80};
  const std::array<int64_t, kEarlyMemorySampleCount> bts_values = {
      10, 100, 10, 10, 10, 10, 10, 10, 10};
  const std::array<int64_t, kEarlyMemorySampleCount> mts_values = {
      20, 200, 20, 20, 20, 20, 20, 20, 20};
  for (size_t i = 0; i < values.size(); ++i) {
    total_samples.Observe(values[i], i);
    bts_samples.Observe(bts_values[i], i);
    mts_samples.Observe(mts_values[i], i);
  }
  // A late observation cannot alter the early mean or peak.
  total_samples.Observe(1000, kEarlyMemorySampleCount);
  bts_samples.Observe(500, kEarlyMemorySampleCount);
  mts_samples.Observe(1000, kEarlyMemorySampleCount);
  const auto required = (1u << kEarlyMemorySampleCount) - 1;
  EXPECT_TRUE(total_samples.Complete(required));
  EXPECT_TRUE(bts_samples.Complete(required));
  EXPECT_TRUE(mts_samples.Complete(required));
  EXPECT_EQ(total_samples.Mean(), 140);
  EXPECT_EQ(bts_samples.Mean(), 20);
  EXPECT_EQ(mts_samples.Mean(), 40);
  EXPECT_EQ(total_samples.count, kEarlyMemorySampleCount);
  EXPECT_EQ(total_samples.peak_bytes, 620);
  EXPECT_EQ(bts_samples.peak_bytes, 100);
  EXPECT_EQ(mts_samples.peak_bytes, 200);
}

TEST(MemorySamplesTest, SingleSampleIsValidButMissingDuePointsAreNot) {
  EarlyMemorySamples samples;
  EXPECT_FALSE(samples.Complete(1));
  samples.Observe(300, 0);
  EXPECT_TRUE(samples.Complete(1));
  EXPECT_EQ(samples.Mean(), 300);
  EXPECT_FALSE(samples.Complete(3));
  // A non-scheduled endpoint cannot fill a missing 1s point.
  samples.Observe(900, -1);
  EXPECT_FALSE(samples.Complete(3));
  EXPECT_EQ(samples.Mean(), 300);
  EXPECT_EQ(samples.count, 1);
  samples.Observe(0, 1);
  EXPECT_TRUE(samples.Complete(3));
  EXPECT_EQ(samples.Mean(), 150);
}

TEST(MemorySamplesTest, DuplicateAttemptsCannotRepairInvalidSamples) {
  EarlyMemorySamples samples;
  samples.Observe(100, 0);
  samples.Observe(900, 0);
  EXPECT_EQ(samples.Mean(), 100);
  samples.Observe(-1, 1);
  samples.Observe(100, 1);
  EXPECT_EQ(samples.attempted_mask, 3u);
  EXPECT_EQ(samples.scheduled_mask, 1u);
  EXPECT_FALSE(samples.Complete(3));
  EXPECT_EQ(samples.count, 1);
}

TEST(MemoryReservoirTest, HeapWeightsEstimateOldStableVmContribution) {
  std::mt19937_64 random(42);
  double attributed = 0;
  for (int n = 0; n < 20000; ++n) {
    MemoryReservoir picker;
    int selected = -1;
    if (picker.Consider(900, random)) selected = 0;
    if (picker.Consider(100, random)) selected = 1;
    if (selected == 0) attributed += picker.total();
  }
  // Old VM's heap can be stable forever: selection depends on size, not growth.
  EXPECT_NEAR(attributed / 20000, 900, 10);
  MemoryReservoir empty;
  EXPECT_FALSE(empty.Consider(0, random));
  EXPECT_FALSE(empty.Consider(-1, random));
  EXPECT_EQ(empty.total(), 0);
}

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
