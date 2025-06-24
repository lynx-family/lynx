// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxDefines.h>
#import <Lynx/LynxTemplateData.h>
#import <sys/utsname.h>
#import "LynxTemplateData+Converter.h"

#import <Lynx/LynxLog.h>
#include "base/include/value/array.h"
#include "base/include/value/byte_array.h"
#include "core/renderer/data/lynx_view_data_manager.h"
#include "core/renderer/dom/ios/lepus_value_converter.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/json_parser.h"

static BOOL const DEFAULT_USE_BOOL_LITERALS = NO;

using namespace lynx::tasm;
using namespace lynx::lepus;

@implementation LynxTemplateData {
  lynx::lepus::Value value_;
  lynx::lepus::Value value_for_js_;
  NSString* _processerName;
  BOOL _readOnly;
  NSMutableArray* _updateActions;
  BOOL _useBoolLiterals;
}

- (instancetype)init {
  if (self = [super init]) {
    _processerName = nil;
    _readOnly = false;

    _updateActions = [[NSMutableArray alloc] init];
  }
  return self;
}

- (void)addObjectToUpdateActions:(id)obj {
  @synchronized(_updateActions) {
    if (_updateActions == nil) {
      return;
    }

    if (obj == nil) {
      return;
    }

    if ([obj isKindOfClass:[NSArray class]]) {
      [_updateActions addObjectsFromArray:(NSArray*)obj];
      return;
    }

    [_updateActions addObject:obj];
  }
}

- (NSArray*)obtainUpdateActions {
  NSMutableArray* array = [NSMutableArray new];
  @synchronized(_updateActions) {
    [array addObjectsFromArray:_updateActions];
    [_updateActions removeAllObjects];
  }
  return array;
}

- (NSArray*)copyUpdateActions {
  NSMutableArray* array = [NSMutableArray new];
  @synchronized(_updateActions) {
    [array addObjectsFromArray:_updateActions];
  }
  return array;
}

lepus_value LynxConvertToLepusValue(id data, BOOL useBoolLiterals) {
  return RecursiveLynxConvertToLepusValue(data, [[NSMutableSet alloc] init], useBoolLiterals);
}

lepus_value RecursiveLynxConvertToLepusValue(id data, NSMutableSet* allObjects,
                                             BOOL useBoolLiterals) {
  if ([data isKindOfClass:[NSNumber class]]) {
    // if use bool literals, convert @YES/@NO to lepus bool true/false, else to lepus number 1/0.
    if (useBoolLiterals) {
      if ([data isKindOfClass:[@YES class]]) {
        return lepus_value([data boolValue]);
      }
    }
    const char* type = [data objCType];
    if (strcmp(type, @encode(char)) == 0 || strcmp(type, @encode(unsigned char)) == 0) {
      return lepus_value([data charValue]);
    } else if (strcmp(type, @encode(int)) == 0 || strcmp(type, @encode(short)) == 0 ||
               strcmp(type, @encode(unsigned int)) == 0 ||
               strcmp(type, @encode(unsigned short)) == 0) {
      return lepus_value([data intValue]);
    } else if (strcmp(type, @encode(long)) == 0 || strcmp(type, @encode(long long)) == 0 ||
               strcmp(type, @encode(unsigned long)) == 0 ||
               strcmp(type, @encode(unsigned long long)) == 0) {
      return lepus_value([data longLongValue]);
    } else if (strcmp(type, @encode(float)) == 0 || strcmp(type, @encode(double)) == 0) {
      return lepus_value([data doubleValue]);
    } else {
      return lepus_value([data doubleValue]);
    }
  } else if ([data isKindOfClass:[NSString class]]) {
    return lepus_value([data UTF8String]);
  } else if ([data isKindOfClass:[NSArray class]]) {
    if ([allObjects containsObject:data]) {
      _LogE(@"LynxConvertToLepusValue has cycle array!");
      return lepus_value();
    }
    [allObjects addObject:data];
    lynx::fml::RefPtr<CArray> ary = CArray::Create();
    ary->reserve([data count]);
    [data enumerateObjectsUsingBlock:^(id _Nonnull value, NSUInteger idx, BOOL* _Nonnull stop) {
      ary->emplace_back(RecursiveLynxConvertToLepusValue(value, allObjects, useBoolLiterals));
    }];
    [allObjects removeObject:data];
    return lepus_value(std::move(ary));
  } else if ([data isKindOfClass:[NSDictionary class]]) {
    if ([allObjects containsObject:data]) {
      _LogE(@"LynxConvertToLepusValue has cycle dict!");
      return lepus_value();
    }
    [allObjects addObject:data];
    lynx::fml::RefPtr<Dictionary> dict = Dictionary::Create();
    [data enumerateKeysAndObjectsUsingBlock:^(NSString* _Nonnull key, id _Nonnull value,
                                              BOOL* _Nonnull stop) {
      dict->SetValue([key UTF8String],
                     RecursiveLynxConvertToLepusValue(value, allObjects, useBoolLiterals));
    }];
    [allObjects removeObject:data];
    return lepus_value(std::move(dict));
  } else if ([data isKindOfClass:[NSData class]]) {
    size_t length = [data length];
    std::unique_ptr<uint8_t[]> buffer;
    if (length > 0) {
      buffer = std::make_unique<uint8_t[]>(length);
    }
    if (buffer && length > 0) {
      [data getBytes:buffer.get() length:length];
    }
    return lepus_value(ByteArray::Create(std::move(buffer), length));
  } else if ([data isKindOfClass:[LynxTemplateData class]]) {
    return *LynxGetLepusValueFromTemplateData(data);
  }
  return lepus_value();
}

