//
//  LynxThreadSafeDictionary.m
//  YYKit <https://github.com/ibireme/YYKit>
//
//  Created by ibireme on 14/10/21.
//  Copyright (c) 2015 ibireme.
//
//  This source code is licensed under the MIT-style license found in the
//  LICENSE file in the root directory of this source tree.
//

#import "LynxThreadSafeDictionary.h"

#define INIT(...)                         \
  self = super.init;                      \
  if (!self) return nil;                  \
  __VA_ARGS__;                            \
  if (!_dic) return nil;                  \
  _lock = [[NSRecursiveLock alloc] init]; \
  return self;

#define LOCK(...) \
  [_lock lock];   \
  __VA_ARGS__;    \
  [_lock unlock];

@implementation LynxThreadSafeDictionary {
  NSMutableDictionary *_dic;  // Subclass a class cluster...
  NSRecursiveLock *_lock;
}

#pragma mark - init

- (instancetype)init {
  INIT(_dic = [[NSMutableDictionary alloc] init]);
}

- (instancetype)initWithObjects:(NSArray *)objects forKeys:(NSArray *)keys {
  INIT(_dic = [[NSMutableDictionary alloc] initWithObjects:objects forKeys:keys]);
}

- (instancetype)initWithCapacity:(NSUInteger)capacity {
  INIT(_dic = [[NSMutableDictionary alloc] initWithCapacity:capacity]);
}

- (instancetype)initWithObjects:(const id[])objects
                        forKeys:(const id<NSCopying>[])keys
                          count:(NSUInteger)cnt {
  INIT(_dic = [[NSMutableDictionary alloc] initWithObjects:objects forKeys:keys count:cnt]);
}

- (instancetype)initWithDictionary:(NSDictionary *)otherDictionary {
  INIT(_dic = [[NSMutableDictionary alloc] initWithDictionary:otherDictionary]);
}

- (instancetype)initWithDictionary:(NSDictionary *)otherDictionary copyItems:(BOOL)flag {
  INIT(_dic = [[NSMutableDictionary alloc] initWithDictionary:otherDictionary copyItems:flag]);
}

#pragma mark - method

- (NSUInteger)count {
  LOCK(NSUInteger c = _dic.count);
  return c;
}

- (id)objectForKey:(id)aKey {
  LOCK(id o = [_dic objectForKey:aKey]);
  return o;
}

- (NSEnumerator *)keyEnumerator {
  LOCK(NSEnumerator *e = [_dic keyEnumerator]);
  return e;
}

- (NSArray *)allKeys {
  LOCK(NSArray *a = [_dic allKeys]);
  return a;
}

- (NSArray *)allKeysForObject:(id)anObject {
  LOCK(NSArray *a = [_dic allKeysForObject:anObject]);
  return a;
}

- (NSArray *)allValues {
  LOCK(NSArray *a = [_dic allValues]);
  return a;
}

- (NSString *)description {
  LOCK(NSString *d = [_dic description]);
  return d;
}

- (BOOL)isEqualToDictionary:(NSDictionary *)otherDictionary {
  if (otherDictionary == self) return YES;

  if ([otherDictionary isKindOfClass:LynxThreadSafeDictionary.class]) {
    LynxThreadSafeDictionary *other = (id)otherDictionary;
    BOOL isEqual;
    [_lock lock];
    [other->_lock lock];
    isEqual = [_dic isEqual:other->_dic];
    [_lock unlock];
    [other->_lock unlock];
    return isEqual;
  }
  return NO;
}

- (NSEnumerator *)objectEnumerator {
  LOCK(NSEnumerator *e = [_dic objectEnumerator]);
  return e;
}

- (NSArray *)objectsForKeys:(NSArray *)keys notFoundMarker:(id)marker {
  LOCK(NSArray *a = [_dic objectsForKeys:keys notFoundMarker:marker]);
  return a;
}

- (NSArray *)keysSortedByValueUsingSelector:(SEL)comparator {
  LOCK(NSArray *a = [_dic keysSortedByValueUsingSelector:comparator]);
  return a;
}

- (id)objectForKeyedSubscript:(id)key {
  LOCK(id o = [_dic objectForKeyedSubscript:key]);
  return o;
}

- (void)enumerateKeysAndObjectsUsingBlock:(void(NS_NOESCAPE ^)(id key, id obj, BOOL *stop))block {
  LOCK([_dic enumerateKeysAndObjectsUsingBlock:block]);
}

#pragma mark - mutable

- (void)removeObjectForKey:(id)aKey {
  LOCK([_dic removeObjectForKey:aKey]);
}

- (void)setObject:(id)anObject forKey:(id<NSCopying>)aKey {
  LOCK([_dic setObject:anObject forKey:aKey]);
}

- (void)addEntriesFromDictionary:(NSDictionary *)otherDictionary {
  LOCK([_dic addEntriesFromDictionary:otherDictionary]);
}

- (void)removeAllObjects {
  LOCK([_dic removeAllObjects]);
}

- (void)removeObjectsForKeys:(NSArray *)keyArray {
  LOCK([_dic removeObjectsForKeys:keyArray]);
}

- (void)setDictionary:(NSDictionary *)otherDictionary {
  LOCK([_dic setDictionary:otherDictionary]);
}

- (void)setObject:(id)obj forKeyedSubscript:(id<NSCopying>)key {
  LOCK([_dic setObject:obj forKeyedSubscript:key]);
}

#pragma mark - protocol

- (id)copyWithZone:(NSZone *)zone {
  return [self mutableCopyWithZone:zone];
}

- (id)mutableCopyWithZone:(NSZone *)zone {
  LOCK(id copiedDictionary = [[self.class allocWithZone:zone] initWithDictionary:_dic]);
  return copiedDictionary;
}

- (NSUInteger)countByEnumeratingWithState:(NSFastEnumerationState *)state
                                  objects:(id __unsafe_unretained[])stackbuf
                                    count:(NSUInteger)len {
  LOCK(NSUInteger count = [_dic countByEnumeratingWithState:state objects:stackbuf count:len]);
  return count;
}

- (BOOL)isEqual:(id)object {
  if (object == self) return YES;

  if ([object isKindOfClass:LynxThreadSafeDictionary.class]) {
    LynxThreadSafeDictionary *other = object;
    BOOL isEqual;
    [_lock lock];
    [other->_lock lock];
    isEqual = [_dic isEqual:other->_dic];
    [_lock unlock];
    [other->_lock unlock];
    return isEqual;
  }
  return NO;
}

- (NSUInteger)hash {
  LOCK(NSUInteger hash = [_dic hash]);
  return hash;
}

@end
