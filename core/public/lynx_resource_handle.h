// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_LYNX_RESOURCE_HANDLE_H_
#define CORE_PUBLIC_LYNX_RESOURCE_HANDLE_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/include/expected.h"
#include "core/base/lynx_export.h"

namespace lynx {
namespace pub {

class LYNX_EXPORT LynxResourceHandle {
 public:
  virtual ~LynxResourceHandle() = default;

  // Creates a reusable handle without opening the file. Returns nullptr unless
  // file_path is a non-empty absolute local filesystem path.
  static std::shared_ptr<LynxResourceHandle> CreateFile(std::string file_path);

  // Returns the source path when this handle describes a local file.
  virtual std::optional<std::string> FilePath() const = 0;

  // Synchronously reads the resource on the calling thread. The handle remains
  // reusable. An empty resource is a successful empty vector.
  virtual base::expected<std::vector<uint8_t>, std::string> ReadAllBytes()
      const = 0;
};

}  // namespace pub
}  // namespace lynx

#endif  // CORE_PUBLIC_LYNX_RESOURCE_HANDLE_H_
