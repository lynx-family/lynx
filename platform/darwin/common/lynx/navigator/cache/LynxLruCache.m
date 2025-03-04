// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxLruCache.h"
#import "LynxLruCacheNode.h"

static const char *kLynxLRUCacheQueue = "kLynxLRUCacheQueue";

@interface LynxLruCache ()

@property(nonatomic, strong) NSMapTable *dictionary;
@property(nonatomic, strong) LynxLruCacheNode *rootNode;
@property(nonatomic, strong) LynxLruCacheNode *tailNode;
@property(nonatomic) NSUInteger size;
@property(nonatomic, strong) dispatch_queue_t queue;

@end

@implementation LynxLruCache {
  LynxViewReCreateBlock createBlock;
  LynxViewEvictedBlock evictedBlock;
};

- (instancetype)initWithCapacity:(NSUInteger)capacity
                        recreate:(LynxViewReCreateBlock)recreateBlock
                     viewEvicted:(LynxViewEvictedBlock)viewEvictedBlock {
  self = [super init];
  if (self) {
    [self commonSetup];
    _capacity = capacity;
    createBlock = recreateBlock;
    evictedBlock = viewEvictedBlock;
  }
  return self;
}

- (void)commonSetup {
  _dictionary = [[NSMapTable alloc] init];
  _queue = dispatch_queue_create(kLynxLRUCacheQueue, 0);
}

#pragma mark - setObject/getObject

- (void)setObject:(id)object forKey:(id<NSCopying>)key {
  NSAssert(object != nil, @"LRUCache cannot store nil object!");

  dispatch_barrier_async(self.queue, ^{
    LynxLruCacheNode *node = [self.dictionary objectForKey:key];
    if (node == nil) {
      node = [LynxLruCacheNode nodeWithValue:object key:key];
      [self.dictionary setObject:node forKey:key];
      self.size++;

      if (self.tailNode == nil) {
        self.tailNode = node;
      }

      if (self.rootNode == nil) {
        self.rootNode = node;
      }
    }

    [self putNodeToTop:node];
    [self checkSpace];
  });
}

- (id)objectForKey:(id)key {
  __block LynxLruCacheNode *node = nil;

  dispatch_sync(self.queue, ^{
    node = [self.dictionary objectForKey:key];
    if (node == nil) {
      LynxView *lynxView = createBlock(key);
      if (lynxView) {
        node = [LynxLruCacheNode nodeWithValue:lynxView key:key];
        [self setObject:node forKey:key];
      }
    }
  });
  return node.value;
}

- (id)removeObjectForKey:(id)key {
  __block LynxLruCacheNode *node = nil;

  dispatch_sync(self.queue, ^{
    node = [self.dictionary objectForKey:key];
    [self.dictionary removeObjectForKey:key];
  });
  return node.value;
}

#pragma mark - helper methods

- (void)putNodeToTop:(LynxLruCacheNode *)node {
  if (node == self.rootNode) {
    return;
  }

  if (node == self.tailNode) {
    self.tailNode = self.tailNode.prev;
  }
  node.prev.next = node.next;
  LynxLruCacheNode *prevRoot = self.rootNode;
  self.rootNode = node;
  self.rootNode.next = prevRoot;
}

- (void)checkSpace {
  if (self.size > self.capacity) {
    LynxLruCacheNode *nextTail = self.tailNode.prev;
    LynxView *viewToBeEvicted = [self.dictionary objectForKey:self.tailNode.key];
    [self.dictionary removeObjectForKey:self.tailNode.key];
    evictedBlock(viewToBeEvicted);
    self.tailNode = nextTail;
    self.tailNode.next = nil;
    self.size--;
  }
}

@end
