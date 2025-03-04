// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxLruCacheNode.h"
#import <Foundation/Foundation.h>

@implementation LynxLruCacheNode

- (instancetype)initWithValue:(id)value key:(id<NSCopying>)key {
  if (value == nil) {
    return nil;
  }

  self = [super init];
  if (self) {
    _value = value;
    _key = key;
  }
  return self;
}

+ (instancetype)nodeWithValue:(id)value key:(id<NSCopying>)key {
  return [[LynxLruCacheNode alloc] initWithValue:value key:key];
}

- (void)setNext:(LynxLruCacheNode *)next {
  if (_next != next) {
    _next = next;
    next.prev = self;
  }
}

- (void)encodeWithCoder:(NSCoder *)coder {
  [coder encodeObject:self.value forKey:@"kLynxLruCacheNodeValueKey"];
  [coder encodeObject:self.key forKey:@"kLynxLruCacheNodeKey"];
  [coder encodeObject:self.next forKey:@"kLynxLruCacheNodeNext"];
  [coder encodeObject:self.prev forKey:@"kLynxLruCacheNodePre"];
}

- (instancetype)initWithCoder:(NSCoder *)coder {
  self = [super init];
  if (self) {
    _value = [coder decodeObjectForKey:@"kLynxLruCacheNodeValueKey"];
    _key = [coder decodeObjectForKey:@"kLynxLruCacheNodeKey"];
    _next = [coder decodeObjectForKey:@"kLynxLruCacheNodeNext"];
    _prev = [coder decodeObjectForKey:@"kLynxLruCacheNodePre"];
  }
  return self;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"%@ %@", self.value, self.next];
}

@end
