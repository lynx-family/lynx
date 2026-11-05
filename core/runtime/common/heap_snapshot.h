// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_COMMON_HEAP_SNAPSHOT_H_
#define CORE_RUNTIME_COMMON_HEAP_SNAPSHOT_H_

#include <memory>
#include <string>
#include <utility>

#include "base/include/buffer.h"
#include "base/include/closure.h"

namespace lynx {
namespace runtime {

// Errors must have static storage duration. A null error means that the
// complete snapshot was written successfully.
using HeapSnapshotCallback = base::MoveOnlyClosure<void, const char*>;

// Move the unique owner through task/actor queues. Dropping a queued request
// reports failure without keeping the page or its VM alive.
class HeapSnapshotRequest {
 public:
  HeapSnapshotRequest(std::string&& output_path,
                      HeapSnapshotCallback&& callback)
      : output_path_(std::move(output_path)), callback_(std::move(callback)) {}
  ~HeapSnapshotRequest();
  HeapSnapshotRequest(const HeapSnapshotRequest&) = delete;
  HeapSnapshotRequest& operator=(const HeapSnapshotRequest&) = delete;

  void Fail(const char* error) { FinishImpl(nullptr, error); }
  // The buffer must own its bytes and allow destruction off the VM thread.
  // File IO and completion always run on the snapshot IO thread.
  void Finish(std::unique_ptr<base::Buffer> snapshot, const char* error) {
    FinishImpl(snapshot.release(), error);
  }

 private:
  // Takes ownership of buffer.
  void FinishImpl(base::Buffer* buffer, const char* error);

  std::string output_path_;
  HeapSnapshotCallback callback_;
};

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_COMMON_HEAP_SNAPSHOT_H_
