// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/gesture/gesture_config.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/public/pub_value.h"

namespace lynx::tasm::gesture {
namespace {

std::optional<GestureCallbackType> CallbackType(const std::string& name) {
  static const std::unordered_map<std::string, GestureCallbackType> callbacks =
      {
          {"onTouchesDown", GestureCallbackType::kTouchesDown},
          {"onTouchesMove", GestureCallbackType::kTouchesMove},
          {"onTouchesUp", GestureCallbackType::kTouchesUp},
          {"onTouchesCancel", GestureCallbackType::kTouchesCancel},
          {"onBegin", GestureCallbackType::kBegin},
          {"onStart", GestureCallbackType::kStart},
          {"onUpdate", GestureCallbackType::kUpdate},
          {"onEnd", GestureCallbackType::kEnd},
      };
  const auto it = callbacks.find(name);
  return it == callbacks.end() ? std::nullopt
                               : std::optional<GestureCallbackType>(it->second);
}

bool IsSupported(GestureType type) {
  switch (type) {
    case GestureType::PAN:
    case GestureType::FLING:
    case GestureType::DEFAULT:
    case GestureType::TAP:
    case GestureType::LONG_PRESS:
    case GestureType::NATIVE:
      return true;
    case GestureType::ROTATION:
    case GestureType::PINCH:
      return false;
  }
}

void ReadNumber(const pub::Value& config, const std::string& key,
                double& target) {
  if (!config.IsMap() || !config.Contains(key)) {
    return;
  }
  auto value = config.GetValueForKey(key);
  if (value && value->IsNumber()) {
    const double number = value->Number();
    if (std::isfinite(number) && number >= 0) {
      target = number;
    }
  }
}

GestureConfig ReadConfig(const GestureDetector& detector) {
  GestureConfig result;
  const auto& config = detector.gesture_config();
  switch (detector.gesture_type()) {
    case GestureType::PAN:
    case GestureType::NATIVE:
      ReadNumber(config, "minDistance", result.min_distance);
      break;
    case GestureType::TAP:
      ReadNumber(config, "maxDistance", result.max_distance);
      ReadNumber(config, "maxDuration", result.max_duration_ms);
      break;
    case GestureType::LONG_PRESS:
      ReadNumber(config, "maxDistance", result.max_distance);
      ReadNumber(config, "minDuration", result.min_duration_ms);
      break;
    case GestureType::DEFAULT:
      ReadNumber(config, "tapSlop", result.tap_slop);
      break;
    case GestureType::FLING:
    case GestureType::ROTATION:
    case GestureType::PINCH:
      break;
  }
  return result;
}

std::vector<uint32_t> Relation(
    const std::unordered_map<std::string, std::vector<uint32_t>>& relations,
    const char* key) {
  const auto it = relations.find(key);
  return it == relations.end() ? std::vector<uint32_t>() : it->second;
}

std::vector<GestureDefinition> NormalizeDefinitions(
    std::vector<GestureDefinition> definitions) {
  std::map<GestureType, GestureDefinition> definitions_by_type;
  for (auto& definition : definitions) {
    if (!IsSupported(definition.gesture_type)) {
      continue;
    }
    const GestureConfig defaults;
    auto sanitize = [](double& value, double fallback) {
      if (!std::isfinite(value) || value < 0) {
        value = fallback;
      }
    };
    sanitize(definition.config.min_distance, defaults.min_distance);
    sanitize(definition.config.max_distance, defaults.max_distance);
    sanitize(definition.config.min_duration_ms, defaults.min_duration_ms);
    sanitize(definition.config.max_duration_ms, defaults.max_duration_ms);
    sanitize(definition.config.tap_slop, defaults.tap_slop);
    std::sort(definition.callbacks.begin(), definition.callbacks.end());
    definition.callbacks.erase(
        std::unique(definition.callbacks.begin(), definition.callbacks.end()),
        definition.callbacks.end());
    auto it = definitions_by_type.find(definition.gesture_type);
    if (it == definitions_by_type.end() ||
        definition.gesture_id < it->second.gesture_id) {
      definitions_by_type.insert_or_assign(definition.gesture_type,
                                           std::move(definition));
    }
  }

  std::vector<GestureDefinition> result;
  result.reserve(definitions_by_type.size());
  for (auto& [type, definition] : definitions_by_type) {
    result.push_back(std::move(definition));
  }
  std::sort(result.begin(), result.end(),
            [](const auto& left, const auto& right) {
              return left.gesture_id < right.gesture_id;
            });
  return result;
}

}  // namespace

std::optional<GestureDefinition> SnapshotGestureDetector(
    const GestureDetector& detector) {
  if (!IsSupported(detector.gesture_type())) {
    return std::nullopt;
  }

  GestureDefinition result;
  result.gesture_id = detector.gesture_id();
  result.gesture_type = detector.gesture_type();
  result.config = ReadConfig(detector);
  const auto& relations = detector.relation_map();
  result.relations.simultaneous = Relation(relations, "simultaneous");
  result.relations.wait_for = Relation(relations, "waitFor");
  result.relations.continue_with = Relation(relations, "continueWith");
  for (const auto& name : detector.gesture_callback_names()) {
    auto callback = CallbackType(name);
    if (callback && std::find(result.callbacks.begin(), result.callbacks.end(),
                              *callback) == result.callbacks.end()) {
      result.callbacks.push_back(*callback);
    }
  }
  return result;
}

std::vector<GestureDefinition> NormalizeGestureDetectors(
    const std::vector<const GestureDetector*>& detectors) {
  std::vector<GestureDefinition> definitions;
  definitions.reserve(detectors.size());
  for (const auto* detector : detectors) {
    if (detector) {
      auto definition = SnapshotGestureDetector(*detector);
      if (definition) {
        definitions.push_back(std::move(*definition));
      }
    }
  }
  return NormalizeDefinitions(std::move(definitions));
}

std::vector<GestureDefinition> NormalizeGestureDefinitions(
    std::vector<GestureDefinition> definitions) {
  return NormalizeDefinitions(std::move(definitions));
}

}  // namespace lynx::tasm::gesture
