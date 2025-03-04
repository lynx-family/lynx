// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_THREAD_H_
#define BASE_INCLUDE_FML_THREAD_H_

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "base/include/base_export.h"
#include "base/include/fml/macros.h"
#include "base/include/fml/message_loop_impl.h"
#include "base/include/fml/task_runner.h"

namespace lynx {
namespace fml {

class Thread {
 public:
  /// Valid values for priority of Thread.
  enum class ThreadPriority : int {
    /// Suitable for threads that shouldn't disrupt high priority work.
    /// The default background priority is the same as low, you can adjust it
    /// with the ThreadConfigSetter defined by yourself
    BACKGROUND,
    /// Suitable for threads that shouldn't disrupt high priority work.
    LOW,
    /// Default priority level.
    NORMAL,
    /// Suitable for threads which execute for runtime engine、layout
    /// engine、template render.
    HIGH,
  };

  /// The ThreadConfig is the thread info include thread name, thread priority.
  struct ThreadConfig {
    ThreadConfig(const std::string& name, ThreadPriority priority,
                 std::shared_ptr<base::closure> additional_setup_closure)
        : name(name),
          priority(priority),
          additional_setup_closure(std::move(additional_setup_closure)) {}

    ThreadConfig(const std::string& name, ThreadPriority priority)
        : ThreadConfig(name, priority, nullptr) {}

    explicit ThreadConfig(const std::string& name)
        : ThreadConfig(name, ThreadPriority::NORMAL, nullptr) {}

    ThreadConfig() : ThreadConfig("", ThreadPriority::NORMAL, nullptr) {}

    std::string name;
    ThreadPriority priority;
    std::shared_ptr<base::closure>
        additional_setup_closure;  // thread config should be copyable
  };

  using ThreadConfigSetter = std::function<void(const ThreadConfig&)>;

  explicit Thread(const std::string& name = "");

  explicit Thread(const ThreadConfig& config);

  explicit Thread(const ThreadConfigSetter& setter,
                  const ThreadConfig& config = ThreadConfig());

  ~Thread();

  const fml::RefPtr<fml::TaskRunner>& GetTaskRunner() const;

  void Join();

  const fml::RefPtr<fml::MessageLoopImpl>& GetLoop() const;

  static void SetCurrentThreadName(const ThreadConfig& config);

 private:
  std::unique_ptr<std::thread> thread_;

  fml::RefPtr<fml::TaskRunner> task_runner_;

  fml::RefPtr<fml::MessageLoopImpl> loop_;

  std::atomic_bool joined_;

  BASE_DISALLOW_COPY_AND_ASSIGN(Thread);
};

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::Thread;
}  // namespace fml

#endif  // BASE_INCLUDE_FML_THREAD_H_
