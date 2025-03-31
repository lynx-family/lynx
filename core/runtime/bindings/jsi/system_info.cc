// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/system_info.h"

#include "core/renderer/tasm/config.h"

namespace lynx {
namespace piper {

piper::Object SystemInfo::Bindings(Runtime &rt) {
  piper::Object obj(rt);
  obj.setProperty(rt, "platform",
                  String::createFromAscii(rt, tasm::Config::Platform()));
  obj.setProperty(rt, "pixelWidth",
                  Value(static_cast<double>(tasm::Config::pixelWidth())));
  obj.setProperty(rt, "pixelHeight",
                  Value(static_cast<double>(tasm::Config::pixelHeight())));
  obj.setProperty(rt, "pixelRatio",
                  Value(static_cast<double>(tasm::Config::pixelRatio())));
  obj.setProperty(rt, "osVersion",
                  String::createFromAscii(rt, tasm::Config::GetOsVersion()));
  obj.setProperty(
      rt, "lynxSdkVersion",
      String::createFromAscii(rt, tasm::Config::GetCurrentLynxVersion()));
  obj.setProperty(
      rt, "engineVersion",
      String::createFromAscii(rt, tasm::Config::GetCurrentLynxVersion()));
  return obj;
}

}  // namespace piper
}  // namespace lynx
