// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/value_wrapper/darwin/value_impl_darwin.h"

namespace lynx {
namespace pub {

int64_t ValueImplDarwin::Type() const { return 0; }

bool ValueImplDarwin::IsUndefined() const { return backend_value_ == nil; }

bool ValueImplDarwin::IsBool() const { return [backend_value_ isKindOfClass:[@YES class]]; }

// Unified as the number type
bool ValueImplDarwin::IsInt32() const {
  return [backend_value_ isKindOfClass:[NSNumber class]] &&
         strcmp([backend_value_ objCType], @encode(int)) == 0;
}
bool ValueImplDarwin::IsInt64() const {
  return [backend_value_ isKindOfClass:[NSNumber class]] &&
         strcmp([backend_value_ objCType], @encode(long long)) == 0;
}
bool ValueImplDarwin::IsUInt32() const {
  // It's always false because NSNumber internally use long long to store unsigned int
  return [backend_value_ isKindOfClass:[NSNumber class]] &&
         strcmp([backend_value_ objCType], @encode(unsigned int)) == 0;
}
bool ValueImplDarwin::IsUInt64() const {
  return [backend_value_ isKindOfClass:[NSNumber class]] &&
         strcmp([backend_value_ objCType], @encode(unsigned long long)) == 0;
}

bool ValueImplDarwin::IsDouble() const {
  return strcmp([backend_value_ objCType], @encode(double)) == 0;
}

bool ValueImplDarwin::IsNumber() const { return [backend_value_ isKindOfClass:[NSNumber class]]; }

bool ValueImplDarwin::IsNil() const { return [backend_value_ isKindOfClass:[NSNull class]]; }

bool ValueImplDarwin::IsString() const { return [backend_value_ isKindOfClass:[NSString class]]; }

bool ValueImplDarwin::IsArray() const { return [backend_value_ isKindOfClass:[NSArray class]]; }

bool ValueImplDarwin::IsArrayBuffer() const {
  return [backend_value_ isKindOfClass:[NSData class]];
}

bool ValueImplDarwin::IsMap() const { return [backend_value_ isKindOfClass:[NSDictionary class]]; }

bool ValueImplDarwin::IsFunction() const { return false; }

bool ValueImplDarwin::Bool() const { return [backend_value_ boolValue]; }

double ValueImplDarwin::Double() const { return [backend_value_ doubleValue]; }

int32_t ValueImplDarwin::Int32() const { return [backend_value_ intValue]; }

uint32_t ValueImplDarwin::UInt32() const { return [backend_value_ unsignedIntValue]; }

int64_t ValueImplDarwin::Int64() const { return [backend_value_ longLongValue]; }

uint64_t ValueImplDarwin::UInt64() const { return [backend_value_ unsignedLongLongValue]; }

double ValueImplDarwin::Number() const { return [backend_value_ doubleValue]; }

uint8_t* ValueImplDarwin::ArrayBuffer() const {
  NSData* data = (NSData*)backend_value_;
  const void* buffer = [data bytes];
  return (uint8_t*)buffer;
}

const std::string& ValueImplDarwin::str() const {
  if (IsString() && cached_str_.empty()) {
    const_cast<ValueImplDarwin*>(this)->cached_str_ = [backend_value_ UTF8String];
  }
  return cached_str_;
}

int ValueImplDarwin::Length() const {
  if (IsMap() || IsArray()) {
    return (int)[backend_value_ count];
  } else if (IsArrayBuffer()) {
    return (int)[backend_value_ length];
  } else {
    return 0;
  }
}

bool ValueImplDarwin::IsEqual(const Value& value) const { return false; }

void ValueImplDarwin::ForeachArray(pub::ForeachArrayFunc func) const {
  if (!IsArray()) {
    return;
  }
  @autoreleasepool {
    NSArray* array = (NSArray*)backend_value_;
    for (NSUInteger i = 0; i < [backend_value_ count]; i++) {
      id value = array[i];
      ValueImplDarwin result(value);
      func(static_cast<int64_t>(i), result);
    }
  }
}

void ValueImplDarwin::ForeachMap(pub::ForeachMapFunc func) const {
  if (!IsMap()) {
    return;
  }

  @autoreleasepool {
    NSDictionary* dict = (NSDictionary*)backend_value_;
    for (id key in dict) {
      id value = dict[key];
      func(ValueImplDarwin(key), ValueImplDarwin(value));
    }
  }
}

std::unique_ptr<Value> ValueImplDarwin::GetValueAtIndex(uint32_t idx) const {
  if (!IsArray() || idx >= static_cast<uint32_t>(Length())) {
    return std::make_unique<ValueImplDarwin>(nil);
  }
  NSArray* array = (NSArray*)backend_value_;
  return std::make_unique<ValueImplDarwin>(array[idx]);
}

bool ValueImplDarwin::Erase(uint32_t idx) const {
  if (![backend_value_ isKindOfClass:[NSMutableArray class]]) {
    return false;
  }
  if ([backend_value_ count] <= idx) {
    return false;
  }
  [(NSMutableArray*)backend_value_ removeObjectAtIndex:idx];
  return true;
}

std::unique_ptr<Value> ValueImplDarwin::GetValueForKey(const std::string& key) const {
  if (!IsMap()) {
    return std::make_unique<ValueImplDarwin>(nil);
  }
  NSDictionary* dict = (NSDictionary*)backend_value_;
  NSString* str = [NSString stringWithUTF8String:key.c_str()];
  return std::make_unique<ValueImplDarwin>(dict[str]);
}

bool ValueImplDarwin::Erase(const std::string& key) const {
  if (![backend_value_ isKindOfClass:[NSMutableDictionary class]]) {
    return false;
  }
  [(NSMutableDictionary*)backend_value_
      removeObjectForKey:[NSString stringWithUTF8String:key.c_str()]];
  return true;
}

bool ValueImplDarwin::Contains(const std::string& key) const {
  if (!IsMap()) {
    return false;
  }
  NSDictionary* dict = (NSDictionary*)backend_value_;
  NSString* str = [NSString stringWithUTF8String:key.c_str()];
  return [dict objectForKey:str] != nil;
}

bool ValueImplDarwin::PushValueToArray(const Value& value) {
  if (![backend_value_ isKindOfClass:[NSMutableArray class]]) {
    return false;
  }
  id v = reinterpret_cast<const ValueImplDarwin*>(&value)->backend_value_;
  if (v == nil) {
    return false;
  }
  [(NSMutableArray*)backend_value_ addObject:v];
  return true;
}

bool ValueImplDarwin::PushValueToArray(std::unique_ptr<Value> value) {
  if (![backend_value_ isKindOfClass:[NSMutableArray class]]) {
    return false;
  }
  id v = reinterpret_cast<const ValueImplDarwin*>(value.get())->backend_value_;
  if (v == nil) {
    return false;
  }
  [(NSMutableArray*)backend_value_ addObject:v];
  return true;
}

bool ValueImplDarwin::PushNullToArray() {
  if (![backend_value_ isKindOfClass:[NSMutableArray class]]) {
    return false;
  }
  [(NSMutableArray*)backend_value_ addObject:[NSNull new]];
  return true;
}

bool ValueImplDarwin::PushArrayBufferToArray(std::unique_ptr<uint8_t[]> value, size_t length) {
  if (![backend_value_ isKindOfClass:[NSMutableArray class]]) {
    return false;
  }
  [(NSMutableArray*)backend_value_ addObject:[NSData dataWithBytes:value.get() length:length]];
  return true;
}

bool ValueImplDarwin::PushStringToArray(const std::string& value) {
  if (![backend_value_ isKindOfClass:[NSMutableArray class]]) {
    return false;
  }
  [(NSMutableArray*)backend_value_ addObject:[NSString stringWithUTF8String:value.c_str()]];
  return true;
}

bool ValueImplDarwin::PushBigIntToArray(const std::string& value) {
  if (![backend_value_ isKindOfClass:[NSMutableArray class]]) {
    return false;
  }
  [(NSMutableArray*)backend_value_ addObject:[NSString stringWithUTF8String:value.c_str()]];
  return true;
}

bool ValueImplDarwin::PushBoolToArray(bool value) {
  if (![backend_value_ isKindOfClass:[NSMutableArray class]]) {
    return false;
  }
  [(NSMutableArray*)backend_value_ addObject:@(value)];
  return true;
}

bool ValueImplDarwin::PushDoubleToArray(double value) {
  if (![backend_value_ isKindOfClass:[NSMutableArray class]]) {
    return false;
  }
  [(NSMutableArray*)backend_value_ addObject:[NSNumber numberWithDouble:value]];
  return true;
}

bool ValueImplDarwin::PushInt32ToArray(int32_t value) {
  if (![backend_value_ isKindOfClass:[NSMutableArray class]]) {
    return false;
  }
  [(NSMutableArray*)backend_value_ addObject:[NSNumber numberWithInt:value]];
  return true;
}

bool ValueImplDarwin::PushUInt32ToArray(uint32_t value) {
  if (![backend_value_ isKindOfClass:[NSMutableArray class]]) {
    return false;
  }
  [(NSMutableArray*)backend_value_ addObject:[NSNumber numberWithUnsignedInt:value]];
  return true;
}

bool ValueImplDarwin::PushInt64ToArray(int64_t value) {
  if (![backend_value_ isKindOfClass:[NSMutableArray class]]) {
    return false;
  }
  [(NSMutableArray*)backend_value_ addObject:[NSNumber numberWithLongLong:value]];
  return true;
}

bool ValueImplDarwin::PushUInt64ToArray(uint64_t value) {
  if (![backend_value_ isKindOfClass:[NSMutableArray class]]) {
    return false;
  }
  [(NSMutableArray*)backend_value_ addObject:[NSNumber numberWithUnsignedLongLong:value]];
  return true;
}

bool ValueImplDarwin::PushValueToMap(const std::string& key, const Value& value) {
  if (![backend_value_ isKindOfClass:[NSMutableDictionary class]]) {
    return false;
  }
  id v = reinterpret_cast<const ValueImplDarwin*>(&value)->backend_value_;
  if (v == nil) {
    return false;
  }
  [(NSMutableDictionary*)backend_value_ setObject:v
                                           forKey:[NSString stringWithUTF8String:key.c_str()]];
  return true;
}

bool ValueImplDarwin::PushValueToMap(const std::string& key, std::unique_ptr<Value> value) {
  if (![backend_value_ isKindOfClass:[NSMutableDictionary class]]) {
    return false;
  }
  id v = reinterpret_cast<const ValueImplDarwin*>(value.get())->backend_value_;
  if (v == nil) {
    return false;
  }
  [(NSMutableDictionary*)backend_value_ setObject:v
                                           forKey:[NSString stringWithUTF8String:key.c_str()]];
  return true;
}

bool ValueImplDarwin::PushNullToMap(const std::string& key) {
  if (![backend_value_ isKindOfClass:[NSMutableDictionary class]]) {
    return false;
  }
  [(NSMutableDictionary*)backend_value_ setObject:[NSNull new]
                                           forKey:[NSString stringWithUTF8String:key.c_str()]];
  return true;
}

bool ValueImplDarwin::PushArrayBufferToMap(const std::string& key, std::unique_ptr<uint8_t[]> value,
                                           size_t length) {
  if (![backend_value_ isKindOfClass:[NSMutableDictionary class]]) {
    return false;
  }
  [(NSMutableDictionary*)backend_value_ setObject:[NSData dataWithBytes:value.get() length:length]
                                           forKey:[NSString stringWithUTF8String:key.c_str()]];
  return true;
}

bool ValueImplDarwin::PushStringToMap(const std::string& key, const std::string& value) {
  if (![backend_value_ isKindOfClass:[NSMutableDictionary class]]) {
    return false;
  }
  [(NSMutableDictionary*)backend_value_ setObject:[NSString stringWithUTF8String:value.c_str()]
                                           forKey:[NSString stringWithUTF8String:key.c_str()]];
  return true;
}

bool ValueImplDarwin::PushBigIntToMap(const std::string& key, const std::string& value) {
  if (![backend_value_ isKindOfClass:[NSMutableDictionary class]]) {
    return false;
  }
  [(NSMutableDictionary*)backend_value_ setObject:[NSString stringWithUTF8String:value.c_str()]
                                           forKey:[NSString stringWithUTF8String:key.c_str()]];
  return true;
}

bool ValueImplDarwin::PushBoolToMap(const std::string& key, bool value) {
  if (![backend_value_ isKindOfClass:[NSMutableDictionary class]]) {
    return false;
  }
  [(NSMutableDictionary*)backend_value_ setObject:@(value)
                                           forKey:[NSString stringWithUTF8String:key.c_str()]];
  return true;
}

bool ValueImplDarwin::PushDoubleToMap(const std::string& key, double value) {
  if (![backend_value_ isKindOfClass:[NSMutableDictionary class]]) {
    return false;
  }
  [(NSMutableDictionary*)backend_value_ setObject:@(value)
                                           forKey:[NSString stringWithUTF8String:key.c_str()]];
  return true;
}

bool ValueImplDarwin::PushInt32ToMap(const std::string& key, int32_t value) {
  if (![backend_value_ isKindOfClass:[NSMutableDictionary class]]) {
    return false;
  }
  [(NSMutableDictionary*)backend_value_ setObject:[NSNumber numberWithInt:value]
                                           forKey:[NSString stringWithUTF8String:key.c_str()]];
  return true;
}

bool ValueImplDarwin::PushUInt32ToMap(const std::string& key, uint32_t value) {
  if (![backend_value_ isKindOfClass:[NSMutableDictionary class]]) {
    return false;
  }
  [(NSMutableDictionary*)backend_value_ setObject:[NSNumber numberWithUnsignedInt:value]
                                           forKey:[NSString stringWithUTF8String:key.c_str()]];
  return true;
}

bool ValueImplDarwin::PushInt64ToMap(const std::string& key, int64_t value) {
  if (![backend_value_ isKindOfClass:[NSMutableDictionary class]]) {
    return false;
  }
  [(NSMutableDictionary*)backend_value_ setObject:[NSNumber numberWithLongLong:value]
                                           forKey:[NSString stringWithUTF8String:key.c_str()]];
  return true;
}

bool ValueImplDarwin::PushUInt64ToMap(const std::string& key, uint64_t value) {
  if (![backend_value_ isKindOfClass:[NSMutableDictionary class]]) {
    return false;
  }
  [(NSMutableDictionary*)backend_value_ setObject:[NSNumber numberWithUnsignedLongLong:value]
                                           forKey:[NSString stringWithUTF8String:key.c_str()]];
  return true;
}

// PubValueFactoryDarwin
std::unique_ptr<Value> PubValueFactoryDarwin::CreateArray() {
  return std::make_unique<ValueImplDarwin>([NSMutableArray array]);
}

std::unique_ptr<Value> PubValueFactoryDarwin::CreateMap() {
  return std::make_unique<ValueImplDarwin>([NSMutableDictionary dictionary]);
}

std::unique_ptr<Value> PubValueFactoryDarwin::CreateBool(bool value) {
  return std::make_unique<ValueImplDarwin>(@(value));
}

std::unique_ptr<Value> PubValueFactoryDarwin::CreateNumber(double value) {
  return std::make_unique<ValueImplDarwin>([NSNumber numberWithDouble:value]);
}

std::unique_ptr<Value> PubValueFactoryDarwin::CreateString(const std::string& value) {
  return std::make_unique<ValueImplDarwin>([NSString stringWithUTF8String:value.c_str()]);
}

std::unique_ptr<Value> PubValueFactoryDarwin::CreateArrayBuffer(std::unique_ptr<uint8_t[]> value,
                                                                size_t length) {
  return std::make_unique<ValueImplDarwin>([NSData dataWithBytes:value.get() length:length]);
}

id ValueUtilsDarwin::ConvertPubValueToOCValue(
    const Value& value, std::vector<std::unique_ptr<pub::Value>>* prev_value_vector, int depth) {
  if (value.backend_type() == lynx::pub::ValueBackendType::ValueBackendTypeDarwin) {
    return (reinterpret_cast<const ValueImplDarwin*>(&value))->backend_value();
  }

  id result = nil;
  if (value.IsBool()) {
    result = [NSNumber numberWithBool:value.Bool()];
  } else if (value.IsInt32()) {
    result = [NSNumber numberWithInt:value.Int32()];
  } else if (value.IsUInt32()) {
    result = [NSNumber numberWithUnsignedInt:value.UInt32()];
  } else if (value.IsInt64()) {
    result = [NSNumber numberWithLongLong:value.Int64()];
  } else if (value.IsUInt64()) {
    result = [NSNumber numberWithUnsignedLongLong:value.UInt64()];
  } else if (value.IsDouble()) {
    result = [NSNumber numberWithDouble:value.Double()];
  } else if (value.IsNumber()) {
    result = [NSNumber numberWithDouble:value.Number()];
  } else if (value.IsMap()) {
    ScopedCircleChecker scoped_circle_checker;
    if (!scoped_circle_checker.CheckCircleOrCacheValue(prev_value_vector, value, depth)) {
      result = ConvertPubValueToOCDictionary(value, prev_value_vector, depth + 1);
    }
  } else if (value.IsArray()) {
    ScopedCircleChecker scoped_circle_checker;
    if (!scoped_circle_checker.CheckCircleOrCacheValue(prev_value_vector, value, depth)) {
      result = ConvertPubValueToOCArray(value, prev_value_vector, depth + 1);
    }
  } else if (value.IsArrayBuffer()) {
    result = [NSData dataWithBytes:value.ArrayBuffer() length:value.Length()];
  } else if (value.IsString()) {
    result = [NSString stringWithUTF8String:value.str().c_str()];
  } else {
    // Null and undefined values are treated as nil for oc
    result = nil;
  }

  return result;
}

NSDictionary* ValueUtilsDarwin::ConvertPubValueToOCDictionary(
    const Value& value, std::vector<std::unique_ptr<pub::Value>>* prev_value_vector, int depth) {
  NSMutableDictionary* dict = [NSMutableDictionary dictionary];
  value.ForeachMap([dict](const lynx::pub::Value& key, const lynx::pub::Value& value) {
    id obj = ConvertPubValueToOCValue(value);
    if (obj) {
      [dict setObject:obj forKey:[NSString stringWithUTF8String:key.str().c_str()]];
    }
  });
  return dict;
}

NSArray* ValueUtilsDarwin::ConvertPubValueToOCArray(
    const Value& value, std::vector<std::unique_ptr<pub::Value>>* prev_value_vector, int depth) {
  NSMutableArray* array = [NSMutableArray array];
  value.ForeachArray([array](int64_t index, const lynx::pub::Value& value) {
    [array addObject:ConvertPubValueToOCValue(value) ?: (id)kCFNull];
  });
  return array;
}

}  // namespace pub
}  // namespace lynx
