// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/timing_handler/timing_utils.h"

#include <list>
#include <string>
#include <unordered_map>

#include "base/include/no_destructor.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"

namespace lynx {
namespace tasm {
namespace timing {

namespace {
std::string camelToSnake(const std::string& camelStr) {
  std::string snakeStr;
  snakeStr.reserve(camelStr.length() * 2);
  for (char ch : camelStr) {
    if (isupper(ch)) {
      if (!snakeStr.empty()) {
        snakeStr += '_';
      }
      snakeStr += tolower(ch);
    } else {
      snakeStr += ch;
    }
  }
  return snakeStr;
}
}  // namespace
/**
 * Converts a given timing key to its corresponding polyfill timing key.
 *
 * This function checks if the provided timing key exists in the predefined
 * key map and returns the corresponding polyfill key. If the key is not
 * found, it falls back to converting the timing key from camel case style to
 * snake case style using the camelToSnake function.
 *
 * The predefined key map contains keys that will be used by the
 * Performance API. The corresponding polyfill keys are designed to be
 * compatible with the classic onSetup/onUpdate API.
 *
 * @param timing_key The input timing key to be converted.
 * @param polyfill_key The output timing key that is populated with the
 * corresponding polyfill key.
 * @return True if a corresponding polyfill timing key was found or created;
 * false if polyfill is not allowed for the given timing key.
 */
bool TryUpdatePolyfillTimingKey(const TimestampKey& timing_key,
                                std::string& polyfill_key) {
  static const lynx::base::NoDestructor<
      std::unordered_map<TimestampKey, TimestampKey>>
      keysAllowedForPolyfill{
          {{kLoadBundleStart, kLoadBundleStartPolyfill},
           {kLoadBundleEnd, kLoadBundleEndPolyfill},
           {kParseStart, kParseStartPolyfill},
           {kParseEnd, kParseEndPolyfill},
           {kResolveStart, kResolveStartPolyfill},
           {kResolveEnd, kResolveEndPolyfill},
           {kCreateVdomStart, kCreateVdomStart},
           {kCreateVdomEnd, kCreateVdomEnd},
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
           {kTemplateBundleParseEnd, kTemplateBundleParseEndPolyfill},
           {kLoadBackgroundStart, kLoadBackgroundStartPolyfill},
           {kLoadBackgroundEnd, kLoadBackgroundEndPolyfill},
           {kReloadBundleFromBts, kReloadBundleFromBtsPolyfill},
           {kPipelineStart, kPipelineStartPolyfill},
           {kPipelineEnd, kPipelineEndPolyfill},
           {kLayoutStart, kLayoutStartPolyfill},
           {kLayoutEnd, kLayoutEndPolyfill},
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
           {kContainerInitEnd, kContainerInitEndPolyfill},
           // polyfill Reload to Load for onTimingSetup
           {kReloadBundleStart, kLoadBundleStartPolyfill},
           {kReloadBundleEnd, kLoadBundleEndPolyfill},
           {kReloadBackgroundStart, kLoadBackgroundStartPolyfill},
           {kReloadBackgroundEnd, kLoadBackgroundEndPolyfill}}};
  static const lynx::base::NoDestructor<std::list<TimestampKey>>
      keysNotAllowedForPolyfill{
          std::initializer_list<TimestampKey>{kMtsRenderStart, kMtsRenderEnd}};

  auto it_allowedPolyfill = keysAllowedForPolyfill->find(timing_key);
  if (it_allowedPolyfill != keysAllowedForPolyfill->end()) {
    polyfill_key = it_allowedPolyfill->second;
    return true;
  }

  auto it_notAllowedPolyfill =
      std::find(keysNotAllowedForPolyfill->begin(),
                keysNotAllowedForPolyfill->end(), timing_key);
  if (it_notAllowedPolyfill != keysNotAllowedForPolyfill->end()) {
    // Polyfill conversion not allowed for these keys - no modification.
    return false;
  }
  // Convert unrecognized camel case keys to snake case to maintain
  // compatibility with TimingAPI callbacks. This approach helps in handling new
  // parameters sent by frontend frameworks that might use camel case by
  // default.
  polyfill_key = camelToSnake(timing_key);
  return true;
}

// Helper function to calculate duration.
TimestampMsFraction CalculateDuration(TimestampMs start_time,
                                      TimestampMs end_time) {
  // Check for abnormal or error cases and handle them.
  if (start_time == 0) {
    if (end_time != 0) {
      // Case 1.start = 0, end != 0,  duration = -1
      return kErrorStartIsZero;
    }
    // Case 2.start == 0, end = 0,  duration = -3
    return kErrorStartAndEndAreZero;
  }
  if (end_time == 0) {
    // Case 3. start != 0, end = 0,  duration = -2
    return kErrorEndIsZero;
  }
  if (start_time > end_time) {
    // Case 4. start > end > 0, duration = -4
    return kErrorStartTimeGreaterThanEndTime;
  }
  // Normal case: both times are valid and start is before end.
  return end_time - start_time;
};

// Helper function to calculate duration.
TimestampMsFraction CalculateMsDuration(TimestampUs start_time,
                                        TimestampUs end_time) {
  // Check for abnormal or error cases and handle them.
  if (start_time == 0) {
    if (end_time != 0) {
      // Case 1.start = 0, end != 0,  duration = -1
      return kErrorStartIsZero;
    }
    // Case 2.start == 0, end = 0,  duration = -3
    return kErrorStartAndEndAreZero;
  }
  if (end_time == 0) {
    // Case 3. start != 0, end = 0,  duration = -2
    return kErrorEndIsZero;
  }
  if (start_time > end_time) {
    // Case 4. start > end > 0, duration = -4
    return kErrorStartTimeGreaterThanEndTime;
  }
  // Normal case: both times are valid and start is before end.
  return ConvertUsToDouble(end_time - start_time);
};

}  // namespace timing
}  // namespace tasm
}  // namespace lynx
