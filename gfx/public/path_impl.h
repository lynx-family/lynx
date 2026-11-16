// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_PUBLIC_PATH_IMPL_H_
#define GFX_PUBLIC_PATH_IMPL_H_
#include "gfx/gfx_export.h"
#include "gfx/public/path_types.h"
namespace lynx::gfx {
// The facade owns one implementation. Native realizations remain
// backend-private and are created on the consumer executor, after source-only
// construction.
class GFX_EXPORT PathImpl {
 public:
  enum class BackendKind { kSourceOnly, kHarmony };
  explicit PathImpl(PathSource source);
  virtual ~PathImpl() = 0;
  virtual BackendKind GetBackendKind() const {
    return BackendKind::kSourceOnly;
  }
  PathImpl(const PathImpl&) = delete;
  PathImpl& operator=(const PathImpl&) = delete;
  uint64_t GetKey() const { return key_; }
  PathSourceView GetSourceView() const { return source_.GetSourceView(); }
  const PathSource& GetSource() const { return source_; }

 private:
  const uint64_t key_;
  const PathSource source_;
};
}  // namespace lynx::gfx
#endif  // GFX_PUBLIC_PATH_IMPL_H_
