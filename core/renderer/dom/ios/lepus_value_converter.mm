// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

#include "base/include/value/array.h"
#include "base/include/value/byte_array.h"
#include "base/include/value/table.h"
#include "core/renderer/dom/ios/lepus_value_converter.h"

namespace lynx {
namespace tasm {

static NSArray *convertLepusArrayToNSArray(const lepus_value &value);
static NSDictionary *convertLepusTableToNSDictionary(const lepus_value &value);

// for JSValue
static id convertQjsValueToNSObject(const lepus::Value &value);
static NSArray *convertQjsArrayToNSArray(const lepus::Value &value);
static NSDictionary *convertQjsTableToNSDictionary(const lepus::Value &value);

static NSString *convertCStringToNSString(const char *value) {
  return [NSString stringWithUTF8String:value];
}

static NSData *convertLepusByteArrayToNSData(const lepus_value &value);

static NSNumber *convertLepusBoolToNSNumber(bool value) { return value ? @(1) : @(0); }

static NSNumber *convertLepusInt64ToNSNumber(int64_t value) { return @(value); }
static NSNumber *convertLepusInt32ToNSNumber(int32_t value) { return @(value); }
static NSNumber *convertLepusUInt64ToNSNumber(uint64_t value) { return @(value); }
static NSNumber *convertLepusUInt32ToNSNumber(uint32_t value) { return @(value); }
static NSNumber *convertLepusDoubleToNSNumber(double value) { return @(value); }

id convertLepusValueToNSObject(const lepus_value &value) {
  if (value.IsJSValue()) {
    return convertQjsValueToNSObject(value);
  }

  id nsValue = nil;
  switch (value.Type()) {
    case lepus::ValueType::Value_String:
      nsValue = convertCStringToNSString(value.CString());
      break;
    case lepus::ValueType::Value_Bool:
      nsValue = convertLepusBoolToNSNumber(value.Bool());
      break;
    case lepus::ValueType::Value_Int64:
      nsValue = convertLepusInt64ToNSNumber(value.Int64());
      break;
    case lepus::ValueType::Value_Int32:
      nsValue = convertLepusInt32ToNSNumber(value.Int32());
      break;
    case lepus::ValueType::Value_UInt64:
      nsValue = convertLepusUInt64ToNSNumber(value.UInt64());
      break;
    case lepus::ValueType::Value_UInt32:
      nsValue = convertLepusUInt32ToNSNumber(value.UInt32());
      break;
    case lepus::ValueType::Value_Double:
      nsValue = convertLepusDoubleToNSNumber(value.Number());
      break;
    case lepus::ValueType::Value_Array:
      nsValue = convertLepusArrayToNSArray(value);
      break;
    case lepus::ValueType::Value_Table:
      nsValue = convertLepusTableToNSDictionary(value);
      break;
    case lepus::ValueType::Value_ByteArray:
      nsValue = convertLepusByteArrayToNSData(value);
      break;
    default:
      break;
  }

  return nsValue;
}

static NSArray *convertLepusArrayToNSArray(const lepus_value &value) {
  auto arr = value.Array();
  NSMutableArray *nsArr = [[NSMutableArray alloc] initWithCapacity:arr->size()];
  for (size_t i = 0; i < arr->size(); i++) {
    auto v = arr->get(i);
    id nsValue = convertLepusValueToNSObject(v);
    if (nsValue) {
      [nsArr addObject:nsValue];
    }
  }
  return nsArr;
}

static NSData *convertLepusByteArrayToNSData(const lepus_value &value) {
  auto bytes = value.ByteArray();
  auto data = [NSData dataWithBytes:bytes->GetPtr() length:bytes->GetLength()];
  return data;
}

static NSDictionary *convertLepusTableToNSDictionary(const lepus_value &value) {
  auto table = value.Table();
  NSMutableDictionary *nsDict = [[NSMutableDictionary alloc] initWithCapacity:table->size()];
  table->for_each([&](const auto &k, auto &v) {
    NSString *key = convertCStringToNSString(k.c_str());
    id nsValue = convertLepusValueToNSObject(v);
    if (nsValue) {
      [nsDict setValue:nsValue forKey:key];
    }
  });
  return nsDict;
}

static id convertQjsValueToNSObject(const lepus::Value &value) {
  id nsValue = nil;
  if (value.IsJSValue()) {
    if (value.IsJSArray()) {
      nsValue = convertQjsArrayToNSArray(value);
    } else if (value.IsJSTable()) {
      nsValue = convertQjsTableToNSDictionary(value);
    } else if (value.IsJSFunction()) {
      LOGE("convertQjsValueToNSObject: Unsupported type: JSFunction.");
      nsValue = nil;
    } else {
      nsValue = convertLepusValueToNSObject(value.ToLepusValue());
    }
  }
  return nsValue;
}

static NSArray *convertQjsArrayToNSArray(const lepus::Value &value) {
  NSMutableArray *nsArray = [[NSMutableArray alloc] init];
  if (value.IsJSArray()) {
    value.IteratorJSValue([nsArray](const lepus::Value &_index, const lepus::Value &element) {
      id nsObject = convertLepusValueToNSObject(element);
      if (nsObject) {
        [nsArray addObject:convertLepusValueToNSObject(element)];
      }
    });
  };
  return nsArray;
}

static NSDictionary *convertQjsTableToNSDictionary(const lepus::Value &value) {
  NSMutableDictionary *nsDict = [[NSMutableDictionary alloc] init];
  if (value.IsJSTable()) {
    value.IteratorJSValue([nsDict](const lepus::Value &key, const lepus::Value &value) {
      NSString *nsKey = convertCStringToNSString(key.CString());
      id nsValue = convertLepusValueToNSObject(value);
      if (nsValue) {
        [nsDict setValue:nsValue forKey:nsKey];
      }
    });
  }
  return nsDict;
}

}  // namespace tasm
}  // namespace lynx
