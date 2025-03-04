// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DATA_IOS_PLATFORM_DATA_DARWIN_H_
#define CORE_RENDERER_DATA_IOS_PLATFORM_DATA_DARWIN_H_

#import <Lynx/LynxTemplateData+Converter.h>

#include <memory>
#include <string>

#include "core/renderer/data/platform_data.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {
class PlatformDataDarwin : public PlatformData {
 public:
  PlatformDataDarwin(LynxTemplateData* data) : _data(data) {}

  virtual ~PlatformDataDarwin() override = default;

 private:
  void EnsureConvertData() override;

  LynxTemplateData* _data;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DATA_IOS_PLATFORM_DATA_DARWIN_H_
