// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/common/heap_snapshot.h"

#include <utility>

#include "base/include/fml/thread.h"
#include "base/include/no_destructor.h"

namespace lynx {
namespace runtime {

HeapSnapshotRequest::~HeapSnapshotRequest() {
  if (callback_) {
    Fail("Cancelled");
  }
}

void HeapSnapshotRequest::FinishImpl(base::Buffer* buffer, const char* error) {
  std::unique_ptr<base::Buffer> snapshot(buffer);
  static base::NoDestructor<fml::Thread> io_thread(fml::Thread::ThreadConfig(
      "LynxHeapSnapshotIO", fml::Thread::ThreadPriority::LOW));
  io_thread->GetTaskRunner()->PostTask(
      [path = std::move(output_path_), callback = std::move(callback_),
       snapshot = std::move(snapshot), error]() mutable {
        if (!error) {
          if (!snapshot) {
            error = "Capture failed";
          } else if (!snapshot->WriteToFile(path)) {
            error = "Write failed";
          }
        }
        snapshot.reset();
        if (callback) {
          callback(error);
        }
      });
}

}  // namespace runtime
}  // namespace lynx
