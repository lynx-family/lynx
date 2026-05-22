// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/testbench_test_replay.h"

#include <cstdio>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <utility>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace replay {
namespace {

class DeferredLayoutObserver : public InspectorCommonObserver {
 public:
  void EndReplayTest(const std::string& file_path) override {
    end_replay_called = true;
    end_replay_file_path = file_path;
  }

  void SendLayoutTree() override { send_layout_tree_called = true; }

  void FlushLayoutTreeForReplayEnd(std::function<void()> callback) override {
    flush_layout_tree_called = true;
    layout_tree_callback = std::move(callback);
  }

  void RunLayoutTreeCallback() {
    if (layout_tree_callback) {
      layout_tree_callback();
    }
  }

  bool send_layout_tree_called = false;
  bool flush_layout_tree_called = false;
  bool end_replay_called = false;
  std::string end_replay_file_path;
  std::function<void()> layout_tree_callback;
};

bool FileExists(const std::string& path) {
  std::ifstream file(path);
  return file.good();
}

std::string ReadFile(const std::string& path) {
  std::ifstream file(path);
  return std::string(std::istreambuf_iterator<char>(file),
                     std::istreambuf_iterator<char>());
}

}  // namespace

TEST(TestBenchTestReplay, EndTestWaitsForLayoutTreeCallback) {
  auto& replay = TestBenchTestReplay::GetInstance();
  auto observer = std::make_shared<DeferredLayoutObserver>();
  const std::string file_path =
      ::testing::TempDir() + "testbench_test_replay_end_test.json";
  std::remove(file_path.c_str());

  replay.SetDevToolObserver(observer);
  replay.StartTest();
  replay.SendFileByAgent("UITree", "{\"name\":\"ui\"}");

  replay.EndTest(file_path);

  EXPECT_FALSE(observer->send_layout_tree_called);
  EXPECT_TRUE(observer->flush_layout_tree_called);
  EXPECT_FALSE(observer->end_replay_called);
  EXPECT_FALSE(FileExists(file_path));
  EXPECT_TRUE(replay.IsStart());

  replay.SendFileByAgent("Layout", "{\"width\":1.0}");
  observer->RunLayoutTreeCallback();

  EXPECT_TRUE(observer->end_replay_called);
  EXPECT_EQ(observer->end_replay_file_path, file_path);
  EXPECT_FALSE(replay.IsStart());
  ASSERT_TRUE(FileExists(file_path));

  std::string content = ReadFile(file_path);
  EXPECT_NE(content.find("\"Layout\""), std::string::npos);
  EXPECT_NE(content.find("\"UITree\""), std::string::npos);

  std::remove(file_path.c_str());
  replay.SetDevToolObserver(nullptr);
}

}  // namespace replay
}  // namespace tasm
}  // namespace lynx
