// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>

#include <limits.h>

#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/value_wrapper/darwin/value_impl_darwin.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "core/value_wrapper/value_wrapper_utils.h"

@interface ValueImplDarwinUnittest : XCTestCase

@end

@implementation ValueImplDarwinUnittest

- (void)setUp {
}

- (void)tearDown {
}

- (void)testValue {
  lynx::pub::PubValueFactoryDarwin factory;
  auto map = factory.CreateMap();
  auto bool_value = factory.CreateBool(true);
  XCTAssertTrue(bool_value->IsBool());
  map->PushNullToMap("null_key");
  map->PushBoolToMap("bool_key", true);
  map->PushValueToMap("bool_value_key", std::move(bool_value));
  map->PushInt32ToMap("int32_key", 11);
  map->PushInt64ToMap("int64_key", INT64_MAX - 10);
  map->PushDoubleToMap("double_key", 3.1415);
  map->PushStringToMap("string_key", "string_value");
  auto array = factory.CreateArray();
  array->PushNullToArray();
  array->PushBoolToArray(false);
  array->PushUInt32ToArray(123);
  array->PushUInt64ToArray(UINT64_MAX - 11);
  array->PushDoubleToArray(3.12345);
  array->PushStringToArray("string_value");

  lynx::pub::ValueImplDarwin undefined(nil);
  NSString *s = @"buffer_string";
  lynx::pub::ValueImplDarwin array_buffer([NSData dataWithBytes:[s UTF8String] length:s.length]);
  auto *buffer = array_buffer.ArrayBuffer();
  NSString *rs = [NSString stringWithUTF8String:(const char *)buffer];

  XCTAssertTrue(map->IsMap());
  XCTAssertTrue(map->Contains("null_key"));
  XCTAssertTrue(array->IsArray());
  XCTAssertEqual(array->IsString(), NO);
  XCTAssertTrue(undefined.IsUndefined());
  XCTAssertEqual(map->Length(), 7);
  XCTAssertEqual(array->Length(), 6);
  XCTAssertTrue(array_buffer.IsArrayBuffer());
  XCTAssertEqual(array_buffer.Length(), 13);
  XCTAssertTrue([s isEqualToString:rs]);

  auto nil_key_value = map->GetValueForKey("null_key");
  auto bool_key_value = map->GetValueForKey("bool_key");
  auto int32_key_value = map->GetValueForKey("int32_key");
  auto int64_key_value = map->GetValueForKey("int64_key");
  auto double_key_value = map->GetValueForKey("double_key");
  auto string_key_value = map->GetValueForKey("string_key");
  XCTAssertTrue(nil_key_value->IsNil());
  XCTAssertTrue(bool_key_value->IsBool());
  XCTAssertTrue(bool_key_value->Bool());
  XCTAssertTrue(int32_key_value->IsNumber());
  XCTAssertEqual(int32_key_value->Int32(), 11);
  XCTAssertTrue(int64_key_value->IsNumber());
  XCTAssertEqual(int64_key_value->Int64(), INT64_MAX - 10);
  XCTAssertTrue(double_key_value->IsDouble());
  XCTAssertEqual(double_key_value->Double(), 3.1415);
  XCTAssertTrue(string_key_value->IsString());
  XCTAssertEqual(string_key_value->str(), "string_value");

  auto double_array_value = array->GetValueAtIndex(4);
  auto uint32_array_value = array->GetValueAtIndex(2);
  auto uint64_array_value = array->GetValueAtIndex(3);
  XCTAssertEqual(double_array_value->IsBool(), NO);
  XCTAssertTrue(double_array_value->IsDouble());
  XCTAssertEqual(double_array_value->Double(), 3.12345);
  XCTAssertTrue(uint32_array_value->IsNumber());
  XCTAssertEqual(uint32_array_value->UInt32(), (uint32_t)123);
  XCTAssertTrue(uint64_array_value->IsNumber());
  XCTAssertEqual(uint64_array_value->UInt64(), UINT64_MAX - 11);

  auto string_double_value = array->GetValueAtIndex(5);
  XCTAssertTrue(string_double_value->IsString());
  XCTAssertEqual(string_double_value->str(), "string_value");

  map->ForeachMap([](const lynx::pub::Value &key, const lynx::pub::Value &value) {
    if (key.str() == "null_key") {
      XCTAssertTrue(value.IsNil());
    }
    if (key.str() == "string_key") {
      XCTAssertTrue(value.IsString());
      XCTAssertEqual(value.str(), "string_value");
    }
  });

  array->ForeachArray([](int64_t index, const lynx::pub::Value &value) {
    switch (index) {
      case 0:
        XCTAssertTrue(value.IsNil());
        break;
      case 1:
        XCTAssertTrue(value.IsBool());
        XCTAssertEqual(value.Bool(), NO);
        break;
      case 2:
        XCTAssertTrue(value.IsNumber());
        XCTAssertEqual(value.Number(), 123);
        break;
      case 3:
        XCTAssertTrue(value.IsNumber());
        XCTAssertEqual(value.UInt64(), UINT64_MAX - 11);
        break;
      case 4:
        XCTAssertTrue(value.IsDouble());
        XCTAssertEqual(value.Double(), 3.12345);
        break;
      case 5:
        XCTAssertTrue(value.IsString());
        XCTAssertEqual(value.str(), "string_value");
        break;

      default:
        break;
    }
  });

  XCTAssertTrue(map->GetValueForKey("none")->IsUndefined());

  array->Erase(0);
  XCTAssertEqual(array->Length(), 5);
}

