// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#if defined(LynxElement)
#pragma push_macro("LynxElement")
#undef LynxElement
#define LYNX_RESTORE_LYNX_ELEMENT_IMPLEMENTATION 1
#endif

#import <Lynx/LynxElement.h>

#import <Lynx/LynxEngineProxy.h>
#import <Lynx/LynxTemplateRender.h>
#import <Lynx/LynxUI.h>

#if defined(LynxElement)
#pragma push_macro("LynxElement")
#undef LynxElement
#define LYNX_RESTORE_LYNX_ELEMENT_LYNXUI 1
#endif

LynxElementStatus const LynxElementStatusAlive = @"alive";
LynxElementStatus const LynxElementStatusDetached = @"detached";

static const int32_t kLynxElementQueryRoot = 0;
static const int32_t kLynxElementQueryStatus = 1;
static const int32_t kLynxElementQueryIdentity = 2;
static const int32_t kLynxElementQueryParent = 3;
static const int32_t kLynxElementQueryChildren = 4;
static const int32_t kLynxElementQueryFindById = 5;
static const int32_t kLynxElementQueryDataset = 6;
static const int32_t kLynxElementQueryAttributes = 7;
static const int32_t kLynxElementQueryAttribute = 8;
static const int32_t kLynxElementQueryJSON = 9;
static const int32_t kLynxElementQueryPosition = 10;
static const int32_t kInvalidLynxElementSign = -1;

static NSDictionary<NSString *, id> *LynxElementDictionaryOrEmpty(id value) {
  return [value isKindOfClass:NSDictionary.class] ? value : @{};
}

static int32_t LynxElementSignFromValue(id value) {
  return [value respondsToSelector:@selector(intValue)] ? [value intValue]
                                                        : kInvalidLynxElementSign;
}

static void LynxElementDispatchOnMain(dispatch_block_t block) {
  if (!block) {
    return;
  }
  if ([NSThread isMainThread]) {
    block();
    return;
  }
  dispatch_async(dispatch_get_main_queue(), block);
}

@interface LynxElement ()

+ (void)getRootWithEngineProxy:(LynxEngineProxy *)engineProxy
                templateRender:(LynxTemplateRender *)templateRender
                      callback:(void (^)(LynxElement *_Nullable root))callback;
- (instancetype)initWithEngineProxy:(LynxEngineProxy *)engineProxy
                     templateRender:(LynxTemplateRender *)templateRender
                               sign:(int32_t)sign;
- (void)collectMatchesWithFilter:(LynxElementFilter)filter
                         matches:(NSMutableArray<LynxElement *> *)matches
                        callback:(void (^)(NSArray<LynxElement *> *result))callback
    __attribute__((objc_direct));
+ (void)collectChildren:(NSArray<LynxElement *> *)children
                  index:(NSUInteger)index
                 filter:(LynxElementFilter)filter
                matches:(NSMutableArray<LynxElement *> *)matches
               callback:(void (^)(NSArray<LynxElement *> *result))callback
    __attribute__((objc_direct));
- (void)queryIdentityString:(NSString *)key
                   callback:(void (^)(NSString *_Nullable result))callback
    __attribute__((objc_direct));

@end

static void LynxElementQuerySign(LynxEngineProxy *proxy, int32_t sign, int32_t type,
                                 NSString *argument, LynxElementResultCallback callback) {
  if (!proxy) {
    if (callback) {
      LynxElementDispatchOnMain(^{
        callback(nil);
      });
    }
    return;
  }
  [proxy queryLynxElement:sign queryType:type argument:argument callback:callback];
}

static LynxElement *LynxElementFromSign(LynxEngineProxy *engineProxy,
                                        LynxTemplateRender *templateRender, int32_t sign) {
  return sign == kInvalidLynxElementSign ? nil
                                         : [[LynxElement alloc] initWithEngineProxy:engineProxy
                                                                     templateRender:templateRender
                                                                               sign:sign];
}

static void LynxElementGetPlatformView(LynxTemplateRender *templateRender, int32_t sign,
                                       void (^callback)(UIView *_Nullable view)) {
  if (!callback) {
    return;
  }
  dispatch_block_t task = ^{
    LynxUI *ui = [templateRender findUIBySign:sign];
    callback(ui.view);
  };
  if ([NSThread isMainThread]) {
    task();
  } else {
    dispatch_async(dispatch_get_main_queue(), task);
  }
}

