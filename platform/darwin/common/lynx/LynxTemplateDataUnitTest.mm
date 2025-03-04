// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import "LynxTemplateData+Converter.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/lepus_value.h"

@interface LynxTemplateDataTest : XCTestCase

@end

@implementation LynxTemplateDataTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)empty {
  LynxTemplateData* templateData = [[LynxTemplateData alloc] initWithJson:@""];
  auto* value = LynxGetLepusValueFromTemplateData(templateData);
  XCTAssertTrue(value->IsEmpty());
}

- (void)markReadOnly {
  LynxTemplateData* templateData = [[LynxTemplateData alloc] initWithJson:@""];
  XCTAssertFalse(templateData.isReadOnly);
  [templateData markReadOnly];
  XCTAssertTrue(templateData.isReadOnly);
}

- (void)fromMap {
  NSDictionary* dict = @{@"a" : @"1"};
  LynxTemplateData* templateData = [[LynxTemplateData alloc] initWithDictionary:dict];
  auto* value = LynxGetLepusValueFromTemplateData(templateData);

  XCTAssertFalse(value->IsEmpty());
  XCTAssertTrue(value->IsTable());
  XCTAssertTrue(value->Contains("a"));
  XCTAssertTrue([dict isEqual:[templateData dictionary]]);
}

- (void)testLong {
  NSDictionary* dict = @{@"long" : @123456789123456L};
  LynxTemplateData* templateData = [[LynxTemplateData alloc] initWithDictionary:dict];
  auto* value = LynxGetLepusValueFromTemplateData(templateData);

  XCTAssertFalse(value->IsEmpty());
  XCTAssertTrue(value->IsTable());
  XCTAssertTrue(value->Contains("long"));
  XCTAssertTrue([dict isEqual:[templateData dictionary]]);
  XCTAssertEqual(123456789123456L, value->GetProperty("long").Number());
}

- (void)testFromString {
  LynxTemplateData* templateData = [[LynxTemplateData alloc]
      initWithJson:@"{\n"
                    "    \"language\": \"English\",\n"
                    "    \"please-input-password\": \"Please input your:\",\n"
                    "    \"input-notify\": \"Please input your {1} and {2} in the {0}\",\n"
                    "    \"input-box\": \"input box\",\n"
                    "    \"input-name\": \"name\",\n"
                    "    \"input-age\": \"age\",\n"
                    "    \"submit\": \"Submit\",\n"
                    "    \"cancel\": \"Cancel\",\n"
                    "    \"mode-lang-day\": \"Day Mode\",\n"
                    "    \"mode-lang-night\": \"Night Mode\",\n"
                    "    \"icon-url\": \"url\"\n"
                    "}\n"];
  auto* value = LynxGetLepusValueFromTemplateData(templateData);

  XCTAssertFalse(value->IsEmpty());
  XCTAssertTrue(value->IsTable());
  XCTAssertEqual(value->GetProperty("please-input-password").StdString(), "Please input your:");
  XCTAssertTrue(*value == [templateData getDataForJSThread]);
}

- (void)testMergeTemplateData {
  NSDictionary* dict = @{@"long" : @123456789123456L};
  LynxTemplateData* templateData = [[LynxTemplateData alloc] initWithDictionary:dict];

  LynxTemplateData* diff = [[LynxTemplateData alloc]
      initWithJson:@"{\n"
                    "    \"language\": \"English\",\n"
                    "    \"please-input-password\": \"Please input your:\",\n"
                    "    \"input-notify\": \"Please input your {1} and {2} in the {0}\",\n"
                    "    \"input-box\": \"input box\",\n"
                    "    \"input-name\": \"name\",\n"
                    "    \"input-age\": \"age\",\n"
                    "    \"submit\": \"Submit\",\n"
                    "    \"cancel\": \"Cancel\",\n"
                    "    \"mode-lang-day\": \"Day Mode\",\n"
                    "    \"mode-lang-night\": \"Night Mode\",\n"
                    "    \"icon-url\": \"url\"\n"
                    "}\n"];

  [templateData updateWithTemplateData:diff];
  auto* value = LynxGetLepusValueFromTemplateData(templateData);

  XCTAssertFalse(value->IsEmpty());
  XCTAssertTrue(value->IsTable());
  XCTAssertTrue(*value == [templateData getDataForJSThread]);
  XCTAssertEqual(value->GetProperty("please-input-password").StdString(), "Please input your:");
  XCTAssertEqual(123456789123456L, value->GetProperty("long").Number());
}

