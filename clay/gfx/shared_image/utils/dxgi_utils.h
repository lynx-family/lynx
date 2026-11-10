// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_UTILS_DXGI_UTILS_H_
#define CLAY_GFX_SHARED_IMAGE_UTILS_DXGI_UTILS_H_

#include <dxgi.h>
#include <wrl/client.h>

#include "base/include/fml/macros.h"
#include "clay/gfx/shared_image/fence_sync.h"

namespace clay {

// Keyed-mutex failures have no fence dependency to retain.
class FailedD3DFenceSync final : public FenceSync {
 public:
  bool ClientWait() override { return false; }
  Type GetType() const override { return Type::kClientWaitOnly; }
  PollResult Poll() override { return PollResult::kFailed; }
};

class ScopedDXGIKeyedMutex {
 public:
  explicit ScopedDXGIKeyedMutex(IDXGIKeyedMutex* keyed_mutex);

  ~ScopedDXGIKeyedMutex();

  bool Valid() const;

  BASE_DISALLOW_COPY_AND_ASSIGN(ScopedDXGIKeyedMutex);

 private:
  Microsoft::WRL::ComPtr<IDXGIKeyedMutex> keyed_mutex_;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_UTILS_DXGI_UTILS_H_
