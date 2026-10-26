// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <atomic>
#include <future>
#include <thread>
#include <vector>

#include "clay/common/thread_host_holder.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

TEST(ThreadHostHolderTest, ConcurrentConfigurationReusesThreads) {
  std::atomic<size_t> raster_creations{0};
  std::atomic<size_t> ui_creations{0};
  ThreadConfigSetter setter = [&](const ThreadConfig& config) {
    if (config.name == "clay.concurrent.raster") {
      ++raster_creations;
    } else if (config.name == "clay.concurrent.ui") {
      ++ui_creations;
    }
    fml::Thread::SetCurrentThreadName(config);
  };
  ThreadHostHolder holder;
  EXPECT_FALSE(holder.GetTaskRunner(ThreadHost::RASTER));
  EXPECT_FALSE(holder.GetTaskRunner(ThreadHost::UI));
  constexpr size_t kWriters = 2;
  std::vector<std::shared_ptr<ThreadHost>> hosts(kWriters);
  std::vector<fml::RefPtr<fml::TaskRunner>> rasters(kWriters);
  std::atomic<size_t> initialized{0};
  std::atomic<size_t> remaining{kWriters};
  std::atomic<bool> invalid_read{false};
  std::promise<void> reader_started;
  auto start = reader_started.get_future().share();
  std::vector<std::thread> writers;
  for (size_t i = 0; i < kWriters; ++i) {
    writers.emplace_back([&, i] {
      start.wait();
      holder.SetMask(ThreadHost::RASTER, ".concurrent", setter);
      hosts[i] = holder.GetThreadHost();
      rasters[i] = holder.GetTaskRunner(ThreadHost::RASTER);
      ++initialized;
      while (initialized.load() != kWriters) {
        std::this_thread::yield();
      }
      holder.SetMask(ThreadHost::RASTER | ThreadHost::UI, ".concurrent",
                     setter);
      --remaining;
    });
  }
  std::thread reader([&] {
    reader_started.set_value();
    do {
      auto ui = holder.GetTaskRunner(ThreadHost::UI);
      auto raster = holder.GetTaskRunner(ThreadHost::RASTER);
      if (ui && !raster) {
        invalid_read = true;
      }
    } while (remaining.load() != 0);
  });
  for (auto& writer : writers) {
    writer.join();
  }
  reader.join();

  auto host = holder.GetThreadHost();
  auto raster = holder.GetTaskRunner(ThreadHost::RASTER);
  auto ui = holder.GetTaskRunner(ThreadHost::UI);
  ASSERT_TRUE(raster);
  ASSERT_TRUE(ui);
  raster->PostSyncTask([] {});
  ui->PostSyncTask([] {});
  EXPECT_FALSE(invalid_read.load());
  EXPECT_EQ(raster_creations.load(), 1u);
  EXPECT_EQ(ui_creations.load(), 1u);
  for (size_t i = 0; i < kWriters; ++i) {
    EXPECT_EQ(hosts[i], host);
    EXPECT_EQ(rasters[i], raster);
  }
}

TEST(ThreadHostHolderTest, IndependentHoldersShareTheIOThread) {
  constexpr size_t kHolders = 4;
  std::vector<std::shared_ptr<ThreadHost>> hosts(kHolders);
  std::vector<std::thread> workers;
  std::atomic<size_t> ready{0};
  std::promise<void> start;
  auto started = start.get_future().share();
  for (size_t i = 0; i < kHolders; ++i) {
    workers.emplace_back([&, i] {
      ThreadHostHolder holder;
      if (i % 2) {
        holder.SetMask(ThreadHost::RASTER);
      }
      ++ready;
      started.wait();
      holder.SetMask(ThreadHost::IO);
      hosts[i] = holder.GetThreadHost();
    });
  }
  while (ready.load() != kHolders) {
    std::this_thread::yield();
  }
  start.set_value();
  for (auto& worker : workers) {
    worker.join();
  }
  ASSERT_NE(hosts.front()->io_thread, nullptr);
  for (const auto& host : hosts) {
    EXPECT_EQ(host->io_thread, hosts.front()->io_thread);
  }
}

}  // namespace clay
