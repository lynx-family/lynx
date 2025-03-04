// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/ui_wrapper/common/android/prop_bundle_android.h"

#include <memory>
#include <utility>

#include "core/base/js_constants.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/events/gesture.h"
#include "core/renderer/tasm/react/android/mapbuffer/readable_map_buffer.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace tasm {

PropBundleAndroid::PropBundleAndroid(bool use_map_buffer)
    : jni_map_(new base::android::JavaOnlyMap),
      jni_event_handler_map_(nullptr),
      use_map_buffer_(use_map_buffer) {}

PropBundleAndroid::PropBundleAndroid(
    const std::shared_ptr<base::android::JavaOnlyMap>& jni_map,
    const std::shared_ptr<base::android::JavaOnlyArray>& jni_event_handler_map)
    : jni_map_(jni_map), jni_event_handler_map_(jni_event_handler_map) {}

void PropBundleAndroid::SetNullProps(const char* key) {
  CopyIfConst();
  jni_map_->PushNull(key);
}

void PropBundleAndroid::SetProps(const char* key, uint value) {
  CopyIfConst();
  jni_map_->PushInt(key, value);
}

void PropBundleAndroid::SetProps(const char* key, int value) {
  CopyIfConst();
  jni_map_->PushInt(key, value);
}

void PropBundleAndroid::SetProps(const char* key, const char* value) {
  CopyIfConst();
  jni_map_->PushString(key, value);
}

void PropBundleAndroid::SetProps(const char* key, bool value) {
  CopyIfConst();
  jni_map_->PushBoolean(key, value);
}

void PropBundleAndroid::SetProps(const char* key, double value) {
  CopyIfConst();
  jni_map_->PushDouble(key, value);
}

void PropBundleAndroid::SetProps(const char* key, const pub::Value& value) {
  CopyIfConst();
  AssembleMap(jni_map_.get(), key, value);
}

void PropBundleAndroid::SetProps(const pub::Value& value) {
  CopyIfConst();
  auto prev_value_vector =
      pub::ScopedCircleChecker::InitVectorIfNecessary(value);
  value.ForeachMap([jni_map_ptr = jni_map_.get(), &prev_value_vector](
                       const pub::Value& k, const pub::Value& v) {
    AssembleMap(jni_map_ptr, k.str().c_str(), v, prev_value_vector.get(), 1,
                false);
  });
}

bool PropBundleAndroid::Contains(const char* key) const {
  return jni_map_->Contains(key);
}

// styles
void PropBundleAndroid::SetNullPropsByID(CSSPropertyID id) {
  if (!use_map_buffer_) {
    const auto& property_name = CSSProperty::GetPropertyName(id);
    SetNullProps(property_name.c_str());
  } else {
    style_buffer_builder_.putNull(static_cast<uint16_t>(id));
  }
}

void PropBundleAndroid::SetPropsByID(CSSPropertyID id, unsigned int value) {
  if (!use_map_buffer_) {
    const auto& property_name = CSSProperty::GetPropertyName(id);
    SetProps(property_name.c_str(), value);
  } else {
    style_buffer_builder_.putLong(static_cast<uint16_t>(id), value);
  }
}

void PropBundleAndroid::SetPropsByID(CSSPropertyID id, int value) {
  if (!use_map_buffer_) {
    const auto& property_name = CSSProperty::GetPropertyName(id);
    SetProps(property_name.c_str(), value);
  } else {
    style_buffer_builder_.putInt(static_cast<uint16_t>(id), value);
  }
}

void PropBundleAndroid::SetPropsByID(CSSPropertyID id, bool value) {
  if (!use_map_buffer_) {
    const auto& property_name = CSSProperty::GetPropertyName(id);
    SetProps(property_name.c_str(), value);
  } else {
    style_buffer_builder_.putBool(static_cast<uint16_t>(id), value);
  }
}

void PropBundleAndroid::SetPropsByID(CSSPropertyID id, double value) {
  if (!use_map_buffer_) {
    const auto& property_name = CSSProperty::GetPropertyName(id);
    SetProps(property_name.c_str(), value);
  } else {
    style_buffer_builder_.putDouble(static_cast<uint16_t>(id), value);
  }
}

void PropBundleAndroid::SetPropsByID(CSSPropertyID id, const char* value) {
  if (!use_map_buffer_) {
    const auto& property_name = CSSProperty::GetPropertyName(id);
    SetProps(property_name.c_str(), value);
  } else {
    style_buffer_builder_.putString(static_cast<uint16_t>(id), value);
  }
}