lynx::lepus::Value* LynxGetLepusValueFromTemplateData(LynxTemplateData* data) {
  if (data == nil) return nullptr;
  return &data->value_;
}

- (instancetype)initWithDictionary:(NSDictionary*)dictionary useBoolLiterals:(BOOL)useBoolLiterals {
  self = [self init];
  _useBoolLiterals = useBoolLiterals;
  if (self) {
    value_ = lynx::lepus::Value(lynx::lepus::Dictionary::Create());
    if (dictionary) {
      [self updateWithDictionary:dictionary];
    }
  }
  return self;
}

- (instancetype)initWithDictionary:(NSDictionary*)dictionary {
  return [self initWithDictionary:dictionary useBoolLiterals:DEFAULT_USE_BOOL_LITERALS];
}

- (void)updateWithTemplateData:(LynxTemplateData*)inputData {
  if (inputData == nil) {
    return;
  }

  [self addObjectToUpdateActions:[inputData copyUpdateActions]];
  [self updateWithLepusValue:inputData->value_];
}

- (void)updateWithLepusValue:(const lynx::lepus::Value&)inputValue {
  if (_readOnly) {
    NSLog(@"can not update readOnly TemplateData");
    return;
  }
  if (value_.IsTable() && value_.Table()->IsConst()) {
    value_ = lynx::lepus::Value::Clone(value_);
  }
  if (inputValue.IsTable()) {
    auto dict = inputValue.Table();
    for (auto iter = dict->begin(); iter != dict->end(); iter++) {
      if (iter->second.IsTable()) {
        lynx::lepus::Value oldValue = value_.GetProperty(iter->first);
        if (oldValue.IsTable()) {
          if (oldValue.Table()->IsConst()) {
            oldValue = lynx::lepus::Value::Clone(oldValue);
            value_.SetProperty(iter->first, oldValue);
          }
          lynx::lepus::Dictionary* table = iter->second.Table().get();
          for (auto it = table->begin(); it != table->end(); it++) {
            oldValue.SetProperty(it->first, it->second);
          }
          continue;
        }
      }
      value_.SetProperty(iter->first, iter->second);
    }
  }
}

- (instancetype)initWithJson:(NSString*)json {
  return [self initWithJson:json useBoolLiterals:DEFAULT_USE_BOOL_LITERALS];
}

- (instancetype)initWithJson:(NSString*)json useBoolLiterals:(BOOL)useBoolLiterals {
  NSData* data = [json dataUsingEncoding:NSUTF8StringEncoding];
  NSError* error;
  NSDictionary* dict = [NSJSONSerialization JSONObjectWithData:data
                                                       options:NSJSONReadingMutableContainers
                                                         error:&error];
  return [self initWithDictionary:dict useBoolLiterals:useBoolLiterals];
}

- (BOOL)checkIsLegalData {
  return !(value_.IsNil() || (value_.IsTable() && value_.Table()->size() == 0) ||
           (value_.IsArray() && value_.Array()->size() == 0));
}

- (void)updateWithJson:(NSString*)json {
  if (json == nil) {
    return;
  }
  NSData* data = [json dataUsingEncoding:NSUTF8StringEncoding];
  NSError* error;
  NSDictionary* dict = [NSJSONSerialization JSONObjectWithData:data
                                                       options:NSJSONReadingMutableContainers
                                                         error:&error];
  [self updateWithDictionary:dict];
}

