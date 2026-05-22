// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_TASK_RUNNERS_H_
#define CLAY_COMMON_TASK_RUNNERS_H_

#include <string>

#include "base/include/fml/macros.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/task_runner.h"

namespace clay {

class TaskRunners {
 public:
  TaskRunners(std::string label, fml::RefPtr<fml::TaskRunner> platform,
              fml::RefPtr<fml::TaskRunner> raster,
              fml::RefPtr<fml::TaskRunner> ui, fml::RefPtr<fml::TaskRunner> io);

  TaskRunners(const TaskRunners& other);

  ~TaskRunners();

  const std::string& GetLabel() const;

  fml::RefPtr<fml::TaskRunner> GetPlatformTaskRunner() const;

  fml::RefPtr<fml::TaskRunner> GetUITaskRunner() const;

  fml::RefPtr<fml::TaskRunner> GetIOTaskRunner() const;

  fml::RefPtr<fml::TaskRunner> GetRasterTaskRunner() const;

  bool IsValid() const;

 private:
  const std::string label_;
  fml::RefPtr<fml::TaskRunner> platform_;
  fml::RefPtr<fml::TaskRunner> raster_;
  fml::RefPtr<fml::TaskRunner> ui_;
  fml::RefPtr<fml::TaskRunner> io_;
};

}  // namespace clay

#endif  // CLAY_COMMON_TASK_RUNNERS_H_
