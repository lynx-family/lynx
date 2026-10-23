// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_DEVTOOL_VM_EVENT_H_
#define CORE_PUBLIC_DEVTOOL_VM_EVENT_H_

#include <cstdint>
#include <string>

namespace lynx {
namespace devtool {

// Shared by the SDK and DevTool. The data field is an opaque string, including
// embedded NUL bytes; JSON encoding belongs to the DevTool protocol boundary.
struct VMEvent {
  std::string type;
  std::string origin;
  std::string target;
  std::string data;
  // Preserved for events from the VM. Incoming events get a fresh timestamp.
  int64_t timestamp = 0;
};

}  // namespace devtool
}  // namespace lynx

#endif  // CORE_PUBLIC_DEVTOOL_VM_EVENT_H_
