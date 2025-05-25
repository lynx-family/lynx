// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "LRUMap.h"

@interface LRUNode : NSObject

@property(nonatomic, weak) id key;
@property(nonatomic, strong) id value;
@property(nonatomic, strong) LRUNode *prev;
@property(nonatomic, strong) LRUNode *next;

- (instancetype)initWithKey:(id)key value:(id)value;

@end

@implementation LRUNode

- (instancetype)initWithKey:(id)key value:(id)value {
  self = [super init];
  if (self) {
    self.key = key;
    self.value = value;
  }
  return self;
}

@end

@interface LRUMap ()

@property(nonatomic, assign) NSUInteger capacity;
@property(nonatomic, assign) NSUInteger count;
@property(nonatomic, strong) NSMapTable *cacheDict;
@property(nonatomic, strong) LRUNode *head;
@property(nonatomic, strong) LRUNode *tail;

@end

@implementation LRUMap

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
  LRUNode *node = [self.cacheDict objectForKey:key];
  if (node) {
    [self moveToHead:node];
    return node.value;
  }
  return nil;
}

- (void)set:(id)key value:(id)value {
  LRUNode *node = [self.cacheDict objectForKey:key];
  if (node) {
    node.value = value;
    [self moveToHead:node];
  } else {
    LRUNode *newNode = [[LRUNode alloc] initWithKey:key value:value];
    if (self.count >= self.capacity) {
      [self removeTail];
    }
    [self addToHead:newNode];
    [self.cacheDict setObject:newNode forKey:key];
    self.count++;
  }
}

- (void)moveToHead:(LRUNode *)node {
  if (node == self.head) return;
  [self removeNode:node];
  [self addToHead:node];
}

- (void)addToHead:(LRUNode *)node {
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

- (void)removeNode:(LRUNode *)node {
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
  LRUNode *node = self.head;
  while (node) {
    [desc appendFormat:@"%@: %@ ", node.key, node.value];
    node = node.next;
  }
  return [desc copy];
}

@end
