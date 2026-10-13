// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_TESTING_MEMORY_SIMULATION_TEST_SUPPORT_H_
#define CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_TESTING_MEMORY_SIMULATION_TEST_SUPPORT_H_

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/services/event_report/event_tracker.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {
namespace performance {
namespace simulation {

constexpr int64_t kMiB = 1024 * 1024;
using Json = rapidjson::Value;

inline int64_t Integer(const Json& row, size_t i) {
  return row[static_cast<rapidjson::SizeType>(i)].GetInt64();
}

inline int64_t PageBytes(const Json& page, int64_t at) {
  const auto age = at - Integer(page, 2);
  const auto percent = Integer(page, 8) & 8 ? 100
                       : age < 5000         ? 60
                       : age < 10000        ? 80
                       : age < 20000        ? 100
                       : age < 25000        ? 90
                                            : 85;
  return std::max(50 * kMiB, Integer(page, 5) * kMiB * percent / 100);
}

inline int64_t ExitedBytes(const Json& page, int64_t last_gc) {
  if (Integer(page, 8) & 2) return 0;
  if (last_gc <= Integer(page, 3)) return (Integer(page, 6) + 32) * kMiB;
  if (Integer(page, 1) == 6 && last_gc >= Integer(page, 7)) return 0;
  return Integer(page, 6) * kMiB;
}

inline double Percentile(const std::vector<double>& sorted, double p) {
  EXPECT_FALSE(sorted.empty());
  if (sorted.empty()) return 0;
  return sorted[static_cast<size_t>(std::ceil(sorted.size() * p)) - 1];
}

// Analytics below sees only event properties plus common platform/RAM metadata.
// It cannot read the replay's page/VM objects or the reference census.
struct Row {
  explicit Row(const report::MoveOnlyEvent& event)
      : name(event.GetName()), props(event.GetPropsAsMap()) {}
  bool Has(const std::string& key) const { return props.count(key) != 0; }
  double Number(const std::string& key) const {
    const auto it = props.find(key);
    EXPECT_NE(it, props.end()) << key;
    if (it == props.end()) return 0;
    return it->second.GetType() == report::EventProp::Type::kInt32
               ? it->second.GetIntValue()
               : it->second.GetDoubleValue();
  }
  std::string Text(const std::string& key) const {
    const auto it = props.find(key);
    EXPECT_NE(it, props.end()) << key;
    return it == props.end() ? "" : it->second.GetStringValue();
  }
  bool Contains(const std::string& key, const std::string& token) const {
    return Has(key) && Text(key).find(token) != std::string::npos;
  }
  std::string name;
  std::unordered_map<std::string, report::EventProp> props;
};

// Q1..Q11 are defined in memory_monitor.h's monitoring query contract.
// Consume reads events only; Verify* checks independent generator truth with
// predefined sampling bounds. Q1 asserts a leader; Q3 checks pinned pages
// have 100% survival in complete windows; Q4..Q11 compare group
// values. RecordProperty exports results, never replaces assertions. Process
// rate=1.
// This replay does not validate the query platform's stratified heap PCT.
struct Analytics {
  std::map<std::string, std::map<std::string, double>> sums;
  std::map<std::string, std::map<std::string, std::vector<double>>> values;
  std::set<std::string> summaries;
  size_t page_rows{0}, global_rows{0}, residency_rows{0};
  size_t short_valid{0}, overflow{0};
  std::array<size_t, 3> reserved_positive{};

  void Consume(const report::MoveOnlyEvent& event, int session, int ram) {
    Row row(event);
    ASSERT_EQ(row.Number("schema_version"), 10);
    EXPECT_FALSE(row.Has("page_id"));
    EXPECT_FALSE(row.Has("normalized_url"));
    EXPECT_FALSE(row.Has("sampled_active_url"));
    EXPECT_FALSE(row.Has("sampled_leak_page_url"));
    if (row.name == "lynxsdk_performance_entry_memory_v2") {
      ++page_rows;
      // Q2: event production owns the fixed-point timing semantics.
      if (row.Text("report_kind") == "scheduled" &&
          row.Number("scheduled_offset_ms") == 5000) {
        values["page_5s"][row.Text("page_key")].push_back(
            row.Number("current_page_memory_bytes"));
        if (row.Number("mts_context_type") == 1) {
          const auto key =
              row.Text("page_key") + "|" + row.Text("bts_runtime_type");
          values["page_mts_5s"][key].push_back(
              row.Number("current_mts_heap_bytes"));
        }
      }
      // Q1: one received summary per instance, not one vote per scheduled row.
      if (!row.Has("has_early_usage_summary") ||
          row.Number("has_early_usage_summary") != 1)
        return;
      const auto identity =
          std::to_string(session) + "/" + std::to_string(event.GetInstanceId());
      ASSERT_TRUE(summaries.insert(identity).second);
      const auto url = row.Text("page_key");
      if (row.Has("early_sample_mean_memory_bytes")) {
        values["page_mean"][url].push_back(
            row.Number("early_sample_mean_memory_bytes"));
        if (row.Number("early_window_target_duration_ms") < 5000) {
          ++short_valid;
          values["page_short"][url].push_back(
              row.Number("early_sample_mean_memory_bytes"));
        }
      }
      if (row.Has("early_sample_peak_memory_bytes")) {
        values["page_peak"][url].push_back(
            row.Number("early_sample_peak_memory_bytes"));
        if (row.Number("early_window_target_duration_ms") < 5000) {
          values["page_peak_short"][url].push_back(
              row.Number("early_sample_peak_memory_bytes"));
        }
      }
      if (row.Has("early_sample_mean_bts_attributed_bytes")) {
        values["page_bts_mean"][url].push_back(
            row.Number("early_sample_mean_bts_attributed_bytes"));
        if (row.Number("early_window_target_duration_ms") < 5000) {
          values["page_bts_short"][url].push_back(
              row.Number("early_sample_mean_bts_attributed_bytes"));
        }
      }
      if (row.Has("early_sample_peak_bts_attributed_bytes")) {
        values["page_bts_peak"][url].push_back(
            row.Number("early_sample_peak_bts_attributed_bytes"));
        if (row.Number("early_window_target_duration_ms") < 5000) {
          values["page_bts_peak_short"][url].push_back(
              row.Number("early_sample_peak_bts_attributed_bytes"));
        }
      }
      if (row.Has("early_sample_mean_mts_heap_bytes")) {
        values["page_mts_mean"][url].push_back(
            row.Number("early_sample_mean_mts_heap_bytes"));
        if (row.Number("early_window_target_duration_ms") < 5000) {
          values["page_mts_short"][url].push_back(
              row.Number("early_sample_mean_mts_heap_bytes"));
        }
      }
      if (row.Has("early_sample_peak_mts_heap_bytes")) {
        values["page_mts_peak"][url].push_back(
            row.Number("early_sample_peak_mts_heap_bytes"));
        if (row.Number("early_window_target_duration_ms") < 5000) {
          values["page_mts_peak_short"][url].push_back(
              row.Number("early_sample_peak_mts_heap_bytes"));
        }
      }
      return;
    }
    if (row.name == "lynxsdk_memory_page_residency") {
      ++residency_rows;
      EXPECT_EQ(row.Number("residency_threshold_ms"), 600000);
      sums["resident"][row.Text("page_key")] += row.Number("alive_page_count");
      sums["resident_weight"][row.Text("page_key")] +=
          row.Number("evaluated_page_count");
      return;
    }

    if (row.name == "lynxsdk_memory_process") {
      ++global_rows;
      // Q8: percentile of the per-event ratio, grouped by RAM/milestone.
      if (row.Has("trigger_pss_milestone_bytes")) {
        for (int gib : {1, 2, 3}) {
          const double threshold = gib * 1024.0 * kMiB;
          if (row.Number("trigger_pss_previous_milestone_bytes") < threshold &&
              row.Number("trigger_pss_milestone_bytes") >= threshold) {
            values["ratios"][std::to_string(ram) + "/" + std::to_string(gib)]
                .push_back(row.Number("lynx_accounted_process_pss_ratio"));
          }
        }
      }
      return;
    }

    if (row.name == "lynxsdk_memory_persistence") {
      if (row.Has("vm_attributed_heap_byte_ms")) {
        sums["vm_area"][row.Text("vm_name")] +=
            row.Number("vm_attributed_heap_byte_ms");
      }
      if (row.Has("leak_page_attributed_byte_ms")) {
        sums["residual_area"][row.Text("leak_page_key")] +=
            row.Number("leak_page_attributed_byte_ms");
      }
      return;
    }

    if (row.name == "lynxsdk_memory_vm_quickjs") {
      const auto kind = row.Text("report_kind");
      if (kind == "slot_overflow") {
        ++overflow;
        // Q9.2: one row for the first failed allocation of each VM.
        const auto name = row.Text("vm_name");
        values["slot_overflow_age"][name].push_back(row.Number("vm_age_ms"));
        values["slot_overflow_loads"][name].push_back(
            row.Number("vm_total_page_count"));
        return;
      }
      EXPECT_EQ(kind, "sample");
      const auto name = row.Text("vm_name");
      if (row.Has("vm_retained_selection_weight")) {
        sums["vm_retained"][name] += row.Number("vm_retained_bytes_weighted");
        sums["vm_retained_weight"][name] +=
            row.Number("vm_retained_selection_weight");
      }
      size_t index = 0;
      for (const char* category : {"unknown", "common", "overflow"}) {
        const auto field = std::string("vm_") + category + "_bytes";
        const auto bytes = row.Number(field);
        EXPECT_GE(bytes, 0);
        values[std::string("reserved_") + category][name].push_back(bytes);
        reserved_positive[index++] += bytes > 0;
      }
      return;
    }

    if (row.name == "lynxsdk_memory_vm_stat") {
      const auto vm_name = row.Text("vm_name");
      sums["vm_reuse"][vm_name] += row.Number("vm_total_page_count_weighted");
      sums["vm_reuse_weight"][vm_name] += row.Number("vm_selection_weight");
      if (row.Has("vm_heap_bytes") && row.Number("vm_age_ms") >= 1800000) {
        sums["old_vm"][vm_name] += row.Number("vm_heap_bytes_weighted");
        sums["old_vm_weight"][vm_name] +=
            row.Number("vm_heap_selection_weight");
        if (row.Number("vm_active_page_count") == 0 &&
            row.Number("vm_idle_ms") >= 600000) {
          sums["idle_vm"][vm_name] += row.Number("vm_heap_bytes_weighted");
        }
      }
      return;
    }

    if (row.name == "lynxsdk_memory_mts_pool") {
      const auto key =
          row.Text("page_key") + "|" +
          std::to_string(static_cast<int>(row.Number("mts_context_type")));
      sums["mts_pool_heap"][key] += row.Number("mts_pool_heap_bytes_weighted");
      sums["mts_pool_runtime"][key] +=
          row.Number("mts_pool_runtime_count_weighted");
      sums["mts_pool_weight"][key] +=
          row.Number("mts_pool_heap_selection_weight");
      EXPECT_EQ(row.Number("mts_pool_selection_weight"),
                row.Number("mts_pool_heap_selection_weight"));
      EXPECT_GE(row.Number("mts_pool_heap_bytes"), 0);
      return;
    }

    if (row.name == "lynxsdk_memory_page") {
      const auto url = row.Text("page_key");
      if (row.Number("page_age_ms") >= 600000) {
        sums["resident_bytes"][url] += row.Number("page_memory_bytes_weighted");
        sums["resident_memory_weight"][url] += row.Number("active_page_count");
      }
      sums["url_bytes"][url] += row.Number("page_group_memory_bytes_weighted");
      sums["url_weight"][url] += row.Number("page_group_selection_weight");
      sums["url_count"][url] += row.Number("active_page_count");
      return;
    }

    if (row.name == "lynxsdk_memory_page_residual") {
      const auto key = row.Text("leak_page_key");
      if (row.Has("leak_page_selection_weight") &&
          row.Number("leak_page_selection_weight") > 0) {
        sums["residual"][key] +=
            row.Number("shared_vm_tracked_exited_page_retained_bytes");
        sums["residual_weight"][key] +=
            row.Number("leak_page_selection_weight");
      }
      return;
    }

    ADD_FAILURE() << "Unexpected memory event " << row.name;
  }

  void VerifySampledSum(const Json& oracle, const char* metric) {
    SCOPED_TRACE(metric);
    ASSERT_TRUE(oracle.HasMember(metric));
    // JSON keys are concrete workload identities, not anonymous expected
    // totals: e.g. oracle["residual"]["feed/detail"] or
    // oracle["old_vm"]["shared_feed"]. Compare EVERY expected group with
    // the event-only sum for that same name, then reject unexpected groups.
    // [0] is the independent census sum; [1] is its sampling variance.
    for (auto it = oracle[metric].MemberBegin();
         it != oracle[metric].MemberEnd(); ++it) {
      const auto key = std::string(it->name.GetString());
      const double expected = it->value[0].GetDouble();
      const double variance = it->value[1].GetDouble();
      // Six standard deviations is fixed before observing sampled results.
      // A small relative allowance covers floating summation only.
      const double tolerance =
          6 * std::sqrt(variance) + std::abs(expected) * 1e-9 + 1e-6;
      EXPECT_NEAR(sums[metric][key], expected, tolerance)
          << metric << "/" << key;
    }
    for (const auto& entry : sums[metric])
      EXPECT_TRUE(oracle[metric].HasMember(entry.first.c_str()))
          << metric << "/" << entry.first;
  }

  void VerifyAggregateRatio(const Json& oracle, const char* numerator_key,
                            const char* denominator_key,
                            const char* result_key) {
    SCOPED_TRACE(result_key);
    ASSERT_TRUE(oracle.HasMember(numerator_key));
    ASSERT_TRUE(oracle.HasMember(denominator_key));
    for (auto it = oracle[numerator_key].MemberBegin();
         it != oracle[numerator_key].MemberEnd(); ++it) {
      const std::string key = it->name.GetString();
      const auto& numerator = it->value;
      ASSERT_TRUE(oracle[denominator_key].HasMember(key.c_str()));
      const auto& denominator = oracle[denominator_key][key.c_str()];
      const double n = numerator[0].GetDouble();
      const double d = denominator[0].GetDouble();
      const double n_error =
          6 * std::sqrt(numerator[1].GetDouble()) + std::abs(n) * 1e-9 + 1e-6;
      const double d_error =
          6 * std::sqrt(denominator[1].GetDouble()) + std::abs(d) * 1e-9 + 1e-6;
      // A rare group can have zero sampled denominator. Do not manufacture a
      // zero mean; only publish/validate ratios with a positive lower bound.
      if (d <= d_error) continue;
      ASSERT_GT(sums[denominator_key][key], 0) << result_key << "/" << key;
      const double expected = n / d;
      const double actual =
          sums[numerator_key][key] / sums[denominator_key][key];
      const double tolerance =
          (n_error + std::abs(expected) * d_error) / (d - d_error);
      EXPECT_NEAR(actual, expected, tolerance) << result_key << "/" << key;
      sums[result_key][key] = actual;
    }
  }

  void VerifyDistribution(const Json& oracle, const char* metric) {
    SCOPED_TRACE(metric);
    ASSERT_TRUE(oracle.HasMember(metric));
    ASSERT_EQ(values[metric].size(), oracle[metric].MemberCount()) << metric;
    // Example: page_5s/feed/detail stores [count, mean, P50, P90, P95, P99].
    // These observations are not PPS samples: require the exact group/count
    // and each order statistic, allowing only floating-point rounding.
    for (auto it = oracle[metric].MemberBegin();
         it != oracle[metric].MemberEnd(); ++it) {
      auto& samples = values[metric][it->name.GetString()];
      ASSERT_EQ(samples.size(), it->value[0].GetUint64())
          << metric << "/" << it->name.GetString();
      ASSERT_FALSE(samples.empty());
      std::sort(samples.begin(), samples.end());
      double sum = 0;
      for (double value : samples) sum += value;
      EXPECT_NEAR(sum / samples.size(), it->value[1].GetDouble(),
                  std::max(1e-10, std::abs(it->value[1].GetDouble()) * 1e-10));
      size_t column = 2;
      for (double p : {.5, .9, .95, .99}) {
        const double actual = Percentile(samples, p);
        EXPECT_NEAR(actual, it->value[column++].GetDouble(),
                    std::max(1e-10, std::abs(actual) * 1e-10))
            << metric;
      }
    }
  }

  // Entry points verify the Q1..Q11 contract in memory_monitor.h by comparing
  // actual event-derived aggregates with independent JSON truth.
  void VerifyPageMemory(const Json& oracle) {
    // Q1.1-Q1.6: total, BTS and MTS early means/peaks, including short pages.
    VerifyDistribution(oracle, "page_mean");
    VerifyDistribution(oracle, "page_short");
    VerifyDistribution(oracle, "page_peak");
    VerifyDistribution(oracle, "page_peak_short");
    VerifyDistribution(oracle, "page_bts_mean");
    VerifyDistribution(oracle, "page_bts_short");
    VerifyDistribution(oracle, "page_bts_peak");
    VerifyDistribution(oracle, "page_bts_peak_short");
    VerifyDistribution(oracle, "page_mts_mean");
    VerifyDistribution(oracle, "page_mts_short");
    VerifyDistribution(oracle, "page_mts_peak");
    VerifyDistribution(oracle, "page_mts_peak_short");
    auto& means = values["page_mean"];
    ASSERT_TRUE(means.count("media/heavy"));
    ASSERT_FALSE(means.at("media/heavy").empty());
    for (const auto& entry : means) {
      if (entry.first != "media/heavy")
        EXPECT_GT(Percentile(means.at("media/heavy"), .95),
                  Percentile(entry.second, .95))
            << entry.first;
    }
  }

  void VerifyFixedAgeMemory(const Json& oracle) {
    // Q2: count/AVG/P50/P90/P95/P99 at 5s, including MTS on JSC BTS pages.
    VerifyDistribution(oracle, "page_5s");
    VerifyDistribution(oracle, "page_mts_5s");
  }

  void VerifyResidentPages(const Json& oracle) {
    // Lifecycle counts are exact and independent of memory-source coverage.
    VerifySampledSum(oracle, "resident");
    VerifySampledSum(oracle, "resident_weight");
    VerifyAggregateRatio(oracle, "resident", "resident_weight",
                         "resident_fraction");
    for (const char* key : {"app/shell", "global/nav"}) {
      ASSERT_GT(sums["resident_weight"][key], 0);
      EXPECT_EQ(sums["resident"][key], sums["resident_weight"][key]);
    }
    VerifySampledSum(oracle, "resident_bytes");
    VerifySampledSum(oracle, "resident_memory_weight");
    VerifyAggregateRatio(oracle, "resident_bytes", "resident_memory_weight",
                         "resident_mean_bytes");
    for (const char* metric :
         {"vm_reuse", "vm_reuse_weight", "vm_retained", "vm_retained_weight"})
      VerifySampledSum(oracle, metric);
    VerifyAggregateRatio(oracle, "vm_reuse", "vm_reuse_weight",
                         "vm_reuse_mean");
    VerifyAggregateRatio(oracle, "vm_retained", "vm_retained_weight",
                         "vm_retained_mean_bytes");
  }

  void VerifyConcurrentPageKeys(const Json& oracle) {
    // Q4: A/C is mean concurrent bytes; B/C is mean concurrent instance count.
    // This verifies all page-key groups, not a fixed ranking between examples.
    for (const char* key : {"url_bytes", "url_weight", "url_count"})
      VerifySampledSum(oracle, key);
    VerifyAggregateRatio(oracle, "url_bytes", "url_weight", "url_mean_bytes");
    VerifyAggregateRatio(oracle, "url_count", "url_weight",
                         "url_mean_instances");
  }

  void VerifyOldVms(const Json& oracle) {
    // Q5: >=30min heap contribution/mean, plus >=10min idle subset.
    // Stable names in this fixture distinguish runtime types.
    for (const char* key : {"old_vm", "old_vm_weight", "idle_vm"})
      VerifySampledSum(oracle, key);
    VerifyAggregateRatio(oracle, "old_vm", "old_vm_weight",
                         "old_vm_mean_bytes");
  }

  void VerifyResidualPages(const Json& oracle) {
    // Q6: confirmed positive tracked QuickJS page-snapshot contribution/mean.
    VerifySampledSum(oracle, "residual");
    VerifySampledSum(oracle, "residual_weight");
    VerifyAggregateRatio(oracle, "residual", "residual_weight",
                         "residual_mean_bytes");
  }

  void VerifyByteTime(const Json& oracle) {
    // Q7: all-trigger VM/page-key byte-ms, clipped at creation/exit and five
    // minutes.
    VerifySampledSum(oracle, "vm_area");
    VerifySampledSum(oracle, "residual_area");
  }

  void VerifyPssRatios(const Json& oracle) {
    // Q8: compare count, mean and four ratio percentiles for every
    // RAM/milestone. RecordProperty in RunSimulation only exports these
    // already-checked values.
    VerifyDistribution(oracle, "ratios");
  }

  void VerifySlotOverflow(const Json& oracle) {
    // Q9: verify first-overflow VM age and load-count distributions.
    for (const char* key : {"slot_overflow_age", "slot_overflow_loads"}) {
      ASSERT_TRUE(oracle.HasMember(key));
      ASSERT_GT(oracle[key].MemberCount(), 0u) << key;
      VerifyDistribution(oracle, key);
    }
  }

  void VerifyReservedSlots() {
    // Q10: each category provides a raw vm_name-grouped percentile population.
    size_t index = 0;
    for (const char* category : {"unknown", "common", "overflow"}) {
      const auto key = std::string("reserved_") + category;
      ASSERT_FALSE(values[key].empty()) << key;
      for (auto& [name, samples] : values[key]) {
        ASSERT_FALSE(samples.empty()) << key << "/" << name;
        std::sort(samples.begin(), samples.end());
        const auto p50 = Percentile(samples, .50);
        const auto p90 = Percentile(samples, .90);
        const auto p95 = Percentile(samples, .95);
        const auto p99 = Percentile(samples, .99);
        EXPECT_LE(p50, p90) << key << "/" << name;
        EXPECT_LE(p90, p95) << key << "/" << name;
        EXPECT_LE(p95, p99) << key << "/" << name;
      }
      EXPECT_GT(reserved_positive[index++], 0u) << key;
    }
  }

  void VerifyMtsPools(const Json& oracle) {
    for (const char* key :
         {"mts_pool_heap", "mts_pool_runtime", "mts_pool_weight"}) {
      VerifySampledSum(oracle, key);
    }
    VerifyAggregateRatio(oracle, "mts_pool_heap", "mts_pool_weight",
                         "mts_pool_mean_bytes");
    VerifyAggregateRatio(oracle, "mts_pool_runtime", "mts_pool_weight",
                         "mts_pool_mean_runtime_count");
  }
};

}  // namespace simulation
}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_MEMORY_MONITOR_TESTING_MEMORY_SIMULATION_TEST_SUPPORT_H_
