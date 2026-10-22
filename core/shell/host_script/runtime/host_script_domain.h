// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_DOMAIN_H_
#define CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_DOMAIN_H_

#include <cstddef>

namespace lynx::shell {

// The single execution domain enumeration shared by the process runtime and
// interception. Values index per-domain arrays, so kCount must stay last.
enum class HostScriptDomain { kBTS, kMTS, kUI, kCount };

inline constexpr size_t kHostScriptDomainCount =
    static_cast<size_t>(HostScriptDomain::kCount);

inline constexpr size_t HostScriptDomainIndex(HostScriptDomain domain) {
  return static_cast<size_t>(domain);
}

inline constexpr const char* HostScriptDomainName(HostScriptDomain domain) {
  switch (domain) {
    case HostScriptDomain::kBTS:
      return "bts";
    case HostScriptDomain::kMTS:
      return "mts";
    case HostScriptDomain::kUI:
      return "ui";
    case HostScriptDomain::kCount:
      break;
  }
  return "unknown";
}

}  // namespace lynx::shell

#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_DOMAIN_H_
