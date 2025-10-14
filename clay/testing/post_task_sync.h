// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef TESTING_POST_TASK_SYNC_H_
#define TESTING_POST_TASK_SYNC_H_

#include "base/include/fml/task_runner.h"

namespace clay {
namespace testing {

void PostTaskSync(const fml::RefPtr<fml::TaskRunner>& task_runner,
                  const std::function<void()>& function);

}  // namespace testing
}  // namespace clay

#endif  // TESTING_POST_TASK_SYNC_H_