- (void)testToMapIntDouble {
  LynxTemplateData* data = [[LynxTemplateData alloc] initWithDictionary:@{}];
  [data updateDouble:1 forKey:@"key1"];
  [data updateDouble:2 forKey:@"key2"];
  [data updateDouble:3.0 forKey:@"key3"];
  [data updateDouble:4.0 forKey:@"key4"];

  auto* value = LynxGetLepusValueFromTemplateData(data);
  XCTAssertTrue(value->GetProperty("key1").Number() == 1);
  XCTAssertTrue(value->GetProperty("key2").Number() == 2);
  XCTAssertTrue(value->GetProperty("key3").Number() == 3);
  XCTAssertTrue(value->GetProperty("key4").Number() == 4);
}

- (void)testUpdateActions0 {
  LynxTemplateData* data = [[LynxTemplateData alloc] initWithDictionary:@{}];
  [data updateDouble:1 forKey:@"key1"];
  [data updateDouble:2 forKey:@"key2"];
  [data updateDouble:3.0 forKey:@"key3"];
  [data updateDouble:4.0 forKey:@"key4"];

  auto* value = LynxGetLepusValueFromTemplateData(data);
  XCTAssertEqual(*value, [data getDataForJSThread]);
}

- (void)testUpdateActions1 {
  LynxTemplateData* data0 = [[LynxTemplateData alloc]
      initWithJson:@"{\n"
                    "    \"language\": \"English\",\n"
                    "    \"please-input-password\": \"Please input your:\",\n"
                    "    \"input-notify\": \"Please input your {1} and {2} in the {0}\",\n"
                    "    \"input-box\": \"input box\",\n"
                    "    \"input-name\": \"name\",\n"
                    "    \"input-age\": \"age\",\n"
                    "    \"submit\": \"Submit\",\n"
                    "    \"cancel\": \"Cancel\",\n"
                    "    \"mode-lang-day\": \"Day Mode\",\n"
                    "    \"mode-lang-night\": \"Night Mode\",\n"
                    "    \"icon-url\": \"url\"\n"
                    "}\n"];
  [data0 updateDouble:1 forKey:@"key1"];
  [data0 updateDouble:2 forKey:@"key2"];
  [data0 updateDouble:3.0 forKey:@"key3"];
  [data0 updateDouble:4.0 forKey:@"key4"];

  LynxTemplateData* data1 = [[LynxTemplateData alloc] initWithDictionary:@{}];
  [data1 updateDouble:1 forKey:@"key1"];
  [data1 updateDouble:2 forKey:@"key2"];
  [data1 updateDouble:3.0 forKey:@"key3"];
  [data1 updateDouble:4.0 forKey:@"key4"];

  [data0 updateWithTemplateData:data1];

  auto* nativeData0 = LynxGetLepusValueFromTemplateData(data0);
  auto jsData0 = [data0 getDataForJSThread];
  XCTAssertTrue(*nativeData0 == jsData0);

  LynxTemplateData* data2 = [data0 deepClone];

  auto* nativeData2 = LynxGetLepusValueFromTemplateData(data0);
  auto jsData2 = [data2 getDataForJSThread];

  XCTAssertTrue(*nativeData0 == *nativeData2);
  XCTAssertTrue(jsData0 == jsData2);
  XCTAssertTrue(*nativeData2 == jsData2);
}

