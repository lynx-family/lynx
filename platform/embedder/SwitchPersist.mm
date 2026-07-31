// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <Foundation/Foundation.h>
#include <cstdint>
#include <string>

#include "platform/embedder/switch_persist.h"

namespace {

uint64_t StableHash(const std::string& input) {
  constexpr uint64_t kFnvOffsetBasis = 14695981039346656037ull;
  constexpr uint64_t kFnvPrime = 1099511628211ull;

  uint64_t hash = kFnvOffsetBasis;
  for (unsigned char c : input) {
    hash ^= c;
    hash *= kFnvPrime;
  }
  return hash;
}

std::string GetAppIdentity() {
  @autoreleasepool {
    NSString* bundle_id = [[NSBundle mainBundle] bundleIdentifier];
    NSString* executable_path = [[NSBundle mainBundle] executablePath];
    std::string app_identity;
    if (bundle_id.length > 0) {
      app_identity.append(bundle_id.UTF8String);
    }
    app_identity.push_back('|');
    if (executable_path.length > 0) {
      app_identity.append(executable_path.UTF8String);
    }
    if (app_identity == "|") {
      app_identity = "default";
    }
    return app_identity;
  }
}

NSUserDefaults* GetScopedUserDefaults() {
  static NSUserDefaults* defaults = []() {
    NSString* suite_name =
        [NSString stringWithFormat:@"com.lynx.devtool.%016llx",
                                   static_cast<unsigned long long>(StableHash(GetAppIdentity()))];
    NSUserDefaults* scoped_defaults = [[NSUserDefaults alloc] initWithSuiteName:suite_name];
    return scoped_defaults ?: [NSUserDefaults standardUserDefaults];
  }();
  return defaults;
}

bool ValueExistsInDefaults(NSUserDefaults* defaults, NSString* key) {
  return [defaults objectForKey:key] != nil;
}
}  // namespace

namespace lynx {
namespace embedder {

bool SwitchPersist::GetValueFromPersistent(const std::string& key, bool default_value) {
  NSString* keyStr = [NSString stringWithUTF8String:key.c_str()];
  NSUserDefaults* scoped_defaults = GetScopedUserDefaults();
  if (ValueExistsInDefaults(scoped_defaults, keyStr)) {
    return [scoped_defaults boolForKey:keyStr];
  }

  NSUserDefaults* legacy_defaults = [NSUserDefaults standardUserDefaults];
  if (!ValueExistsInDefaults(legacy_defaults, keyStr)) {
    return default_value;
  }
  return [legacy_defaults boolForKey:keyStr];
}

bool SwitchPersist::SetValueToPersistent(const std::string& key, bool value) {
  [GetScopedUserDefaults() setBool:value forKey:[NSString stringWithUTF8String:key.c_str()]];
  return true;
}

}  // namespace embedder
}  // namespace lynx
