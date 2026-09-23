// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_REPLAY_FIXTURE_EVALUATOR_H_
#define CORE_SERVICES_REPLAY_FIXTURE_EVALUATOR_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "base/include/expected.h"
#include "core/services/replay/fixture_common.h"

namespace lynx {
namespace tasm {
namespace replay {

struct FixtureAction {
  std::string function_name;
  int64_t delay_ms = 0;
  std::string params_json;
};

struct FixtureSharedDataEntry {
  std::string key;
  std::string value_json;
};

struct FixtureEvaluationResult {
  std::vector<FixtureAction> actions;
  std::vector<FixtureSharedDataEntry> shared_data;
};

struct FixtureEvaluationLimits {
  // These defaults bound one synchronous evaluation. Callers may provide
  // tighter limits for tests or constrained replay environments.
  int64_t timeout_ms = kDefaultFixtureTimeoutMs;
  size_t memory_limit_bytes = kDefaultFixtureMemoryLimitBytes;
  size_t max_result_entries = 10000;
  size_t max_result_bytes = 16 * 1024 * 1024;
  size_t max_script_bytes = kDefaultFixtureMaxScriptBytes;
  size_t max_asset_bytes = kDefaultFixtureMaxAssetBytes;
};

// Evaluates <fixture_directory>/fixture.js in an isolated runtime and returns
// the initial replay actions and shared data declared by the script. The input
// directory must already be downloaded and extracted by the platform adapter.
base::expected<FixtureEvaluationResult, std::string> EvaluateFixture(
    const std::string& fixture_directory);
base::expected<FixtureEvaluationResult, std::string> EvaluateFixture(
    const std::string& fixture_directory,
    const FixtureEvaluationLimits& limits);

}  // namespace replay
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_REPLAY_FIXTURE_EVALUATOR_H_
