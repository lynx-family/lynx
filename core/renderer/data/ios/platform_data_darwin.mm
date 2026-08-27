// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/data/ios/platform_data_darwin.h"

#include "core/renderer/utils/lynx_env.h"

namespace lynx {
namespace tasm {

void PlatformDataDarwin::ConsumeActionsAsync(LynxTemplateData* data) {
  if (data == nil || !LynxEnv::GetInstance().EnablePlatformDataAsyncConsumeActions()) {
    return;
  }
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    [data getDataForJSThread];
  });
}

void PlatformDataDarwin::EnsureConvertData() {
  if (value_converted_from_platform_data_.IsEmpty() && _data != nil) {
    value_converted_from_platform_data_ = [_data getDataForJSThread];
  }
}

void PlatformDataDarwin::ShallowCopy() {
  PlatformData::ShallowCopy();
  LynxTemplateData* source_data = _data;
  _data = [source_data getTemplateDataForJSThread];
  ConsumeActionsAsync(source_data);
}

PlatformDataDarwin::~PlatformDataDarwin() { ConsumeActionsAsync(_data); }

}  // namespace tasm
}  // namespace lynx
