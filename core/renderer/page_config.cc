// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/page_config.h"

#include <vector>

namespace lynx {
namespace tasm {

const PageConfig::PageConfigMap<TernaryBool>& PageConfig::GetFuncBoolMap() {
  static const base::NoDestructor<const PageConfigMap<TernaryBool>>
      kPageConfigFuncBoolMap{
          {{"trailNewImage",
            {&PageConfig::SetTrailNewImage, &PageConfig::GetTrailNewImage}},
           {"asyncRedirect",
            {&PageConfig::SetAsyncRedirectUrl,
             &PageConfig::GetAsyncRedirectUrl}},
           {"enableUseMapBuffer",
            {&PageConfig::SetEnableUseMapBuffer,
             &PageConfig::GetEnableUseMapBuffer}},
           {"enableUIOperationOptimize",
            {&PageConfig::SetEnableUIOperationOptimize,
             &PageConfig::GetEnableUIOperationOptimize}},
           {"enableFiberElementForRadonDiff",
            {&PageConfig::SetEnableFiberElementForRadonDiff,
             &PageConfig::GetEnableFiberElementForRadonDiff}},
           {kEnableSignalAPI,
            {&PageConfig::SetEnableSignalAPI,
             &PageConfig::GetEnableSignalAPI}}}};
  return *kPageConfigFuncBoolMap;
}

}  // namespace tasm
}  // namespace lynx