- (void)updateWithDictionary:(NSDictionary*)dict {
  [self addObjectToUpdateActions:dict];

  lepus_value value = LynxConvertToLepusValue(dict, _useBoolLiterals);
  [self updateWithLepusValue:value];
}

- (void)setObject:(id)object withKey:(NSString*)key {
  [self updateObject:object forKey:key];
}

- (void)updateObject:(id)object forKey:(NSString*)key {
  if (_readOnly) {
    NSLog(@"can not update readOnly TemplateData");
    return;
  }

  [self
      addObjectToUpdateActions:@{key != nil            ? key
                                 : @"" : object != nil ? object
                                                       : [NSNull null]}];

  lepus_value value = LynxConvertToLepusValue(object, _useBoolLiterals);
  value_.Table()->SetValue(lynx::base::String([key UTF8String]), value);
}

- (void)updateBool:(BOOL)value forKey:(NSString*)key {
  if (_readOnly) {
    NSLog(@"can not update readOnly TemplateData");
    return;
  }

  [self addObjectToUpdateActions:@{key != nil ? key : @"" : @(value)}];

  value_.Table()->SetValue(lynx::base::String([key UTF8String]), (bool)value);
}

- (void)updateInteger:(NSInteger)value forKey:(NSString*)key {
  if (_readOnly) {
    NSLog(@"can not update readOnly TemplateData");
    return;
  }

  [self addObjectToUpdateActions:@{key != nil ? key : @"" : @(value)}];

  value_.Table()->SetValue(lynx::base::String([key UTF8String]), (int64_t)value);
}

- (void)updateDouble:(CGFloat)value forKey:(NSString*)key {
  if (_readOnly) {
    NSLog(@"can not update readOnly TemplateData");
    return;
  }

  [self addObjectToUpdateActions:@{key != nil ? key : @"" : @(value)}];

  value_.Table()->SetValue(lynx::base::String([key UTF8String]), (double)value);
}

- (LynxTemplateData*)deepClone {
  auto base_value = *LynxGetLepusValueFromTemplateData(self);
  LynxTemplateData* data = [[LynxTemplateData alloc] init];
  data->value_ = lynx::lepus::Value::Clone(base_value);
  data->_processerName = self.processorName;
  data->_readOnly = self.isReadOnly;
  [data addObjectToUpdateActions:[self copyUpdateActions]];
  data->value_for_js_ = lynx::lepus::Value::Clone(value_for_js_);
  return data;
}

- (void)markState:(NSString*)name {
  _processerName = name;
}

- (NSString*)processorName {
  return _processerName;
}

- (lynx::lepus::Value)getDataForJSThread {
  NSArray* array = [self obtainUpdateActions];

  // Init value_for_js_ or update _updateActions to value_for_js_.
  [array enumerateObjectsUsingBlock:^(id _Nonnull obj, NSUInteger idx, BOOL* _Nonnull stop) {
    if ([obj isKindOfClass:[NSDictionary class]]) {
      lynx::lepus::Value dict = LynxConvertToLepusValue(obj, _useBoolLiterals);
      if (!dict.IsTable()) {
        return;
      }
      if (value_for_js_.IsTable()) {
        auto table = dict.Table();
        auto target = value_for_js_.Table();
        for (auto iter = table->begin(); iter != table->end(); iter++) {
          target->SetValue(iter->first, std::move(iter->second));
        }
      } else {
        value_for_js_ = std::move(dict);
      }
    }
  }];

  return value_for_js_;
}

- (void)markReadOnly {
  _readOnly = true;
}

- (BOOL)isReadOnly {
  return _readOnly;
}

- (NSDictionary*)dictionary {
  id dict = convertLepusValueToNSObject(value_);

  if ([dict isKindOfClass:[NSDictionary class]]) {
    return dict;
  } else {
    return nil;
  }
}

std::shared_ptr<lynx::tasm::TemplateData> ConvertLynxTemplateDataToTemplateData(
    LynxTemplateData* data) {
  if (!data) {
    return nullptr;
  }
  return std::make_shared<lynx::tasm::TemplateData>(
      *LynxGetLepusValueFromTemplateData(data), data.isReadOnly,
      data.processorName ? data.processorName.UTF8String : "");
}

@end
