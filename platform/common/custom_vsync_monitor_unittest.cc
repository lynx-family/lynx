// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/common/custom_vsync_monitor.h"

#include <atomic>
#include <functional>
#include <future>
#include <thread>
#include <utility>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace platform {
namespace testing {
namespace {

using Callback = base::VSyncMonitorPlatformImpl::Callback;

class Host {
 public:
  std::shared_ptr<CustomVSyncMonitor> CreateMonitor() {
    return std::make_shared<CustomVSyncMonitor>([this](Callback callback) {
      auto reply = std::make_shared<Callback>(std::move(callback));
      replies.push_back(reply);
      if (on_request) {
        on_request(reply);
      }
    });
  }

  std::vector<std::shared_ptr<Callback>> replies;
  std::function<void(std::shared_ptr<Callback>)> on_request;
};

TEST(CustomVSyncMonitorTest, CoalescesConcurrentConsumersAndIsolatesSources) {
  Host first_host;
  Host second_host;
  auto first = first_host.CreateMonitor();
  auto second = second_host.CreateMonitor();
  std::atomic<int> delivered{0};
  std::vector<std::thread> threads;
  for (int i = 0; i < 8; ++i) {
    threads.emplace_back([&] {
      first->RequestVSync([&](int64_t start, int64_t target) {
        EXPECT_EQ(start, 100);
        EXPECT_EQ(target, 200);
        ++delivered;
      });
    });
  }
  for (auto& thread : threads) {
    thread.join();
  }
  int second_delivered = 0;
  second->RequestVSync([&](int64_t, int64_t) { ++second_delivered; });
  ASSERT_EQ(first_host.replies.size(), 1u);
  ASSERT_EQ(second_host.replies.size(), 1u);
  (*first_host.replies[0])(100, 200);
  EXPECT_EQ(delivered, 8);
  EXPECT_EQ(second_delivered, 0);
  (*second_host.replies[0])(300, 400);
  EXPECT_EQ(second_delivered, 1);
}

TEST(CustomVSyncMonitorTest, DuplicateReplyCannotConsumeReentrantRequest) {
  Host host;
  auto monitor = host.CreateMonitor();
  int delivered = 0;
  monitor->RequestVSync([&](int64_t, int64_t) {
    ++delivered;
    monitor->RequestVSync([&](int64_t, int64_t) { ++delivered; });
  });
  auto first_reply = host.replies.front();
  (*first_reply)(100, 200);
  ASSERT_EQ(host.replies.size(), 2u);
  (*first_reply)(100, 200);
  EXPECT_EQ(delivered, 1);
  (*host.replies.back())(200, 300);
  EXPECT_EQ(delivered, 2);
}

TEST(CustomVSyncMonitorTest, ConcurrentDuplicateRepliesDeliverOnce) {
  Host host;
  auto monitor = host.CreateMonitor();
  std::atomic<int> delivered{0};
  monitor->RequestVSync([&](int64_t, int64_t) { ++delivered; });
  auto reply = host.replies.front();
  std::vector<std::thread> threads;
  for (int i = 0; i < 8; ++i) {
    threads.emplace_back([reply] { (*reply)(100, 200); });
  }
  for (auto& thread : threads) {
    thread.join();
  }
  EXPECT_EQ(delivered, 1);
}

TEST(CustomVSyncMonitorTest,
     InlineReplyFreezesNextBatchWithoutReenteringCurrentBatch) {
  Host host;
  auto monitor = host.CreateMonitor();
  std::vector<int> events;
  monitor->RequestVSync([&](int64_t, int64_t) {
    events.push_back(1);
    monitor->RequestVSync([&](int64_t start, int64_t target) {
      EXPECT_EQ(start, 200);
      EXPECT_EQ(target, 300);
      events.push_back(4);
    });
    EXPECT_EQ(host.replies.size(), 2u);
    events.push_back(2);
  });
  monitor->RequestVSync([&](int64_t, int64_t) {
    events.push_back(3);
    monitor->RequestVSync([&](int64_t start, int64_t target) {
      EXPECT_EQ(start, 300);
      EXPECT_EQ(target, 400);
      events.push_back(5);
    });
    EXPECT_EQ(host.replies.size(), 2u);
    (*host.replies.back())(900, 1000);
  });
  host.on_request = [&](auto reply) {
    const int64_t start = host.replies.size() * 100;
    (*reply)(start, start + 100);
  };
  (*host.replies.front())(100, 200);
  EXPECT_EQ(events, (std::vector<int>{1, 2, 3, 4, 5}));
  EXPECT_EQ(host.replies.size(), 3u);
}

TEST(CustomVSyncMonitorTest,
     HostRequestReturningDuringDeliveryRequestsNextSignal) {
  Host host;
  auto monitor = host.CreateMonitor();
  std::promise<std::shared_ptr<Callback>> first_request;
  auto first_reply = first_request.get_future();
  std::promise<void> release_host;
  auto host_can_return = release_host.get_future();
  host.on_request = [&](auto reply) {
    if (host.replies.size() == 1) {
      first_request.set_value(reply);
      host_can_return.wait();
    } else {
      (*reply)(200, 300);
    }
  };
  std::vector<int> events;
  std::thread request_thread([&] {
    monitor->RequestVSync([&](int64_t, int64_t) {
      events.push_back(1);
      monitor->RequestVSync([&](int64_t, int64_t) { events.push_back(4); });
      release_host.set_value();
      request_thread.join();
      EXPECT_EQ(host.replies.size(), 2u);
      events.push_back(2);
    });
  });
  auto reply = first_reply.get();
  monitor->RequestVSync([&](int64_t, int64_t) { events.push_back(3); });
  (*reply)(100, 200);
  EXPECT_EQ(events, (std::vector<int>{1, 2, 3, 4}));
  EXPECT_EQ(host.replies.size(), 2u);
}

TEST(CustomVSyncMonitorTest,
     AsyncReplyBuffersOneFrozenBatchWithoutOverlappingDelivery) {
  Host host;
  auto monitor = host.CreateMonitor();
  std::promise<void> current_started;
  auto current_is_running = current_started.get_future();
  std::promise<void> release_current;
  auto current_can_return = release_current.get_future();
  std::atomic<int> active{0};
  std::atomic<int> completed{0};
  auto deliver = [&](int64_t start, int64_t target, int64_t expected_start) {
    EXPECT_EQ(active.fetch_add(1), 0);
    EXPECT_EQ(start, expected_start);
    EXPECT_EQ(target, expected_start + 100);
    if (expected_start == 200) {
      EXPECT_EQ(host.replies.size(), 3u);
    }
    ++completed;
    --active;
  };
  monitor->RequestVSync([&](int64_t, int64_t) {
    EXPECT_EQ(active.fetch_add(1), 0);
    monitor->RequestVSync(
        [&](int64_t start, int64_t target) { deliver(start, target, 200); });
    current_started.set_value();
    current_can_return.wait();
    ++completed;
    --active;
  });
  auto first_reply = host.replies.front();
  std::thread delivery_thread([first_reply] { (*first_reply)(100, 200); });
  current_is_running.wait();
  EXPECT_EQ(host.replies.size(), 2u);
  auto buffered_reply = host.replies.back();
  (*buffered_reply)(200, 300);
  for (int i = 0; i < 2; ++i) {
    monitor->RequestVSync(
        [&](int64_t start, int64_t target) { deliver(start, target, 300); });
  }
  EXPECT_EQ(host.replies.size(), 2u);
  (*buffered_reply)(900, 1000);
  EXPECT_EQ(completed, 0);
  EXPECT_EQ(active, 1);

  release_current.set_value();
  delivery_thread.join();
  EXPECT_EQ(completed, 2);
  EXPECT_EQ(active, 0);
  ASSERT_EQ(host.replies.size(), 3u);
  (*host.replies.back())(300, 400);
  EXPECT_EQ(completed, 4);
  EXPECT_EQ(active, 0);
}

TEST(CustomVSyncMonitorTest,
     HostRequestReturningAfterDeliveryContinuesNextRequest) {
  Host host;
  auto monitor = host.CreateMonitor();
  std::promise<std::shared_ptr<Callback>> first_request;
  auto first_reply = first_request.get_future();
  std::promise<void> release_host;
  auto host_can_return = release_host.get_future();
  host.on_request = [&](auto reply) {
    if (host.replies.size() == 1) {
      first_request.set_value(reply);
      host_can_return.wait();
    } else {
      (*reply)(200, 300);
    }
  };
  std::vector<int> events;
  std::thread request_thread([&] {
    monitor->RequestVSync([&](int64_t, int64_t) {
      events.push_back(1);
      monitor->RequestVSync([&](int64_t start, int64_t target) {
        EXPECT_EQ(start, 200);
        EXPECT_EQ(target, 300);
        events.push_back(4);
      });
      events.push_back(2);
    });
  });
  auto reply = first_reply.get();
  monitor->RequestVSync([&](int64_t, int64_t) { events.push_back(3); });
  (*reply)(100, 200);
  EXPECT_EQ(events, (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(host.replies.size(), 1u);

  release_host.set_value();
  request_thread.join();
  EXPECT_EQ(events, (std::vector<int>{1, 2, 3, 4}));
  EXPECT_EQ(host.replies.size(), 2u);
}

TEST(CustomVSyncMonitorTest, DestructionReleasesConsumersAndInvalidatesReply) {
  Host host;
  auto monitor = host.CreateMonitor();
  auto consumer = std::make_shared<int>(0);
  std::weak_ptr<int> weak_consumer = consumer;
  monitor->RequestVSync([consumer = std::move(consumer)](int64_t, int64_t) {
    ADD_FAILURE() << "A destroyed source must not deliver a pending callback";
  });
  auto reply = host.replies.front();
  monitor.reset();
  EXPECT_TRUE(weak_consumer.expired());
  (*reply)(100, 200);
  (*reply)(100, 200);
}

TEST(CustomVSyncMonitorTest, CallbackMayReleaseLastExternalOwner) {
  Host host;
  auto monitor = host.CreateMonitor();
  std::weak_ptr<CustomVSyncMonitor> weak = monitor;
  int delivered = 0;
  monitor->RequestVSync([&](int64_t, int64_t) {
    monitor->RequestVSync([&](int64_t, int64_t) { ++delivered; });
    monitor.reset();
    ++delivered;
  });
  monitor->RequestVSync([&](int64_t, int64_t) { ++delivered; });
  host.on_request = [](auto reply) { (*reply)(200, 300); };
  auto reply = host.replies.front();
  (*reply)(100, 200);
  EXPECT_EQ(delivered, 3);
  EXPECT_EQ(host.replies.size(), 2u);
  EXPECT_TRUE(weak.expired());
  (*reply)(100, 200);
  (*host.replies.back())(200, 300);
  EXPECT_EQ(delivered, 3);
}

TEST(CustomVSyncMonitorTest,
     LastOwnerReleaseDeliversBufferedBatchAndClearsWaitingConsumers) {
  Host host;
  auto monitor = host.CreateMonitor();
  std::weak_ptr<CustomVSyncMonitor> weak_monitor = monitor;
  auto waiting_consumer = std::make_shared<int>(0);
  std::weak_ptr<int> weak_waiting_consumer = waiting_consumer;
  int current_delivered = 0;
  int buffered_delivered = 0;
  int waiting_delivered = 0;
  host.on_request = [&](auto reply) {
    if (host.replies.size() == 2) {
      (*reply)(200, 300);
    }
  };

  monitor->RequestVSync([&](int64_t, int64_t) {
    ++current_delivered;
    monitor->RequestVSync([&](int64_t start, int64_t target) {
      EXPECT_EQ(start, 200);
      EXPECT_EQ(target, 300);
      EXPECT_FALSE(weak_waiting_consumer.expired());
      ++buffered_delivered;
    });
    EXPECT_EQ(host.replies.size(), 2u);
    EXPECT_EQ(buffered_delivered, 0);
    monitor->RequestVSync([&, consumer = std::move(waiting_consumer)](
                              int64_t, int64_t) { ++waiting_delivered; });
    EXPECT_EQ(host.replies.size(), 2u);
    monitor.reset();
    EXPECT_FALSE(weak_monitor.expired());
    EXPECT_FALSE(weak_waiting_consumer.expired());
  });

  auto first_reply = host.replies.front();
  (*first_reply)(100, 200);
  EXPECT_EQ(current_delivered, 1);
  EXPECT_EQ(buffered_delivered, 1);
  EXPECT_EQ(waiting_delivered, 0);
  EXPECT_TRUE(weak_monitor.expired());
  EXPECT_TRUE(weak_waiting_consumer.expired());
  ASSERT_EQ(host.replies.size(), 3u);
  (*host.replies.back())(300, 400);
  EXPECT_EQ(waiting_delivered, 0);
}

TEST(CustomVSyncMonitorTest,
     ReleasingCallbackCapturesRequestsEarlyAndDefersDelivery) {
  Host host;
  auto monitor = host.CreateMonitor();
  std::vector<int> events;
  std::vector<int64_t> frame_times;
  int captures_released = 0;
  for (int i = 1; i <= 2; ++i) {
    auto capture = std::shared_ptr<int>(new int(i), [&](int* value) {
      delete value;
      ++captures_released;
      monitor->RequestVSync([&](int64_t start, int64_t target) {
        EXPECT_EQ(captures_released, 2);
        EXPECT_EQ(target, start + 100);
        frame_times.push_back(start);
        events.push_back(3);
      });
    });
    monitor->RequestVSync([&, i, capture = std::move(capture)](
                              int64_t, int64_t) { events.push_back(i); });
  }
  host.on_request = [&](auto reply) {
    const int64_t start = host.replies.size() * 100;
    (*reply)(start, start + 100);
  };
  (*host.replies.front())(100, 200);
  EXPECT_EQ(events, (std::vector<int>{1, 2, 3, 3}));
  EXPECT_EQ(frame_times, (std::vector<int64_t>{200, 300}));
  EXPECT_EQ(host.replies.size(), 3u);
}

TEST(CustomVSyncMonitorTest, SynchronousHostHandlesReentrantRequests) {
  auto monitor = std::make_shared<CustomVSyncMonitor>(
      [](Callback reply) { reply(100, 200); });
  int delivered = 0;
  std::function<void(int64_t, int64_t)> callback;
  callback = [&](int64_t, int64_t) {
    if (++delivered < 10000) {
      monitor->RequestVSync(
          [&](int64_t start, int64_t target) { callback(start, target); });
    }
  };
  monitor->RequestVSync(
      [&](int64_t start, int64_t target) { callback(start, target); });
  EXPECT_EQ(delivered, 10000);
}

TEST(CustomVSyncMonitorTest, NoHostSignalDoesNotDeliverOrFallBack) {
  Host host;
  auto monitor = host.CreateMonitor();
  int delivered = 0;
  monitor->RequestVSync([&](int64_t, int64_t) { ++delivered; });
  monitor->RequestVSync([&](int64_t, int64_t) { ++delivered; });
  EXPECT_EQ(delivered, 0);
  EXPECT_EQ(host.replies.size(), 1u);
  monitor->RequestVSync(nullptr);
  EXPECT_EQ(host.replies.size(), 1u);
  (*host.replies.front())(0, 1);
  EXPECT_EQ(delivered, 2);
}

TEST(CustomVSyncMonitorTest, MissingHostCallbackDoesNotRetainConsumers) {
  auto monitor = std::make_shared<CustomVSyncMonitor>(
      CustomVSyncMonitor::RequestCallback{});
  int delivered = 0;
  for (int i = 0; i < 2; ++i) {
    auto consumer = std::make_shared<int>(0);
    std::weak_ptr<int> weak_consumer = consumer;
    monitor->RequestVSync(
        [&, consumer = std::move(consumer)](int64_t, int64_t) { ++delivered; });
    EXPECT_TRUE(weak_consumer.expired());
    EXPECT_EQ(delivered, 0);
  }
}

}  // namespace
}  // namespace testing
}  // namespace platform
}  // namespace lynx