- (void)testValueConvert {
  lynx::pub::PubValueFactoryDarwin factory;
  auto map = factory.CreateMap();
  auto bool_value = factory.CreateBool(true);
  map->PushNullToMap("null_key");
  map->PushBoolToMap("bool_key", true);
  map->PushValueToMap("bool_value_key", std::move(bool_value));
  map->PushInt32ToMap("int32_key", 11);
  map->PushInt64ToMap("int64_key", INT64_MAX - 10);
  map->PushDoubleToMap("double_key", 3.1415);
  map->PushStringToMap("string_key", "string_value");

  auto array = factory.CreateArray();
  array->PushNullToArray();
  array->PushBoolToArray(false);
  array->PushUInt32ToArray(123);
  array->PushUInt64ToArray(UINT64_MAX - 11);
  array->PushDoubleToArray(3.12345);
  array->PushStringToArray("string_value");

  auto lepus_map = lynx::pub::ValueUtils::ConvertValueToLepusValue(*map.get());
  auto lepus_array = lynx::pub::ValueUtils::ConvertValueToLepusValue(*array.get());

  XCTAssertTrue(lepus_map.IsTable());
  XCTAssertTrue(lepus_map.Contains(lynx::base::String("null_key")));
  XCTAssertTrue(lepus_map.Contains(lynx::base::String("bool_key")));
  XCTAssertTrue(lepus_map.Contains(lynx::base::String("bool_value_key")));
  XCTAssertTrue(lepus_map.Contains(lynx::base::String("int32_key")));
  XCTAssertTrue(lepus_map.Contains(lynx::base::String("int64_key")));
  XCTAssertTrue(lepus_map.Contains(lynx::base::String("double_key")));
  XCTAssertTrue(lepus_map.Contains(lynx::base::String("string_key")));
  XCTAssertTrue(lepus_map.Table()->GetValue(lynx::base::String("null_key")).IsNil());
  XCTAssertTrue(lepus_map.Table()->GetValue(lynx::base::String("none")).IsNil());
  XCTAssertEqual(lepus_map.Table()->GetValue(lynx::base::String("string_key")).String().str(),
                 "string_value");
  XCTAssertTrue(lepus_array.IsArray());
  XCTAssertEqual(lepus_array.GetLength(), 6);
  XCTAssertEqual(lepus_array.Array()->get(3).Number(), UINT64_MAX - 11);
  XCTAssertTrue(lepus_array.Array()->get(2).IsNumber());
  XCTAssertFalse(lepus_array.Array()->get(2).IsInt32());
  XCTAssertFalse(lepus_array.Array()->get(3).IsInt64());

  lynx::pub::PubValueFactoryDefault default_factory;
  auto default_array = default_factory.CreateArray();
  default_array->PushNullToArray();
  default_array->PushBoolToArray(false);
  default_array->PushUInt32ToArray(123);
  default_array->PushUInt64ToArray(UINT64_MAX - 11);
  default_array->PushDoubleToArray(3.12345);
  default_array->PushStringToArray("string_value");

  auto prev_value_vector =
      lynx::pub::ScopedCircleChecker::InitVectorIfNecessary(*default_array.get());
  NSArray *oc_array = lynx::pub::ValueUtilsDarwin::ConvertPubValueToOCValue(
      *default_array.get(), prev_value_vector.get(), 0);
  XCTAssertTrue([oc_array isKindOfClass:[NSArray class]]);
  XCTAssertEqual((int)[oc_array count], 6);
  XCTAssertEqual(oc_array[0], [NSNull new]);
  XCTAssertEqual(oc_array[1], @(NO));
  XCTAssertEqual([oc_array[2] intValue], 123);
  XCTAssertEqual([oc_array[3] unsignedLongLongValue], UINT64_MAX - 11);
  XCTAssertEqual([oc_array[4] doubleValue], 3.12345);
  XCTAssertTrue([oc_array[5] isEqualToString:@"string_value"]);

  auto default_map = default_factory.CreateMap();
  default_map->PushNullToMap("null_key");
  default_map->PushBoolToMap("bool_key", true);
  default_map->PushInt32ToMap("int32_key", 11);
  default_map->PushInt64ToMap("int64_key", INT64_MAX - 10);
  default_map->PushDoubleToMap("double_key", 3.1415);
  default_map->PushStringToMap("string_key", "string_value");

  NSDictionary *oc_dict = lynx::pub::ValueUtilsDarwin::ConvertPubValueToOCValue(*default_map.get());
  XCTAssertEqual(oc_dict[@"null_key"], nil);
  XCTAssertEqual(oc_dict[@"bool_key"], @(YES));
  XCTAssertEqual([oc_dict[@"int32_key"] intValue], 11);
  XCTAssertEqual([oc_dict[@"int64_key"] longLongValue], INT64_MAX - 10);
  XCTAssertEqual([oc_dict[@"double_key"] doubleValue], 3.1415);
  XCTAssertTrue([oc_dict[@"string_key"] isEqualToString:@"string_value"]);

  auto nil_value = lynx::lepus::Value();
  nil_value.SetNil();
  auto undefined_value = lynx::lepus::Value();
  undefined_value.SetUndefined();
  XCTAssertTrue(lynx::pub::ValueUtilsDarwin::ConvertPubValueToOCValue(
                    lynx::pub::ValueImplLepus(nil_value)) == nil);
  XCTAssertTrue(lynx::pub::ValueUtilsDarwin::ConvertPubValueToOCValue(
                    lynx::pub::ValueImplLepus(undefined_value)) == nil);
}

- (void)testPerformanceExample {
  // This is an example of a performance test case.
  [self measureBlock:^{
      // Put the code you want to measure the time of here.
  }];
}

@end