void PropBundleAndroid::SetPropsByID(CSSPropertyID id,
                                     const pub::Value& value) {
  if (!use_map_buffer_) {
    const auto& property_name = CSSProperty::GetPropertyName(id);
    SetProps(property_name.c_str(), value);
  } else {
    AssembleMapBuffer(style_buffer_builder_, static_cast<uint16_t>(id), value);
  }
}

void PropBundleAndroid::SetEventHandler(const pub::Value& event) {
  CopyIfConst();
  if (!jni_event_handler_map_) {
    jni_event_handler_map_ = std::make_unique<base::android::JavaOnlyArray>();
  }
  constexpr const static char* kName = "name";
  constexpr const static char* kType = "type";
  constexpr const static char* kFunction = "function";
  constexpr const static char* kLepusType = "lepusType";

  auto event_array = pub::ValueUtils::ConvertValueToLepusArray(event).Array();
  auto name = event_array->get(0).String();
  auto type = event_array->get(1).String();
  auto is_js_event = event_array->get(2).Bool();
  auto function = event_array->get(3).String();

  auto jni_map = std::make_unique<base::android::JavaOnlyMap>();
  jni_map->PushString(kName, name.c_str());
  if (is_js_event) {
    jni_map->PushString(kType, type.c_str());
    if (!function.empty()) {
      jni_map->PushString(kFunction, function.c_str());
    }
  } else {
    jni_map->PushString(kLepusType, type.c_str());
  }
  jni_event_handler_map_->PushMap(jni_map.get());
}

/**
 * Sets the gesture detector in the Android platform.
 *
 * @param detector The gesture detector object to be set.
 */
void PropBundleAndroid::SetGestureDetector(const GestureDetector& detector) {
  // If the map for event handlers does not exist yet, create a new one.
  if (!jni_gesture_detector_map_) {
    jni_gesture_detector_map_ =
        std::make_unique<base::android::JavaOnlyArray>();
  }

  // Constants for keys used in the map.
  constexpr const static char* kId = "id";
  constexpr const static char* kType = "type";
  constexpr const static char* kCallbackNames = "callbackNames";
  constexpr const static char* kRelationMap = "relationMap";
  constexpr const static char* kConfigMap = "configMap";

  // Create a new map for the gesture detector.
  auto jni_map = std::make_unique<base::android::JavaOnlyMap>();

  // Add the gesture ID and type to the map.
  jni_map->PushInt(kId, detector.gesture_id());
  jni_map->PushInt(kType, static_cast<int>(detector.gesture_type()));

  // Add the gesture callback names to an array and add the array to the map.
  auto jni_callback_names = std::make_unique<base::android::JavaOnlyArray>();
  for (const auto& callback : detector.gesture_callbacks()) {
    jni_callback_names->PushString(callback.name_.str());
  }
  jni_map->PushArray(kCallbackNames, std::move(jni_callback_names.get()));

  AssembleMap(jni_map.get(), kConfigMap,
              pub::ValueImplLepus(detector.gesture_config()));

  // Create a new map for gesture relations.
  auto jni_relation_map = std::make_unique<base::android::JavaOnlyMap>();

  // Add the IDs of the gestures that can be performed simultaneously to an
  // array and add the array to the map.
  const auto& relation_map = detector.relation_map();
  auto jni_simultaneous_array =
      std::make_unique<base::android::JavaOnlyArray>();
  for (const auto& id : relation_map.at("simultaneous")) {
    jni_simultaneous_array->PushInt(id);
  }
  jni_relation_map->PushArray("simultaneous",
                              std::move(jni_simultaneous_array.get()));

  // Add the IDs of the gestures that must be performed before this gesture to
  // an array and add the array to the map.
  auto jni_wait_for_array = std::make_unique<base::android::JavaOnlyArray>();
  for (const auto& id : relation_map.at("waitFor")) {
    jni_wait_for_array->PushInt(id);
  }
  jni_relation_map->PushArray("waitFor", std::move(jni_wait_for_array.get()));

  // Add the IDs of the gestures that must be performed before this gesture to
  // an array and add the array to the map.
  auto jni_continue_with_array =
      std::make_unique<base::android::JavaOnlyArray>();
  for (const auto& id : relation_map.at("continueWith")) {
    jni_continue_with_array->PushInt(id);
  }
  jni_relation_map->PushArray("continueWith",
                              std::move(jni_continue_with_array.get()));

  // Add the relation map to the gesture detector map.
  jni_map->PushMap(kRelationMap, std::move(jni_relation_map.get()));

  // Add the gesture detector map to the event handler map.
  jni_gesture_detector_map_->PushMap(std::move(jni_map.get()));
}