- (void)testUpdateNil {
  LynxTemplateData* data0 = [[LynxTemplateData alloc]
      initWithJson:@"{\n"
                    "    \"language\": \"English\",\n"
                    "    \"please-input-password\": \"Please input your:\",\n"
                    "    \"input-notify\": \"Please input your {1} and {2} in the {0}\",\n"
                    "    \"input-box\": \"input box\",\n"
                    "    \"input-name\": \"name\",\n"
                    "    \"input-age\": \"age\",\n"
                    "    \"submit\": \"Submit\",\n"
                    "    \"cancel\": \"Cancel\",\n"
                    "    \"mode-lang-day\": \"Day Mode\",\n"
                    "    \"mode-lang-night\": \"Night Mode\",\n"
                    "    \"icon-url\": \"url\"\n"
                    "}\n"];
  [data0 updateDouble:1 forKey:@"key1"];
  [data0 updateDouble:2 forKey:@"key2"];
  [data0 updateDouble:3.0 forKey:@"key3"];
  [data0 updateDouble:4.0 forKey:@"key4"];
  [data0 updateDouble:4.0 forKey:nil];
  [data0 updateObject:nil forKey:nil];
  [data0 updateObject:nil forKey:@"key5"];
  [data0 updateWithJson:nil];
  [data0 updateBool:true forKey:nil];
  [data0 updateInteger:1 forKey:nil];
  [data0 updateWithTemplateData:nil];
  [data0 updateWithDictionary:nil];

  LynxTemplateData* data1 = [[LynxTemplateData alloc] initWithDictionary:@{}];
  [data1 updateDouble:1 forKey:@"key1"];
  [data1 updateDouble:2 forKey:@"key2"];
  [data1 updateDouble:3.0 forKey:@"key3"];
  [data1 updateDouble:4.0 forKey:@"key4"];

  [data0 updateWithTemplateData:data1];

  const auto& check_table_equal = [](const lynx::lepus::Value& left,
                                     const lynx::lepus::Value& right) {
    if (!left.IsTable()) {
      return false;
    }
    if (!right.IsTable()) {
      return false;
    }
    bool result = true;
    auto left_table = left.Table();
    auto right_table = right.Table();

    lynx::tasm::ForEachLepusValue(left,
                                  [&result, &right_table](const auto& key, const auto& value) {
                                    auto key_str = key.String();
                                    auto iter = right_table->find(key_str);
                                    if (iter == right_table->end()) {
                                      if (key_str.empty()) {
                                        iter = right_table->find("");
                                        if (iter == right_table->end()) {
                                          result = false;
                                        }
                                      } else {
                                        result = false;
                                      }
                                    }
                                    result = result && iter->second == value;
                                  });

    lynx::tasm::ForEachLepusValue(right,
                                  [&result, &left_table](const auto& key, const auto& value) {
                                    auto key_str = key.String();
                                    auto iter = left_table->find(key_str);
                                    if (iter == left_table->end()) {
                                      if (key_str.empty()) {
                                        iter = left_table->find("");
                                        if (iter == left_table->end()) {
                                          result = false;
                                        }
                                      } else {
                                        result = false;
                                      }
                                    }
                                    result = result && iter->second == value;
                                  });

    return result;
  };

  auto* nativeData0 = LynxGetLepusValueFromTemplateData(data0);
  auto jsData0 = [data0 getDataForJSThread];
  std::ostringstream native_data_stream_0;
  nativeData0->PrintValue(native_data_stream_0);
  NSLog(@"nativeData0: %@",
        [[NSString alloc] initWithUTF8String:native_data_stream_0.str().c_str()]);

  std::ostringstream js_data_stream_0;
  jsData0.PrintValue(js_data_stream_0);
  NSLog(@"jsData0: %@", [[NSString alloc] initWithUTF8String:js_data_stream_0.str().c_str()]);

  nativeData0->Print();
  jsData0.Print();
  NSLog(@"-----------");

  LynxTemplateData* data2 = [data0 deepClone];

  auto* nativeData2 = LynxGetLepusValueFromTemplateData(data0);
  auto jsData2 = [data2 getDataForJSThread];
  nativeData2->Print();
  jsData2.Print();
  NSLog(@"-----------");

  NSLog(@"nativeData0: %@",
        [[NSString alloc] initWithUTF8String:native_data_stream_0.str().c_str()]);

  std::ostringstream native_data_stream_2;
  nativeData2->PrintValue(native_data_stream_2);
  NSLog(@"nativeData2: %@",
        [[NSString alloc] initWithUTF8String:native_data_stream_2.str().c_str()]);

  NSLog(@"jsData0: %@", [[NSString alloc] initWithUTF8String:js_data_stream_0.str().c_str()]);
  std::ostringstream js_data_2;
  jsData2.PrintValue(js_data_2);
  NSLog(@"jsData2: %@", [[NSString alloc] initWithUTF8String:js_data_2.str().c_str()]);

  NSLog(@"nativeData2: %@",
        [[NSString alloc] initWithUTF8String:native_data_stream_2.str().c_str()]);
  NSLog(@"jsData2: %@", [[NSString alloc] initWithUTF8String:js_data_2.str().c_str()]);
}

