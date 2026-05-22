// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/utils/dxgi_utils.h"

#include "clay/fml/logging.h"

namespace clay {
ScopedDXGIKeyedMutex::ScopedDXGIKeyedMutex(IDXGIKeyedMutex* keyed_mutex)
    : keyed_mutex_(keyed_mutex) {
  FML_DCHECK(keyed_mutex_);
  HRESULT result = keyed_mutex_->AcquireSync(0, INFINITE);
  if (result != S_OK) {
    // Maybe failed or abandoned
    FML_LOG(ERROR) << "Failed to acquire keyed mutex, result: " << result;
    keyed_mutex_.Reset();
  }
}

ScopedDXGIKeyedMutex::~ScopedDXGIKeyedMutex() {
  if (keyed_mutex_) {
    if (FAILED(keyed_mutex_->ReleaseSync(0))) {
      FML_LOG(ERROR) << "Failed to release keyed mutex";
    }
  }
}
}  // namespace clay
