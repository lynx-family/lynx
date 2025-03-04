// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_SSR_CLIENT_SSR_HYDRATE_INFO_H_
#define CORE_SERVICES_SSR_CLIENT_SSR_HYDRATE_INFO_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "core/renderer/dom/fiber/fiber_element.h"

namespace lynx {
namespace tasm {

// A structure holding the status before ssr page get hydrated.
struct SSRHydrateInfo {
  bool hydrate_data_identical_as_ssr_ = false;

  // Not applicable for Radon TTML now.
  bool waiting_for_hydrating_ = false;
  // hydrate info only for reactlynx ssr now.
  std::string custom_hydrate_info_ = "";
  // ssr list element.
  std::vector<fml::RefPtr<FiberElement>> list_node_ref_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_SSR_CLIENT_SSR_HYDRATE_INFO_H_
