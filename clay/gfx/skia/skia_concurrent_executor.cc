// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/skia/skia_concurrent_executor.h"

#include "base/trace/native/trace_event.h"

namespace clay {

SkiaConcurrentExecutor::SkiaConcurrentExecutor(const OnWorkCallback& on_work)
    : on_work_(on_work) {}

SkiaConcurrentExecutor::~SkiaConcurrentExecutor() = default;

void SkiaConcurrentExecutor::add(fml::closure work) {
  if (!work) {
    return;
  }
  on_work_([work]() {
    TRACE_EVENT("flutter", "SkiaExecutor");
    work();
  });
}

}  // namespace clay
