// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_TASM_I18N_I18N_BINDER_DARWIN_H_
#define CORE_RENDERER_TASM_I18N_I18N_BINDER_DARWIN_H_

#include "core/renderer/tasm/i18n/i18n.h"

namespace lynx {
namespace tasm {
class I18nBinderDarwin : public I18nBinder {
 public:
  I18nBinderDarwin() = default;
  ~I18nBinderDarwin() = default;
  void Bind(intptr_t ptr) override;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_TASM_I18N_I18N_BINDER_DARWIN_H_
