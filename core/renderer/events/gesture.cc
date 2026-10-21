// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/events/gesture.h"

#include "base/include/log/logging.h"

namespace lynx {
namespace tasm {
namespace {
constexpr char kConfig[] = "config";
}  // namespace

GestureDetectorImpl::GestureDetectorImpl(uint32_t gesture_id,
                                         GestureType gesture_type,
                                         const lepus::Value& callback_config,
                                         const lepus::Value& relation_map,
                                         runtime::MTSRuntime* ctx)
    : gesture_id_(gesture_id),
      gesture_type_(gesture_type),
      gesture_config_(
          PubLepusValue(callback_config.GetProperty(BASE_STATIC_STRING(kConfig))
                            .ToLepusValue())) {
  BASE_STATIC_STRING_DECL(kCallbacks, "callbacks");
  lepus::Value::ForEachLepusValue(
      callback_config.GetProperty(kCallbacks),
      [this, ctx](const lepus::Value&, const lepus::Value& value) {
        BASE_STATIC_STRING_DECL(kName, "name");
        BASE_STATIC_STRING_DECL(kCallback, "callback");
        const auto name = value.GetProperty(kName);
        const auto callback = value.GetProperty(kCallback);
        if (!name.IsString()) {
          LOGW("CreateGestureDetector' "
               << value.Number()
               << " parameter must contain type, and type must be string.");
          return;
        }
        if (!(callback.IsCallable() || callback.IsObject())) {
          LOGW("CreateGestureDetector' " << value.Number()
                                         << " parameter must contain callback, "
                                            "and callback must be callable or "
                                            "object.");
          return;
        }
        if (callback.IsCallable()) {
          gesture_callback_vec_.emplace_back(name.String(), lepus::Value(),
                                             callback, ctx);
        } else {
          gesture_callback_vec_.emplace_back(name.String(), callback, ctx);
        }
      });

  for (const auto& key : {BASE_STATIC_STRING(kGestureWaitFor),
                          BASE_STATIC_STRING(kGestureSimultaneous),
                          BASE_STATIC_STRING(kGestureContinueWith)}) {
    auto& ids = relation_map_[key.str()];
    // PAPI also accepts object collections here; retain its iteration and
    // numeric filtering while ET supplies dense arrays.
    lepus::Value::ForEachLepusValue(
        relation_map.GetProperty(key),
        [&ids](const lepus::Value&, const lepus::Value& value) {
          if (value.IsNumber()) {
            ids.emplace_back(value.Number());
          }
        });
  }
}

}  // namespace tasm
}  // namespace lynx
