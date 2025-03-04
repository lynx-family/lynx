// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_IOS_LEPUS_VALUE_CONVERTER_H_
#define CORE_RENDERER_DOM_IOS_LEPUS_VALUE_CONVERTER_H_

#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {

extern id convertLepusValueToNSObject(const lepus_value &value);

}
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_IOS_LEPUS_VALUE_CONVERTER_H_
