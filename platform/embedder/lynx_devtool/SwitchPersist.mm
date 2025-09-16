// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <Foundation/Foundation.h>

#include "platform/embedder/lynx_devtool/switch_persist.h"

namespace {

bool ValueExistInPersistent(const std::string& key) {
  if ([[NSUserDefaults standardUserDefaults]
          objectForKey:[NSString stringWithUTF8String:key.c_str()]]) {
    return true;
  } else {
    return false;
  }
}
}  // namespace

namespace lynx {
namespace embedder {

bool SwitchPersist::GetValueFromPersistent(const std::string& key, bool default_value) {
  if (!ValueExistInPersistent(key)) {
    return default_value;
  }
  NSString* keyStr = [NSString stringWithUTF8String:key.c_str()];
  bool value = default_value;
  if ([[NSUserDefaults standardUserDefaults] objectForKey:keyStr]) {
    value = [[NSUserDefaults standardUserDefaults] boolForKey:keyStr];
  }
  return value;
}

bool SwitchPersist::SetValueToPersistent(const std::string& key, bool value) {
  [[NSUserDefaults standardUserDefaults] setBool:value
                                          forKey:[NSString stringWithUTF8String:key.c_str()]];
  return true;
}

}  // namespace embedder
}  // namespace lynx
