// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_UTILS_DXGI_UTILS_H_
#define CLAY_GFX_SHARED_IMAGE_UTILS_DXGI_UTILS_H_

#include <dxgi.h>
#include <wrl/client.h>

#include "base/include/fml/macros.h"

namespace clay {

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
