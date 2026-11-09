// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_PUBLIC_PATH_IMPL_H_
#define GFX_PUBLIC_PATH_IMPL_H_
#include "gfx/gfx_export.h"
#include "gfx/public/path_types.h"
namespace lynx::gfx {
// Path owns this immutable value directly; it has no separate reference count.
class GFX_EXPORT PathImpl final {
 public:
  explicit PathImpl(PathSource source);
  ~PathImpl();
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
