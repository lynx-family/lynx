// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/ui_wrapper/common/ios/prop_bundle_darwin.h"
#include "core/base/js_constants.h"
#include "core/renderer/events/gesture.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/value_wrapper/value_impl_lepus.h"

#import "core/runtime/bindings/jsi/modules/ios/lynx_module_darwin.h"
#include "third_party/modp_b64/modp_b64.h"

// TODO(chenyouhui): Remove GestureDetector from prop_bundle completely
#ifdef OS_IOS
#import "LynxGestureDetectorDarwin.h"
#endif
#import "LynxLog.h"

namespace lynx {
namespace tasm {
namespace {
NSData* convertJSIArrayBufferDataToNSData(uint8_t* data, int length) {
  if (@available(iOS 10.0, *)) {
    return [NSData dataWithBytes:data length:length];  // copy mode
  } else {
    size_t len = modp_b64_decode_len(modp_b64_encode_len(length));
    if (len < 0) {
      len = 0;
    }
    uint8_t* dest_buf = (uint8_t*)malloc(len);
    memcpy(dest_buf, data, len);
    return [NSData dataWithBytesNoCopy:dest_buf length:len];  // no copy mode
  }
}

}  // namespace

PropBundleDarwin::PropBundleDarwin() { propMap = [[NSMutableDictionary alloc] init]; }

void PropBundleDarwin::SetNullProps(const char* key) {
  [propMap setObject:[NSNull alloc] forKey:[[NSString alloc] initWithUTF8String:key]];
}

void PropBundleDarwin::SetProps(const char* key, uint value) {
  [propMap setObject:[NSNumber numberWithUnsignedInt:value]
              forKey:[[NSString alloc] initWithUTF8String:key]];
}

void PropBundleDarwin::SetProps(const char* key, int value) {
  [propMap setObject:[NSNumber numberWithInt:value]
              forKey:[[NSString alloc] initWithUTF8String:key]];
}

void PropBundleDarwin::SetProps(const char* key, const char* value) {
  [propMap setObject:[[NSString alloc] initWithUTF8String:value]
              forKey:[[NSString alloc] initWithUTF8String:key]];
}

void PropBundleDarwin::SetProps(const char* key, bool value) {
  [propMap setObject:[NSNumber numberWithBool:value]
              forKey:[[NSString alloc] initWithUTF8String:key]];
}

void PropBundleDarwin::SetProps(const char* key, double value) {
  [propMap setObject:[NSNumber numberWithDouble:value]
              forKey:[[NSString alloc] initWithUTF8String:key]];
}

void PropBundleDarwin::SetProps(const char* key, const pub::Value& value) {
  AssembleMap(propMap, key, value);
}

void PropBundleDarwin::SetProps(const pub::Value& value) {
  auto prev_value_vector = pub::ScopedCircleChecker::InitVectorIfNecessary(value);
  value.ForeachMap([this, &prev_value_vector](const pub::Value& k, const pub::Value& v) {
    AssembleMap(propMap, k.str().c_str(), v, prev_value_vector.get(), 0, false);
  });
}

bool PropBundleDarwin::Contains(const char* key) const {
  return [propMap objectForKey:[[NSString alloc] initWithUTF8String:key]] != nil;
}

void PropBundleDarwin::SetEventHandler(const pub::Value& event) {
  auto event_array = pub::ValueUtils::ConvertValueToLepusArray(event).Array();
  const auto& name = event_array->get(0).StdString();
  const auto& type = event_array->get(1).StdString();
  auto is_js_event = event_array->get(2).Bool();

  if (!name.empty()) {
    NSString* event;
    if (!type.empty()) {
      event = [NSString stringWithFormat:@"%s(%s)", name.c_str(), type.c_str()];
    } else {
      event = [NSString stringWithUTF8String:name.c_str()];
    }
    if (is_js_event) {
      if (!eventSet) {
        eventSet = [[NSMutableSet alloc] init];
      }
      [eventSet addObject:event];
    } else {
      if (!lepusEventSet) {
        lepusEventSet = [[NSMutableSet alloc] init];
      }
      [lepusEventSet addObject:event];
    }
  }
}

/**
 * Sets a gesture detector in the receiver's set of gesture detectors.
 *
 * @param gestureDetector The gesture detector to add to the set.
 */
void PropBundleDarwin::SetGestureDetector(const GestureDetector& gestureDetector) {
#ifdef OS_IOS
  if (!gestureDetectorSet) {
    // If gestureDetectorSet is null, create a new NSMutableSet to hold the detectors
    gestureDetectorSet = [[NSMutableSet alloc] init];
  }

  // Extract the gesture callback names into a plain C array, which can be used to create an NSArray
  const auto& gestureCallbacks = gestureDetector.gesture_callbacks();
  NSString* callbackNames[gestureCallbacks.size()];
  int i = 0;
  for (const auto& callback : gestureCallbacks) {
    callbackNames[i++] = [NSString stringWithUTF8String:callback.name_.c_str()];
  }
  NSArray<NSString*>* gestureCallbackArray = [NSArray arrayWithObjects:callbackNames
                                                                 count:gestureCallbacks.size()];

  // Use a move constructor to move the vector's contents into an NSMutableArray, which is faster
  // than copying
  NSMutableDictionary<NSString*, NSArray<NSNumber*>*>* relationMap =
      [NSMutableDictionary dictionary];
  for (auto& it : gestureDetector.relation_map()) {
    NSString* key = [NSString stringWithUTF8String:it.first.c_str()];
    std::vector<uint32_t> value = it.second;
    NSMutableArray<NSNumber*>* objcValue = [NSMutableArray arrayWithCapacity:value.size()];
    for (auto& v : value) {
      NSNumber* objcNumber = [NSNumber numberWithUnsignedInt:v];
      [objcValue addObject:objcNumber];
    }
    [relationMap setObject:objcValue forKey:key];
  }

  NSMutableDictionary* configMap = [NSMutableDictionary dictionary];
  AssembleMap(configMap, "config", pub::ValueImplLepus(gestureDetector.gesture_config()));

  // Use the optimized NSArray and NSDictionary objects to create the detector object
  LynxGestureDetectorDarwin* detectorDarwin = [[LynxGestureDetectorDarwin alloc]
         initWithGestureID:gestureDetector.gesture_id()
               gestureType:static_cast<LynxGestureTypeDarwin>(gestureDetector.gesture_type())
      gestureCallbackNames:gestureCallbackArray
               relationMap:relationMap
                 configMap:[configMap objectForKey:@"config"]];

  // Add the detector to the set
  [gestureDetectorSet addObject:detectorDarwin];
#endif
}

void PropBundleDarwin::ResetEventHandler() {
  [eventSet removeAllObjects];
  [lepusEventSet removeAllObjects];
}

void PropBundleDarwin::AssembleMap(NSMutableDictionary* map, const char* key,
                                   const pub::Value& value,
                                   std::vector<std::unique_ptr<pub::Value>>* prev_value_vector,
                                   int depth, bool need_handle_null_value) {
  NSString* oc_key = [[NSString alloc] initWithUTF8String:key];
  if (value.IsNil()) {
    // When PropBundle is passed as an argument to InvokeUIMethod, the js null value will be ignored
    // during data conversion
    if (!need_handle_null_value) {
      return;
    }
    [map setObject:[NSNull alloc] forKey:oc_key];
  } else if (value.IsString()) {
    NSString* oc_str = [[NSString alloc] initWithUTF8String:value.str().c_str()];
    if (oc_str) {
      [map setObject:oc_str forKey:oc_key];
    } else {
      _LogE(@"Value is not an utf8 string when set props with key %@.", oc_key);
      [map setObject:[NSNull alloc] forKey:oc_key];
    }
  } else if (value.IsNumber()) {
    [map setObject:[NSNumber numberWithDouble:value.Number()] forKey:oc_key];
  } else if (value.IsArrayBuffer()) {
    [map setObject:convertJSIArrayBufferDataToNSData(value.ArrayBuffer(), value.Length())
            forKey:oc_key];
  } else if (value.IsArray()) {
    pub::ScopedCircleChecker scoped_value_helper;
    if (!scoped_value_helper.CheckCircleOrCacheValue(prev_value_vector, value, depth)) {
      NSMutableArray* oc_array = [[NSMutableArray alloc] init];
      value.ForeachArray([this, &oc_array, &prev_value_vector, depth, need_handle_null_value](
                             int64_t index, const pub::Value& v) {
        AssembleArray(oc_array, v, prev_value_vector, depth + 1, need_handle_null_value);
      });
      [map setObject:oc_array forKey:oc_key];
    }
  } else if (value.IsMap()) {
    pub::ScopedCircleChecker scoped_value_helper;
    if (!scoped_value_helper.CheckCircleOrCacheValue(prev_value_vector, value, depth)) {
      NSMutableDictionary* oc_map = [[NSMutableDictionary alloc] init];
      value.ForeachMap([this, &oc_map, &prev_value_vector, depth, need_handle_null_value](
                           const pub::Value& k, const pub::Value& v) {
        AssembleMap(oc_map, k.str().c_str(), v, prev_value_vector, depth + 1,
                    need_handle_null_value);
      });
      [map setObject:oc_map forKey:oc_key];
    }
  } else if (value.IsBool()) {
    [map setObject:[NSNumber numberWithBool:value.Bool()] forKey:oc_key];
  } else if (value.IsUndefined()) {
    // Todo(songshourui): handle undefined value later
  } else if (value.backend_type() == pub::ValueBackendType::ValueBackendTypePiper &&
             value.IsInt64()) {
    // If the backend_type of this PubValue is ValueBackendTypePiper, and it is of int64 (BigInt)
    // type, then on the iOS platform, it is necessary to convert int64 into NSString. This is a
    // legacy logic, and in order not to break the original behavior, this logic is still retained.
    [map setObject:[NSString stringWithFormat:@"%lld", value.Int64()] forKey:oc_key];
  } else {
    LOGE("PropBundleDarwin::AssembleMap unknow type :" << value.Type() << " key :" << key);
    assert(false);
  }
}

void PropBundleDarwin::AssembleArray(NSMutableArray* array, const pub::Value& value,
                                     std::vector<std::unique_ptr<pub::Value>>* prev_value_vector,
                                     int depth, bool need_handle_null_value) {
  if (value.IsNil()) {
    if (!need_handle_null_value) {
      return;
    }
    [array addObject:[NSNull alloc]];
  } else if (value.IsString()) {
    NSString* oc_str = [[NSString alloc] initWithUTF8String:value.str().c_str()];
    if (oc_str) {
      [array addObject:oc_str];
    } else {
      _LogE(@"Value is not an utf8 string when set props with array");
      [array addObject:[NSNull alloc]];
    }
  } else if (value.IsNumber()) {
    [array addObject:[NSNumber numberWithDouble:value.Number()]];
  } else if (value.IsArrayBuffer()) {
    [array addObject:convertJSIArrayBufferDataToNSData(value.ArrayBuffer(), value.Length())];
  } else if (value.IsArray()) {
    pub::ScopedCircleChecker scoped_value_helper;
    if (!scoped_value_helper.CheckCircleOrCacheValue(prev_value_vector, value, depth)) {
      NSMutableArray* oc_array = [[NSMutableArray alloc] init];
      value.ForeachArray([this, &oc_array, &prev_value_vector, depth, need_handle_null_value](
                             int64_t index, const pub::Value& v) {
        AssembleArray(oc_array, v, prev_value_vector, depth, need_handle_null_value);
      });
      [array addObject:oc_array];
    }
  } else if (value.IsMap()) {
    pub::ScopedCircleChecker scoped_value_helper;
    if (!scoped_value_helper.CheckCircleOrCacheValue(prev_value_vector, value, depth)) {
      NSMutableDictionary* oc_map = [[NSMutableDictionary alloc] init];
      value.ForeachMap([this, &oc_map, &prev_value_vector, depth, need_handle_null_value](
                           const pub::Value& k, const pub::Value& v) {
        AssembleMap(oc_map, k.str().c_str(), v, prev_value_vector, depth + 1,
                    need_handle_null_value);
      });
      [array addObject:oc_map];
    }
  } else if (value.IsBool()) {
    [array addObject:[NSNumber numberWithBool:value.Bool()]];
  } else if (value.IsUndefined()) {
    // Todo(songshourui): handle undefined value later
  } else if (value.backend_type() == pub::ValueBackendType::ValueBackendTypePiper &&
             value.IsInt64()) {
    // // If the backend_type of this PubValue is ValueBackendTypePiper, and it is of int64 (BigInt)
    // type, then on the iOS platform, it is necessary to convert int64 into NSString. This is a
    // legacy logic, and in order not to break the original behavior, this logic is still retained.
    [array addObject:[NSString stringWithFormat:@"%lld", value.Int64()]];
  } else {
    assert(false);
  }
}

std::unique_ptr<PropBundle> PropBundleDarwin::ShallowCopy() {
  auto pda = std::unique_ptr<PropBundleDarwin>(new PropBundleDarwin());
  pda->propMap = [propMap mutableCopy];
  if (eventSet) {
    pda->eventSet = [eventSet mutableCopy];
  }
  if (lepusEventSet) {
    pda->lepusEventSet = [lepusEventSet mutableCopy];
  }
  return pda;
}

std::unique_ptr<PropBundle> PropBundleCreatorDarwin::CreatePropBundle() {
  return std::unique_ptr<PropBundleDarwin>(new PropBundleDarwin());
}

}  // namespace tasm
}  // namespace lynx