void PropBundleAndroid::ResetEventHandler() {
  CopyIfConst();
  if (jni_event_handler_map_) {
    jni_event_handler_map_->Clear();
  }
}

// TODO(songshourui.null): add unit test for this function when JNI method call
// be execd in JAVA UT.
void PropBundleAndroid::AssembleMap(
    base::android::JavaOnlyMap* map, const char* key, const pub::Value& value,
    std::vector<std::unique_ptr<pub::Value>>* prev_value_vector, int depth,
    bool need_handle_null_value) {
  if (value.IsNil()) {
    // When PropBundle is passed as an argument to InvokeUIMethod, the js null
    // value will be ignored during data conversion
    if (!need_handle_null_value) {
      return;
    }
    map->PushNull(key);
  } else if (value.IsString()) {
    map->PushString(key, value.str().c_str());
  } else if (value.IsInt32()) {
    map->PushInt(key, value.Int32());
  } else if (value.IsInt64()) {
    map->PushInt64(key, value.Int64());
  } else if (value.IsUInt32()) {
    map->PushInt64(key, value.UInt32());
  } else if (value.IsUInt64()) {
    map->PushInt64(key, value.UInt64());
  } else if (value.IsNumber()) {
    map->PushDouble(key, value.Number());
  } else if (value.IsArrayBuffer()) {
    map->PushByteArray(key, value.ArrayBuffer(), value.Length());
  } else if (value.IsArray()) {
    pub::ScopedCircleChecker scoped_circle_checker;
    if (!scoped_circle_checker.CheckCircleOrCacheValue(prev_value_vector, value,
                                                       depth)) {
      auto jni_array = std::make_unique<base::android::JavaOnlyArray>();
      value.ForeachArray(
          [jni_array_ptr = jni_array.get(), &prev_value_vector, depth,
           need_handle_null_value](int64_t index, const pub::Value& v) {
            AssembleArray(jni_array_ptr, v, prev_value_vector, depth + 1,
                          need_handle_null_value);
          });
      map->PushArray(key, jni_array.get());
    }
  } else if (value.IsMap()) {
    pub::ScopedCircleChecker scoped_circle_checker;
    if (!scoped_circle_checker.CheckCircleOrCacheValue(prev_value_vector, value,
                                                       depth)) {
      auto jni_map = std::make_unique<base::android::JavaOnlyMap>();
      value.ForeachMap(
          [jni_map_ptr = jni_map.get(), &prev_value_vector, depth,
           need_handle_null_value](const pub::Value& k, const pub::Value& v) {
            AssembleMap(jni_map_ptr, k.str().c_str(), v, prev_value_vector,
                        depth + 1, need_handle_null_value);
          });
      map->PushMap(key, jni_map.get());
    }
  } else if (value.IsBool()) {
    map->PushBoolean(key, value.Bool());
  } else if (value.IsUndefined()) {
    // Todo(songshourui): handle undefined value later
  } else {
    LOGE("PropBundleAndroid::AssembleMap unsupported type :"
         << value.Type() << " key :" << key);
  }
}

// TODO(songshourui.null): add unit test for this function when JNI method call
// be execd in JAVA UT.
void PropBundleAndroid::AssembleArray(
    base::android::JavaOnlyArray* array, const pub::Value& value,
    std::vector<std::unique_ptr<pub::Value>>* prev_value_vector, int depth,
    bool need_handle_null_value) {
  if (value.IsNil()) {
    if (!need_handle_null_value) {
      return;
    }
    array->PushNull();
  } else if (value.IsString()) {
    array->PushString(value.str());
  } else if (value.IsInt32()) {
    array->PushInt(value.Int32());
  } else if (value.IsInt64()) {
    array->PushInt64(value.Int64());
  } else if (value.IsUInt32()) {
    array->PushInt64(value.UInt32());
  } else if (value.IsUInt64()) {
    array->PushInt64(value.UInt64());
  } else if (value.IsNumber()) {
    array->PushDouble(value.Number());
  } else if (value.IsArray()) {
    pub::ScopedCircleChecker scoped_circle_checker;
    if (!scoped_circle_checker.CheckCircleOrCacheValue(prev_value_vector, value,
                                                       depth)) {
      auto jni_array = std::make_unique<base::android::JavaOnlyArray>();
      value.ForeachArray(
          [jni_array_ptr = jni_array.get(), &prev_value_vector, depth,
           need_handle_null_value](int64_t index, const pub::Value& v) {
            AssembleArray(jni_array_ptr, v, prev_value_vector, depth + 1,
                          need_handle_null_value);
          });
      array->PushArray(jni_array.get());
    }
  } else if (value.IsMap()) {
    pub::ScopedCircleChecker scoped_circle_checker;
    if (!scoped_circle_checker.CheckCircleOrCacheValue(prev_value_vector, value,
                                                       depth)) {
      auto jni_map = std::make_unique<base::android::JavaOnlyMap>();
      value.ForeachMap(
          [jni_map_ptr = jni_map.get(), &prev_value_vector, depth,
           need_handle_null_value](const pub::Value& k, const pub::Value& v) {
            AssembleMap(jni_map_ptr, k.str().c_str(), v, prev_value_vector,
                        depth + 1, need_handle_null_value);
          });
      array->PushMap(jni_map.get());
    }
  } else if (value.IsArrayBuffer()) {
    array->PushByteArray(value.ArrayBuffer(), value.Length());
  } else if (value.IsBool()) {
    array->PushBoolean(value.Bool());
  } else if (value.IsUndefined()) {
    // Todo(songshourui): handle undefined value later
  } else {
    LOGE("PropBundleAndroid::AssembleArray unsupported type " << value.Type());
    DCHECK(false);
  }
}