- (void)testUseBoolLiterals {
  NSDictionary* dictionary = @{@"prop_bool" : @YES, @"prop_num" : @1234, @"prop_str" : @"test"};

  // use bool literals
  LynxTemplateData* template_data_use_bool_literals =
      [[LynxTemplateData alloc] initWithDictionary:dictionary useBoolLiterals:true];
  XCTAssertEqualObjects(dictionary, [template_data_use_bool_literals dictionary]);

  // former format, convert @YES to 1
  LynxTemplateData* template_data = [[LynxTemplateData alloc] initWithDictionary:dictionary];
  NSDictionary* dictionary_lose_bool =
      @{@"prop_bool" : @1, @"prop_num" : @1234, @"prop_str" : @"test"};
  XCTAssertEqualObjects(dictionary_lose_bool, [template_data dictionary]);
}

// test getDataForJSThread
- (void)testUseBoolLiteralsWithJS {
  constexpr auto get_js_data = [](LynxTemplateData* template_data) {
    static NSString* const key = @"data";
    [template_data updateObject:@{@"jsBool" : @YES, @"jsNumber" : @2} forKey:key];
    return [template_data getDataForJSThread].GetProperty([key UTF8String]);
  };

  // use bool literals
  {
    LynxTemplateData* template_data = [[LynxTemplateData alloc] initWithDictionary:nil
                                                                   useBoolLiterals:true];
    auto data = get_js_data(template_data);
    XCTAssertTrue(data.GetProperty("jsBool").IsBool());
    XCTAssertTrue(data.GetProperty("jsNumber").IsNumber());
  }

  // former format, convert @YES to 1
  {
    LynxTemplateData* template_data = [[LynxTemplateData alloc] initWithDictionary:nil];
    auto data = get_js_data(template_data);
    XCTAssertTrue(data.GetProperty("jsBool").IsNumber());
  }
}

- (void)testUpdateActionsCrash {
  __block BOOL operationCompleted = NO;

  LynxTemplateData* data = [[LynxTemplateData alloc] initWithDictionary:@{}];

  for (int i = 0; i < 1000; ++i) {
    NSString* key = [NSString stringWithFormat:@"key_%d", i];
    [data updateDouble:i forKey:key];
  }

  dispatch_queue_t globalQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
  dispatch_async(globalQueue, ^{
    auto js_value = [data getDataForJSThread];
    operationCompleted = YES;
  });

  for (int i = 1000; i < 2000000; ++i) {
    NSString* key = [NSString stringWithFormat:@"key_%d", i];
    [data updateDouble:i forKey:key];
  }

  NSDate* loopUntil = [NSDate dateWithTimeIntervalSinceNow:10];
  while (operationCompleted == NO && [loopUntil timeIntervalSinceNow] > 0) {
    [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:loopUntil];
  }

  if (!operationCompleted) {
    XCTFail(@"Wait for timeout!");
  }

  auto* value = LynxGetLepusValueFromTemplateData(data);
  XCTAssertTrue(*value == [data getDataForJSThread]);
}

@end