@implementation LynxElement {
  __weak LynxEngineProxy *_engineProxy;
  __weak LynxTemplateRender *_templateRender;
  int32_t _sign;
}

+ (void)getRootWithEngineProxy:(LynxEngineProxy *)engineProxy
                templateRender:(LynxTemplateRender *)templateRender
                      callback:(void (^)(LynxElement *_Nullable root))callback {
  if (!callback) {
    return;
  }
  LynxElementQuerySign(engineProxy, 0, kLynxElementQueryRoot, nil, ^(id _Nullable result) {
    callback(LynxElementFromSign(engineProxy, templateRender, LynxElementSignFromValue(result)));
  });
}

- (instancetype)initWithEngineProxy:(LynxEngineProxy *)engineProxy
                     templateRender:(LynxTemplateRender *)templateRender
                               sign:(int32_t)sign {
  if (self = [super init]) {
    _engineProxy = engineProxy;
    _templateRender = templateRender;
    _sign = sign;
  }
  return self;
}

- (void)findNodeById:(NSString *)idSelector
            callback:(void (^)(LynxElement *_Nullable result))callback {
  if (!callback) {
    return;
  }
  LynxEngineProxy *engineProxy = _engineProxy;
  LynxTemplateRender *templateRender = _templateRender;
  LynxElementQuerySign(engineProxy, _sign, kLynxElementQueryFindById, idSelector, ^(id result) {
    callback(LynxElementFromSign(engineProxy, templateRender, LynxElementSignFromValue(result)));
  });
}

- (void)findNodeByFilter:(LynxElementFilter)filter
                callback:(void (^)(LynxElement *_Nullable result))callback {
  if (!callback) {
    return;
  }
  [self findNodesByFilter:filter
                 callback:^(NSArray<LynxElement *> *result) {
                   callback(result.count > 0 ? result.firstObject : nil);
                 }];
}

- (void)findNodesByFilter:(LynxElementFilter)filter
                 callback:(void (^)(NSArray<LynxElement *> *result))callback {
  if (!callback) {
    return;
  }
  NSMutableArray<LynxElement *> *matches = [NSMutableArray array];
  [self collectMatchesWithFilter:filter matches:matches callback:callback];
}

- (void)collectMatchesWithFilter:(LynxElementFilter)filter
                         matches:(NSMutableArray<LynxElement *> *)matches
                        callback:(void (^)(NSArray<LynxElement *> *result))callback {
  if (filter && filter(self)) {
    [matches addObject:self];
  }
  [self getChildren:^(NSArray<LynxElement *> *children) {
    [LynxElement collectChildren:children index:0 filter:filter matches:matches callback:callback];
  }];
}

+ (void)collectChildren:(NSArray<LynxElement *> *)children
                  index:(NSUInteger)index
                 filter:(LynxElementFilter)filter
                matches:(NSMutableArray<LynxElement *> *)matches
               callback:(void (^)(NSArray<LynxElement *> *result))callback {
  if (index >= children.count) {
    callback([matches copy]);
    return;
  }
  [children[index] collectMatchesWithFilter:filter
                                    matches:matches
                                   callback:^(__unused NSArray<LynxElement *> *result) {
                                     [LynxElement collectChildren:children
                                                            index:index + 1
                                                           filter:filter
                                                          matches:matches
                                                         callback:callback];
                                   }];
}

- (void)getStatus:(void (^)(LynxElementStatus result))callback {
  if (!callback) {
    return;
  }
  LynxElementQuerySign(_engineProxy, _sign, kLynxElementQueryStatus, nil, ^(id result) {
    callback([result boolValue] ? LynxElementStatusAlive : LynxElementStatusDetached);
  });
}

- (void)getSign:(void (^)(NSNumber *result))callback {
  if (callback) {
    callback(@(_sign));
  }
}

- (void)getTagName:(void (^)(NSString *_Nullable result))callback {
  [self queryIdentityString:@"tagName" callback:callback];
}

- (void)getId:(void (^)(NSString *_Nullable result))callback {
  [self queryIdentityString:@"id" callback:callback];
}