void PropBundleAndroid::AssembleMapBuffer(
    base::android::MapBufferBuilder& builder, uint16_t key,
    const pub::Value& value) {
  if (value.IsNil()) {
    builder.putNull(key);
  } else if (value.IsString()) {
    builder.putString(key, value.str().c_str());
  } else if (value.IsInt32()) {
    builder.putInt(key, value.Int32());
  } else if (value.IsInt64()) {
    builder.putLong(key, value.Int64());
  } else if (value.IsUInt32()) {
    builder.putLong(key, value.UInt32());
  } else if (value.IsUInt64()) {
    builder.putLong(key, value.UInt64());
  } else if (value.IsNumber()) {
    builder.putDouble(key, value.Number());
  } else if (value.IsBool()) {
    builder.putBool(key, value.Bool());
  } else if (value.IsArray()) {
    base::android::MapBufferBuilder buffer_builder{};
    value.ForeachArray([&buffer_builder](int64_t index, const pub::Value& v) {
      AssembleMapBuffer(buffer_builder, index, v);
    });
    builder.putMapBuffer(key, buffer_builder.build());
  } else if (value.IsMap()) {
    // not supported in mapbuffer.
  } else if (value.IsUndefined()) {
    // TODO(@nihao.royal): to support undefined.
  } else {
    LOGE("PropBundleAndroid::AssembleMapBuffer unsupported type "
         << value.Type());
    DCHECK(false);
  }
}

// To optimize the copy performance of the prop bundle on Android, its
// ShallowCopy function uses the copy-on-write strategy. The ShallowCopy
// function does not actually copy the JNI map inside, but marks it as const.
// When a real write operation is performed on the prop bundle on Android, the
// JNI map inside will be modified.
std::unique_ptr<PropBundle> PropBundleAndroid::ShallowCopy() {
  auto prop_bundle =
      std::make_unique<PropBundleAndroid>(jni_map_, jni_event_handler_map_);
  MarkConst();
  prop_bundle->MarkConst();
  return prop_bundle;
}

// This function is used to check whether it is necessary to copy the contents
// of the prop bundle before writing to it. If the prop bundle is const, then a
// copy of the JNI map in the prop bundle will be performed.
void PropBundleAndroid::CopyIfConst() {
  if (is_const_ && jni_map_) {
    jni_map_ = jni_map_->ShallowCopy();
  }
  if (is_const_ && jni_event_handler_map_) {
    jni_event_handler_map_ = jni_event_handler_map_->ShallowCopy();
  }
  is_const_ = false;
}

std::unique_ptr<PropBundle> PropBundleCreatorAndroid::CreatePropBundle() {
  return std::unique_ptr<PropBundle>(new PropBundleAndroid());
}

std::unique_ptr<PropBundle> PropBundleCreatorAndroid::CreatePropBundle(
    bool use_map_buffer) {
  return std::unique_ptr<PropBundle>(new PropBundleAndroid(use_map_buffer));
}

base::android::ScopedLocalJavaRef<jobject>
PropBundleAndroid::GetStyleMapBuffer() {
  if (use_map_buffer_) {
    if (style_map_buffer_ == nullptr) {
      style_map_buffer_ = std::make_unique<base::android::MapBuffer>(
          style_buffer_builder_.build());
    }

    return base::android::JReadableMapBuffer::CreateReadableMapBuffer(
        *style_map_buffer_);
  } else {
    // return null.
    JNIEnv* env = base::android::AttachCurrentThread();
    return base::android::ScopedLocalJavaRef<jobject>(env, nullptr);
  }
}

}  // namespace tasm

}  // namespace lynx
