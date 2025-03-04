// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_REPLAY_TESTBENCH_TEST_REPLAY_H_
#define CORE_SERVICES_REPLAY_TESTBENCH_TEST_REPLAY_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/include/base_export.h"
#include "base/include/no_destructor.h"
#include "core/inspector/observer/inspector_common_observer.h"

namespace lynx {
namespace tasm {
namespace replay {

class TestBenchTestReplay {
 public:
  static constexpr uint32_t kFileDataBufferSize = 65536;

  static TestBenchTestReplay& GetInstance();

  void SendJsonFile(const char* name, std::string& json);

  void SendFileByAgent(const std::string& type, const std::string& file);

  void SetDevToolObserver(
      const std::shared_ptr<lynx::tasm::InspectorCommonObserver>& observer);

  void StartTest();

  void EndTest(const std::string& file_path);

  bool IsStart() { return is_start_; }

 private:
  void SaveDumpFile(const std::string& filename);
  friend base::NoDestructor<TestBenchTestReplay>;
  TestBenchTestReplay() = default;
  ~TestBenchTestReplay() = default;
  TestBenchTestReplay(const TestBenchTestReplay&) = delete;
  TestBenchTestReplay& operator=(const TestBenchTestReplay&) = delete;

  bool is_start_ = false;
  std::map<std::string, std::vector<std::string>> dump_file_;
  std::shared_ptr<lynx::tasm::InspectorCommonObserver> observer_;
};

}  // namespace replay
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_REPLAY_TESTBENCH_TEST_REPLAY_H_
