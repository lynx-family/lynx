// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "LynxLRUMap.h"

@interface LynxLRUNode : NSObject

@property(nonatomic, weak) id key;
@property(nonatomic, strong) id value;
@property(nonatomic, strong) LynxLRUNode *prev;
@property(nonatomic, strong) LynxLRUNode *next;

- (instancetype)initWithKey:(id)key value:(id)value;

@end

@implementation LynxLRUNode

- (instancetype)initWithKey:(id)key value:(id)value {
  self = [super init];
  if (self) {
    self.key = key;
    self.value = value;
  }
  return self;
}

@end

@interface LynxLRUMap ()

@property(nonatomic, assign) NSUInteger capacity;
@property(nonatomic, assign) NSUInteger count;
@property(nonatomic, strong) NSMapTable *cacheDict;
@property(nonatomic, strong) LynxLRUNode *head;
@property(nonatomic, strong) LynxLRUNode *tail;

@end

@implementation LynxLRUMap

- (instancetype)initWithCapacity:(NSUInteger)capacity {
  self = [super init];
  if (self) {
    self.capacity = capacity;
    self.count = 0;
    self.cacheDict = [NSMapTable mapTableWithKeyOptions:NSPointerFunctionsStrongMemory
                                           valueOptions:NSPointerFunctionsStrongMemory];
  }
  return self;
}

- (NSUInteger)getCapacity {
  return self.capacity;
}

- (id)get:(id)key {
  LynxLRUNode *node = [self.cacheDict objectForKey:key];
  if (node) {
    [self moveToHead:node];
    return node.value;
  }
  return nil;
}

- (void)set:(id)key value:(id)value {
  LynxLRUNode *node = [self.cacheDict objectForKey:key];
  if (node) {
    node.value = value;
    [self moveToHead:node];
  } else {
    LynxLRUNode *newNode = [[LynxLRUNode alloc] initWithKey:key value:value];
    if (self.count >= self.capacity) {
      [self removeTail];
    }
    [self addToHead:newNode];
    [self.cacheDict setObject:newNode forKey:key];
    self.count++;
  }
}

- (void)moveToHead:(LynxLRUNode *)node {
  if (node == self.head) return;
  [self removeNode:node];
  [self addToHead:node];
}

- (void)addToHead:(LynxLRUNode *)node {
  node.next = self.head;
  node.prev = nil;
  if (self.head) {
    self.head.prev = node;
  }
  self.head = node;
  if (!self.tail) {
    self.tail = self.head;
  }
}

- (void)removeNode:(LynxLRUNode *)node {
  if (node.prev) {
    node.prev.next = node.next;
  } else {
    self.head = node.next;
  }
  if (node.next) {
    node.next.prev = node.prev;
  } else {
    self.tail = node.prev;
  }
}

- (void)removeTail {
  if (self.tail) {
    [self.cacheDict removeObjectForKey:self.tail.key];
    [self removeNode:self.tail];
    self.count--;
  }
}

- (NSString *)description {
  NSMutableString *desc = [NSMutableString string];
  LynxLRUNode *node = self.head;
  while (node) {
    [desc appendFormat:@"%@: %@ ", node.key, node.value];
    node = node.next;
  }
  return [desc copy];
}

@end
