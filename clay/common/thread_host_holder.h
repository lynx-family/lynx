// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_THREAD_HOST_HOLDER_H_
#define CLAY_COMMON_THREAD_HOST_HOLDER_H_

#include <memory>
#include <mutex>
#include <string>

#include "clay/common/thread_host.h"

namespace clay {

// Singleton to contain ThreadHost.
class ThreadHostHolder {
 public:
  static ThreadHostHolder& Instance();

  ThreadHostHolder();

  void SetMask(
      uint64_t mask, const std::string& thread_label = "",
      const ThreadConfigSetter& setter = fml::Thread::SetCurrentThreadName);

  void SetConfig(const clay::ThreadHost::ThreadHostConfig& config);

  // Retain thread ownership. Use GetTaskRunner for thread access while the
  // configuration can be extended by another caller.
  const std::shared_ptr<ThreadHost> GetThreadHost() const;

  // Copy runners under the configuration lock instead of reading ThreadHost's
  // thread members while another caller may be adding threads.
  fml::RefPtr<fml::TaskRunner> GetTaskRunner(ThreadHost::Type type) const;

 private:
  mutable std::mutex mutex_;
  std::shared_ptr<ThreadHost> thread_host_;
};

}  // namespace clay

#endif  // CLAY_COMMON_THREAD_HOST_HOLDER_H_
