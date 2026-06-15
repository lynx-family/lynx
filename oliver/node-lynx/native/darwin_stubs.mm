// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

#include <memory>

#import "LynxDynamicComponentFetcher.h"

#include "base/include/value/base_value.h"
#include "core/template_bundle/lynx_template_bundle.h"

@interface LynxEnv : NSObject
@property(nonatomic, assign) BOOL devtoolEnabled;
+ (instancetype)sharedInstance;
@end

@implementation LynxEnv
+ (instancetype)sharedInstance {
  static LynxEnv* instance;
  static dispatch_once_t once_token;
  dispatch_once(&once_token, ^{
    instance = [[LynxEnv alloc] init];
  });
  return instance;
}
@end

@interface LynxExternalResourceFetcherWrapper : NSObject {
  id<LynxDynamicComponentFetcher> _component_fetcher;
}
- (instancetype)initWithDynamicComponentFetcher:(id<LynxDynamicComponentFetcher>)fetcher;
- (BOOL)fetchResource:(NSString*)url withLoadedBlock:(void (^)(NSData*, NSError*))block;
@end

@implementation LynxExternalResourceFetcherWrapper
- (instancetype)initWithDynamicComponentFetcher:(id<LynxDynamicComponentFetcher>)fetcher {
  self = [super init];
  if (self) {
    _component_fetcher = fetcher;
  }
  return self;
}

- (BOOL)fetchResource:(NSString*)url withLoadedBlock:(void (^)(NSData*, NSError*))block {
  if (!_component_fetcher ||
      ![_component_fetcher respondsToSelector:@selector(loadDynamicComponent:withLoadedBlock:)]) {
    return NO;
  }
  [_component_fetcher loadDynamicComponent:url withLoadedBlock:block];
  return YES;
}
@end

@interface LynxResourceRequest : NSObject
@property(nonatomic, readonly, copy) NSString* url;
@property(nonatomic, readonly, assign) NSInteger type;
@property(nonatomic, strong) id requestParams;
@property(nonatomic, assign) NSInteger mode;
- (instancetype)initWithUrl:(NSString*)url;
- (instancetype)initWithUrl:(NSString*)url type:(NSInteger)type;
- (instancetype)initWithUrl:(NSString*)url andRequestParams:(id)requestParams;
- (instancetype)initWithUrl:(NSString*)url type:(NSInteger)type context:(id)context;
- (id)getLynxResourceServiceRequestParams;
@end

@implementation LynxResourceRequest {
  NSString* _url;
  NSInteger _type;
}
@synthesize url = _url;
@synthesize type = _type;

- (instancetype)initWithUrl:(NSString*)url {
  return [self initWithUrl:url type:0 context:nil];
}

- (instancetype)initWithUrl:(NSString*)url type:(NSInteger)type {
  return [self initWithUrl:url type:type context:nil];
}

- (instancetype)initWithUrl:(NSString*)url andRequestParams:(id)requestParams {
  self = [self initWithUrl:url type:0 context:nil];
  if (self) {
    self.requestParams = requestParams;
  }
  return self;
}

- (instancetype)initWithUrl:(NSString*)url type:(NSInteger)type context:(id)context {
  self = [super init];
  if (self) {
    _url = [url copy];
    _type = type;
  }
  return self;
}

- (id)getLynxResourceServiceRequestParams {
  return nil;
}
@end

@interface LynxPerformanceEntryConverter : NSObject
+ (id)makePerformanceEntry:(NSDictionary*)dictionary;
+ (NSNumber*)getNumberObject:(NSDictionary*)dictionary
                        name:(NSString*)name
                defaultValue:(NSNumber*)value;
+ (NSString*)getStringObject:(NSDictionary*)dictionary
                        name:(NSString*)name
                defaultValue:(NSString*)value;
+ (BOOL)getBooleanObject:(NSDictionary*)dictionary name:(NSString*)name defaultValue:(BOOL)value;
+ (NSDictionary*)getDictionaryObject:(NSDictionary*)dictionary
                                name:(NSString*)name
                        defaultValue:(NSDictionary*)value;
@end

@implementation LynxPerformanceEntryConverter
+ (id)makePerformanceEntry:(NSDictionary*)dictionary {
  return nil;
}

+ (NSNumber*)getNumberObject:(NSDictionary*)dictionary
                        name:(NSString*)name
                defaultValue:(NSNumber*)value {
  id result = dictionary[name];
  return [result isKindOfClass:[NSNumber class]] ? result : value;
}

+ (NSString*)getStringObject:(NSDictionary*)dictionary
                        name:(NSString*)name
                defaultValue:(NSString*)value {
  id result = dictionary[name];
  return [result isKindOfClass:[NSString class]] ? result : value;
}

+ (BOOL)getBooleanObject:(NSDictionary*)dictionary name:(NSString*)name defaultValue:(BOOL)value {
  id result = dictionary[name];
  return [result respondsToSelector:@selector(boolValue)] ? [result boolValue] : value;
}

+ (NSDictionary*)getDictionaryObject:(NSDictionary*)dictionary
                                name:(NSString*)name
                        defaultValue:(NSDictionary*)value {
  id result = dictionary[name];
  return [result isKindOfClass:[NSDictionary class]] ? result : value;
}
@end

@interface LynxThreadSafeDictionary : NSMutableDictionary
@end

@implementation LynxThreadSafeDictionary {
  NSMutableDictionary* _storage;
  NSLock* _lock;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _storage = [[NSMutableDictionary alloc] init];
    _lock = [[NSLock alloc] init];
  }
  return self;
}

- (NSUInteger)count {
  [_lock lock];
  NSUInteger count = _storage.count;
  [_lock unlock];
  return count;
}

- (id)objectForKey:(id)aKey {
  [_lock lock];
  id object = [_storage objectForKey:aKey];
  [_lock unlock];
  return object;
}

- (NSEnumerator*)keyEnumerator {
  [_lock lock];
  NSArray* keys = [_storage allKeys];
  [_lock unlock];
  return [keys objectEnumerator];
}

- (void)setObject:(id)anObject forKey:(id<NSCopying>)aKey {
  [_lock lock];
  [_storage setObject:anObject forKey:aKey];
  [_lock unlock];
}

- (void)removeObjectForKey:(id)aKey {
  [_lock lock];
  [_storage removeObjectForKey:aKey];
  [_lock unlock];
}

- (id)objectForKeyedSubscript:(id)key {
  return [self objectForKey:key];
}

- (void)setObject:(id)obj forKeyedSubscript:(id<NSCopying>)key {
  [_lock lock];
  if (obj) {
    [_storage setObject:obj forKey:key];
  } else {
    [_storage removeObjectForKey:key];
  }
  [_lock unlock];
}
@end

@interface LynxTemplateData : NSObject
@end

@implementation LynxTemplateData
@end

@class LynxTemplateBundle;

lynx::lepus::Value LynxConvertToLepusValue(id data, BOOL useBoolLiterals) {
  return lynx::lepus::Value();
}

lynx::lepus::Value* LynxGetLepusValueFromTemplateData(LynxTemplateData* data) {
  static lynx::lepus::Value empty;
  return &empty;
}

std::shared_ptr<lynx::tasm::LynxTemplateBundle> LynxGetRawTemplateBundle(
    LynxTemplateBundle* bundle) {
  return nullptr;
}
