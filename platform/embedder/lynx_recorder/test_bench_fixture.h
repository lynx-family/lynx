// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_LYNX_RECORDER_TEST_BENCH_FIXTURE_H_
#define PLATFORM_EMBEDDER_LYNX_RECORDER_TEST_BENCH_FIXTURE_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "clay/fml/file.h"

namespace lynx {
namespace embedder {

// Shared by the replay owner and the JS module lease. Assets are removed only
// after both owners release them, including on failed initialization.
class TestBenchFixture {
 public:
  static bool IsArchive(const std::string& bytes);
  static std::shared_ptr<TestBenchFixture> Open(const std::string& bytes,
                                                std::string* error);
  const std::string& directory() const { return directory_.path(); }
  const std::string& record() const { return record_; }
  const std::string* Template(const std::string& asset) const;

 private:
  bool Extract(const std::string& bytes);
  bool Evaluate(std::string* error);
  ::fml::ScopedTemporaryDirectory directory_;
  std::string record_;
  std::unordered_map<std::string, std::string> templates_;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_LYNX_RECORDER_TEST_BENCH_FIXTURE_H_
