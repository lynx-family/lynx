// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_REPLAY_LYNX_REPLAY_HELPER_H_
#define CORE_SERVICES_REPLAY_LYNX_REPLAY_HELPER_H_
#include <limits>
#include <optional>

#include "core/runtime/jsi/jsi.h"
#include "third_party/rapidjson/document.h"

namespace lynx {

namespace piper {
class ReplayHelper {
 public:
  static piper::Value convertRapidJsonObjectToJSIValue(Runtime& runtime,
                                                       rapidjson::Value& value);
  static piper::Value convertRapidJsonStringToJSIValue(Runtime& runtime,
                                                       rapidjson::Value& value);
  static piper::Value convertRapidJsonNumberToJSIValue(Runtime& runtime,
                                                       rapidjson::Value& value);
  static std::optional<piper::Value> convertRapidJsonLynxValObjectToJSIValue(
      Runtime& runtime, rapidjson::Value& value);
};
}  // namespace piper
}  // namespace lynx
#endif  // CORE_SERVICES_REPLAY_LYNX_REPLAY_HELPER_H_
