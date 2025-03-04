// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/timing_handler/timing_utils.h"

#include <string>
#include <unordered_map>

#include "base/include/no_destructor.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"

namespace lynx {
namespace tasm {
namespace timing {
/**
 * Converts a given timing key to its corresponding polyfill timing key.
 *
 * This function checks if the provided timing key exists in the predefined
 * key map and returns the corresponding polyfill key. If the key is not
 * found, it falls back to converting the timing key from camel case style to
 * snake case style using the GetSnakeStyleKey function.
 *
 * The predefined key map contains keys that will be used by the
 * PerformanceObserver. The corresponding polyfill keys are designed to be
 * compatible with the classic onSetup/onUpdate API.
 *
 * @param timing_key The input timing key to be converted.
 * @return The corresponding polyfill timing key.
 */
TimestampKey GetPolyfillTimingKey(const TimestampKey& timing_key) {
  static const lynx::base::NoDestructor<
      std::unordered_map<TimestampKey, TimestampKey>>
      keyMap{
          {{kLoadBundleStart, kLoadBundleStartPolyfill},
           {kLoadBundleEnd, kLoadBundleEndPolyfill},
           {kParseStart, kParseStartPolyfill},
           {kParseEnd, kParseEndPolyfill},
           {kResolveStart, kResolveStartPolyfill},
           {kResolveEnd, kResolveEndPolyfill},
           {kMtsRenderStart, kMtsRenderStartPolyfill},
           {kMtsRenderEnd, kMtsRenderEndPolyfill},
           {kVmExecuteStart, kVmExecuteStartPolyfill},
           {kVmExecuteEnd, kVmExecuteEndPolyfill},
           {kPaintEnd, kPaintEndPolyfill},
           {kPaintingUiOperationExecuteStart,
            kPaintingUiOperationExecuteStartPolyfill},
           {kPaintingUiOperationExecuteEnd,
            kPaintingUiOperationExecuteEndPolyfill},
           {kLayoutUiOperationExecuteEnd, kLayoutUiOperationExecuteEndPolyfill},
           {kLayoutUiOperationExecuteStart,
            kLayoutUiOperationExecuteStartPolyfill},
           {kTemplateBundleParseStart, kTemplateBundleParseStartPolyfill},
           {kTemplateBundleParseEndPolyfill, kTemplateBundleParseEndPolyfill},
           {kLoadBackgroundStart, kLoadBackgroundStartPolyfill},
           {kLoadBackgroundEnd, kLoadBackgroundEndPolyfill},
           {kReloadFromBackground, kReloadFromBackgroundPolyfill},
           {kPipelineStart, kPipelineStartPolyfill},
           {kPipelineEnd, kPipelineEndPolyfill},
           {kLayoutStart, kLayoutStartPolyfill},
           {kLayoutEnd, kLayoutEndPolyfill},
           {kLynxTTI, kLynxTTIPolyfill},
           {kTotalTTI, kTotalTTIPolyfill},
           {kLynxFCP, kLynxFCPPolyfill},
           {kTotalFCP, kTotalActualFMP},
           {kActualFMP, kActualFMPPolyfill},
           {kLynxActualFMP, kLynxActualFMPPolyfill},
           {kTotalActualFMP, kTotalActualFMPPolyfill},
           {kDataProcessorStart, kDataProcessorStartPolyfill},
           {kDataProcessorEnd, kDataProcessorEndPolyfill},
           {kSetInitDataStart, kSetInitDataStartPolyfill},
           {kSetInitDataEnd, kSetInitDataEndPolyfill},
           {kCreateLynxStart, kCreateLynxStartPolyfill},
           {kCreateLynxEnd, kCreateLynxEndPolyfill},
           {kLoadCoreStart, kLoadCoreStartPolyfill},
           {kLoadCoreEnd, kLoadCoreEndPolyfill},
           {kPrepareTemplateStart, kPrepareTemplateStartPolyfill},
           {kPrepareTemplateEnd, kPrepareTemplateEndPolyfill},
           {kOpenTime, kOpenTimePolyfill},
           {kContainerInitStart, kContainerInitStartPolyfill},
           {kContainerInitEnd, kContainerInitEndPolyfill}}};
  auto it = keyMap->find(timing_key);
  if (it != keyMap->end()) {
    return it->second;
  }
  return timing_key;
}
}  // namespace timing
}  // namespace tasm
}  // namespace lynx
