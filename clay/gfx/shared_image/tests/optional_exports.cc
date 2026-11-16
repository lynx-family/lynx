// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <dlfcn.h>

#include <cstdio>
#include <initializer_list>

#include "clay/public/clay.h"
int main() {
  // This base reference anchors clay.cc; no optional API is referenced by
  // address or called directly, so dead stripping is tested independently.
  if (!ClayFenceSyncClientWait(nullptr)) return 1;
  for (const char* name :
       {"ClayFenceSyncServerWait", "ClayFenceSyncExport", "ClayFenceSyncPoll"})
    if (!dlsym(RTLD_DEFAULT, name)) {
      std::fprintf(stderr, "Missing retained export: %s\n", name);
      return 1;
    }
  for (const char* name :
       {"ClayFenceSyncOperate", "ClayFenceSyncEncodeMetalWait",
        "ClayFenceSyncDupNativeFenceFD"})
    if (dlsym(RTLD_DEFAULT, name)) {
      std::fprintf(stderr, "unexpected draft export: %s\n", name);
      return 1;
    }
  std::puts(
      "PASS optional Metal/fence exports survive Apple dead-strip without "
      "strong references");
  return 0;
}