- (void)queryIdentityString:(NSString *)key
                   callback:(void (^)(NSString *_Nullable result))callback {
  if (!callback) {
    return;
  }
  LynxElementQuerySign(_engineProxy, _sign, kLynxElementQueryIdentity, nil, ^(id result) {
    id value = LynxElementDictionaryOrEmpty(result)[key];
    callback([value isKindOfClass:NSString.class] ? value : nil);
  });
}

- (void)getParent:(void (^)(LynxElement *_Nullable result))callback {
  if (!callback) {
    return;
  }
  LynxEngineProxy *engineProxy = _engineProxy;
  LynxTemplateRender *templateRender = _templateRender;
  LynxElementQuerySign(engineProxy, _sign, kLynxElementQueryParent, nil, ^(id result) {
    callback(LynxElementFromSign(engineProxy, templateRender, LynxElementSignFromValue(result)));
  });
}

- (void)getChildren:(void (^)(NSArray<LynxElement *> *result))callback {
  if (!callback) {
    return;
  }
  LynxEngineProxy *engineProxy = _engineProxy;
  LynxTemplateRender *templateRender = _templateRender;
  LynxElementQuerySign(engineProxy, _sign, kLynxElementQueryChildren, nil, ^(id result) {
    NSMutableArray<LynxElement *> *elements = [NSMutableArray array];
    if ([result isKindOfClass:NSArray.class]) {
      for (id sign in (NSArray *)result) {
        LynxElement *element =
            LynxElementFromSign(engineProxy, templateRender, LynxElementSignFromValue(sign));
        if (element) {
          [elements addObject:element];
        }
      }
    }
    callback([elements copy]);
  });
}

- (void)getChildCount:(void (^)(NSNumber *result))callback {
  if (!callback) {
    return;
  }
  [self getChildren:^(NSArray<LynxElement *> *result) {
    callback(@(result.count));
  }];
}

- (void)getRoot:(void (^)(LynxElement *_Nullable result))callback {
  [LynxElement getRootWithEngineProxy:_engineProxy
                       templateRender:_templateRender
                             callback:callback];
}

- (void)getPositionInfo:(void (^)(NSDictionary<NSString *, id> *_Nullable result))callback {
  if (!callback) {
    return;
  }
  LynxElementQuerySign(_engineProxy, _sign, kLynxElementQueryPosition, nil, ^(id result) {
    callback([result isKindOfClass:NSDictionary.class] ? result : nil);
  });
}

- (void)getDataset:(void (^)(NSDictionary<NSString *, id> *result))callback {
  if (!callback) {
    return;
  }
  LynxElementQuerySign(_engineProxy, _sign, kLynxElementQueryDataset, nil, ^(id result) {
    callback(LynxElementDictionaryOrEmpty(result));
  });
}

- (void)getAttribute:(NSString *)name callback:(void (^)(id _Nullable result))callback {
  if (callback) {
    LynxElementQuerySign(_engineProxy, _sign, kLynxElementQueryAttribute, name, callback);
  }
}

- (void)getAttributes:(void (^)(NSDictionary<NSString *, id> *result))callback {
  if (!callback) {
    return;
  }
  LynxElementQuerySign(_engineProxy, _sign, kLynxElementQueryAttributes, nil, ^(id result) {
    callback(LynxElementDictionaryOrEmpty(result));
  });
}

- (void)getPlatformView:(void (^)(UIView *_Nullable result))callback {
  LynxElementGetPlatformView(_templateRender, _sign, callback);
}

- (void)toJSONString:(void (^)(NSString *_Nullable result))callback {
  if (!callback) {
    return;
  }
  LynxElementQuerySign(_engineProxy, _sign, kLynxElementQueryJSON, nil, ^(id result) {
    callback([result isKindOfClass:NSString.class] ? result : nil);
  });
}

@end

#ifdef LYNX_RESTORE_LYNX_ELEMENT_LYNXUI
#pragma pop_macro("LynxElement")
#undef LYNX_RESTORE_LYNX_ELEMENT_LYNXUI
#endif

#ifdef LYNX_RESTORE_LYNX_ELEMENT_IMPLEMENTATION
#pragma pop_macro("LynxElement")
#undef LYNX_RESTORE_LYNX_ELEMENT_IMPLEMENTATION
#endif
