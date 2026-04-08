// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/NSObject+LynxScrollCoordinatorKVO.h>
#import <objc/objc.h>
#import <objc/runtime.h>

@interface LYNX_DUMMY_CLASS_NSObject_LynxScrollCoordinatorKVO : NSObject
@end

@implementation LYNX_DUMMY_CLASS_NSObject_LynxScrollCoordinatorKVO
@end

static const int kNSObjectLynxScrollCoordinatorKVOBlockKey;

@interface _LynxNSObjectKVOBlockTarget : NSObject

@property(nonatomic, copy) void (^block)(__weak id obj, id oldVal, id newVal);

- (id)initWithBlock:(void (^)(__weak id obj, id oldVal, id newVal))block;

@end

@implementation _LynxNSObjectKVOBlockTarget

- (id)initWithBlock:(void (^)(__weak id obj, id oldVal, id newVal))block {
  self = [super init];
  if (self) {
    self.block = block;
  }
  return self;
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context {
  if (!self.block) return;

  BOOL isPrior = [[change objectForKey:NSKeyValueChangeNotificationIsPriorKey] boolValue];
  if (isPrior) return;

  NSKeyValueChange changeKind = [[change objectForKey:NSKeyValueChangeKindKey] integerValue];
  if (changeKind != NSKeyValueChangeSetting) return;

  id oldVal = [change objectForKey:NSKeyValueChangeOldKey];
  if (oldVal == [NSNull null]) oldVal = nil;

  id newVal = [change objectForKey:NSKeyValueChangeNewKey];
  if (newVal == [NSNull null]) newVal = nil;

  self.block(object, oldVal, newVal);
}

@end

@implementation NSObject (LynxScrollCoordinatorKVO)

- (void)lynx_scrollCoordinator_addObserverBlockForKeyPath:(NSString *)keyPath
                                                    block:(void (^)(__weak id obj, id oldVal,
                                                                    id newVal))block {
  if (!keyPath || !block) return;
  _LynxNSObjectKVOBlockTarget *target = [[_LynxNSObjectKVOBlockTarget alloc] initWithBlock:block];
  NSMutableDictionary *dic = [self _lynx_allNSObjectObserverBlocks];
  NSMutableArray *arr = dic[keyPath];
  if (!arr) {
    arr = [NSMutableArray new];
    dic[keyPath] = arr;
  }
  [arr addObject:target];
  [self addObserver:target
         forKeyPath:keyPath
            options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld
            context:NULL];
}

- (void)lynx_scrollCoordinator_removeObserverBlocksForKeyPath:(NSString *)keyPath {
  if (!keyPath) return;
  NSMutableDictionary *dic =
      objc_getAssociatedObject(self, &kNSObjectLynxScrollCoordinatorKVOBlockKey);
  if (!dic) {
    return;
  }
  NSMutableArray *arr = dic[keyPath];
  [arr enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
    [self removeObserver:obj forKeyPath:keyPath];
  }];

  [dic removeObjectForKey:keyPath];
}

- (void)lynx_scrollCoordinator_removeObserverBlocks {
  NSMutableDictionary *dic =
      objc_getAssociatedObject(self, &kNSObjectLynxScrollCoordinatorKVOBlockKey);
  if (!dic) {
    return;
  }
  [dic enumerateKeysAndObjectsUsingBlock:^(NSString *key, NSArray *arr, BOOL *stop) {
    [arr enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
      [self removeObserver:obj forKeyPath:key];
    }];
  }];

  [dic removeAllObjects];
}

- (NSMutableDictionary *)_lynx_allNSObjectObserverBlocks {
  NSMutableDictionary *targets =
      objc_getAssociatedObject(self, &kNSObjectLynxScrollCoordinatorKVOBlockKey);
  if (!targets) {
    targets = [NSMutableDictionary new];
    objc_setAssociatedObject(self, &kNSObjectLynxScrollCoordinatorKVOBlockKey, targets,
                             OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }
  return targets;
}

@end
