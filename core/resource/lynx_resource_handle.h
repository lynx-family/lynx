// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RESOURCE_LYNX_RESOURCE_HANDLE_H_
#define CORE_RESOURCE_LYNX_RESOURCE_HANDLE_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/include/expected.h"

namespace lynx {
namespace pub {

class LynxResourceHandle {
 public:
  using Data = std::shared_ptr<const std::vector<uint8_t>>;
  using DataResult = base::expected<Data, std::string>;
  using CreateResult =
      base::expected<std::shared_ptr<LynxResourceHandle>, std::string>;

  virtual ~LynxResourceHandle() = default;

  LynxResourceHandle(const LynxResourceHandle&) = delete;
  LynxResourceHandle& operator=(const LynxResourceHandle&) = delete;
  LynxResourceHandle(LynxResourceHandle&&) = delete;
  LynxResourceHandle& operator=(LynxResourceHandle&&) = delete;

  // Synchronously creates an immutable snapshot. Invalid paths return an
  // error. File read failures are stored in the returned handle and reported
  // when its data is requested.
  static CreateResult CreateFile(std::string file_path);

  // Returns the source path when this handle describes a local file.
  virtual std::optional<std::string> FilePath() const = 0;

  // Returns shared immutable snapshot data without copying it. Implementations
  // that only support the ReadAllBytes hook receive a copying compatibility
  // adapter. A successful result must contain a non-null shared pointer.
  virtual DataResult GetData() const;

  // Returns the immutable snapshot size in bytes, or -1 when the data is
  // unavailable.
  int64_t Size() const;

 protected:
  LynxResourceHandle() = default;

  // Returns an independent copy of the immutable snapshot. An empty resource
  // is a successful empty vector.
  virtual base::expected<std::vector<uint8_t>, std::string> ReadAllBytes()
      const = 0;
};

}  // namespace pub
}  // namespace lynx

#endif  // CORE_RESOURCE_LYNX_RESOURCE_HANDLE_H_
