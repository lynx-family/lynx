// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/testing/post_task_sync.h"

#include "base/include/fml/synchronization/waitable_event.h"

namespace clay {
namespace testing {

void PostTaskSync(const fml::RefPtr<fml::TaskRunner>& task_runner,
                  const std::function<void()>& function) {
  fml::AutoResetWaitableEvent latch;
  task_runner->PostTask([&] {
    function();
    latch.Signal();
  });
  latch.Wait();
}

}  // namespace testing
}  // namespace clay
