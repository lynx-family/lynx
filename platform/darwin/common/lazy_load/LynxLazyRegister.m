// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxLazyRegister.h"
#import <dlfcn.h>
#import <mach-o/dyld.h>
#import <mach-o/getsect.h>
#import <pthread.h>

@interface LynxTask : NSObject
@property(nonatomic, assign) BOOL repeatable;
+ (LynxTask *)taskWithData:(LynxData)data;
- (void)start;
- (void)startWithObject:(id)object;
@end

static NSMutableDictionary<NSString *, NSMutableArray<LynxTask *> *> *lynx_tasks;

@interface _LynxFunction : LynxTask
- (instancetype)initWithPointer:(const void *)pointer NS_DESIGNATED_INITIALIZER;
@end

@interface _LynxFunctionInfoData : LynxTask
@property(nonatomic, assign, readonly) LynxFunctionInfo functionInfo;
- (instancetype)initWithPointer:(const void *)pointer NS_DESIGNATED_INITIALIZER;
@end

@interface _LynxObjCMethod : LynxTask
@property(nonatomic, readonly) Class classOfMethod;
@property(nonatomic, readonly) SEL selector;
- (instancetype)initWithPointer:(const void *)pointer NS_DESIGNATED_INITIALIZER;
@end

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-designated-initializers"
@implementation LynxTask

+ (LynxTask *)taskWithData:(LynxData)data {
  LynxTask *task;
  switch (data.type) {
    case LynxTypeFunction:
      task = [[_LynxFunction alloc] initWithPointer:data.value];
      break;
    case LynxTypeObjCMethod:
      task = [[_LynxObjCMethod alloc] initWithPointer:data.value];
      break;
    case LynxTypeFunctionInfo:
      task = [[_LynxFunctionInfoData alloc] initWithPointer:data.value];
      break;
    default:
      NSAssert(NO, @"Unrecognized lynx data type: %@", @(data.type));
      break;
  }
  task.repeatable = data.repeatable;
  return task;
}

- (void)start {
  [NSException raise:NSGenericException format:@"'start' should be implemented in subclass."];
}

- (void)startWithObject:(id)object {
  [NSException raise:NSGenericException
              format:@"'startWithObject:' should be implemented in subclass."];
}

@end

@implementation _LynxFunction {
  const void *_function;
}

- (instancetype)initWithPointer:(const void *)pointer {
  if (self = [super init]) {
    _function = pointer;
  }
  return self;
}

- (void)start {
  if (_function) {
    ((void (*)(void))_function)();
  }
}

- (void)startWithObject:(id)object {
  if (_function) {
    ((void (*)(id))_function)(object);
  }
}

@end

@implementation _LynxFunctionInfoData {
  LynxFunctionInfo *_functionInfoPointer;
}

- (instancetype)initWithPointer:(const void *)pointer {
  if (self = [super init]) {
    _functionInfoPointer = (LynxFunctionInfo *)pointer;
  }
  return self;
}

- (void)start {
  if (_functionInfoPointer->function) {
    ((void (*)(void))_functionInfoPointer->function)();
  }
}

- (void)startWithObject:(id)object {
  if (_functionInfoPointer->function) {
    ((void (*)(id))_functionInfoPointer->function)(object);
  }
}

- (LynxFunctionInfo)functionInfo {
  return *_functionInfoPointer;
}

@end

@implementation _LynxObjCMethod {
  Class _class;
  SEL _selector;
}

- (instancetype)initWithPointer:(const void *)pointer {
  char *funcPointer = (char *)pointer;
  if (*(funcPointer) != '+') {
    NSAssert(NO, @"Lynx can only export class method.");
    return nil;
  }
  NSString *func = [NSString stringWithUTF8String:pointer];
  __auto_type components = [func componentsSeparatedByString:@" "];
  if (components.count != 2) {
    return nil;
  }
  if (self = [super init]) {
    NSRange range = [components[0] rangeOfString:@"("];
    NSString *classStr = nil;
    if (range.length > 0) {
      classStr = [components[0] substringWithRange:NSMakeRange(2, range.location - 2)];
      _class = NSClassFromString(classStr);
    } else {
      classStr = [components[0] stringByReplacingOccurrencesOfString:@"+[" withString:@""];
      _class = NSClassFromString(classStr);
    }
    _selector = NSSelectorFromString([components[1] stringByReplacingOccurrencesOfString:@"]"
                                                                              withString:@""]);
  }
  return self;
}
#pragma clang diagnostic pop

- (void)start {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  if ([_class respondsToSelector:_selector]) {
    [_class performSelector:_selector];
  }
#pragma clang diagnostic pop
}

- (void)startWithObject:(id)object {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  if ([_class respondsToSelector:_selector]) {
    [_class performSelector:_selector withObject:object];
  }
#pragma clang diagnostic pop
}

- (Class)classOfMethod {
  return _class;
}

- (SEL)selector {
  return _selector;
}

@end

@implementation LynxLazyRegister

static pthread_mutex_t _lock;
static pthread_mutexattr_t _attr;

typedef struct _LynxDataNode {
  LynxData *dataArray;
  unsigned long size;
  struct _LynxDataNode *next;
  struct _LynxDataNode *head;
} LynxDataNode;

LynxDataNode *_LNODE;

static void load_image(const struct mach_header *mh, intptr_t vmaddr_slide) {
  Dl_info info;
  if (dladdr(mh, &info) == 0) {
    return;
  }
  const void *header = mh;
  unsigned long size;
  LynxData *dataArray = (LynxData *)getsectiondata(header, LynxSegmentName, LynxSectionName, &size);
  if (dataArray == NULL) {
    return;
  }
  // Cannot use Objective-C methods in the function passed to _dyld_register_func_for_add_image,
  // otherwise it may cause a deadlock. Therefore, store the data in a linked list first,
  // and convert it to an Objective-C object after the function execution is completed.
  LynxDataNode *node = (LynxDataNode *)malloc(sizeof(LynxDataNode));
  node->dataArray = dataArray;
  node->size = size;
  node->next = NULL;
  node->head = NULL;
  pthread_mutex_lock(&_lock);
  if (_LNODE != NULL) {
    node->head = _LNODE->head;
    _LNODE->next = node;
  } else {
    node->head = node;
  }
  _LNODE = node;
  pthread_mutex_unlock(&_lock);
  // The dynamic library linked through dl_open() will be resolved in the next runloop
  dispatch_async(dispatch_get_main_queue(), ^{
    lynx_parse_node();
  });
}

static void lynx_parse_node(void) {
  pthread_mutex_lock(&_lock);
  if (_LNODE == NULL) {
    pthread_mutex_unlock(&_lock);
    return;
  }
  // Convert the data in the linked list to Objective-C objects
  LynxDataNode *head = _LNODE->head;
  LynxDataNode *node = head;
  while (node != NULL) {
    LynxData *dataArray = node->dataArray;

    unsigned long size = node->size;
    size_t count = size / sizeof(LynxData);
    for (size_t i = 0; i < count; i++) {
      LynxData data = dataArray[i];
      NSString *key = [NSString stringWithUTF8String:data.key];
      LynxTask *task = [LynxTask taskWithData:data];
      if (!task) {
        return;
      }
      __auto_type array = lynx_tasks[key];
      if (!array) {
        array = [NSMutableArray array];
        lynx_tasks[key] = array;
      }
      [array addObject:task];
    }
    LynxDataNode *temp = node;
    node = node->next;
    free(temp);
  }
  _LNODE = NULL;
  pthread_mutex_unlock(&_lock);
}

+ (instancetype)sharedInstance {
  static LynxLazyRegister *g = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    g = [[LynxLazyRegister alloc] init];
  });
  return g;
}

+ (void)loadLynxInitTask {
#ifndef LYNX_START_KEY
#define LYNX_START_KEY @LYNX_BASE_INIT_KEY
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    [[LynxLazyRegister sharedInstance] startTasksForKey:LYNX_START_KEY];
  });
#undef LYNX_START_KEY
#endif
}

+ (void)initialize {
  pthread_mutexattr_init(&_attr);
  pthread_mutexattr_settype(&_attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&_lock, &_attr);
  pthread_mutexattr_destroy(&_attr);
  lynx_tasks = NSMutableDictionary.new;
  [self loadImage];
}

- (instancetype)init {
  self = [super init];
  return self;
}

+ (void)loadImage {
  _dyld_register_func_for_add_image(load_image);
  lynx_parse_node();
}

- (void)startTasksForKey:(NSString *)key {
  pthread_mutex_lock(&_lock);
  if (!lynx_tasks) {
    pthread_mutex_unlock(&_lock);
    return;
  }
  NSMutableArray *taskArray = lynx_tasks[key].mutableCopy;
  pthread_mutex_unlock(&_lock);
  if (!taskArray) {
    return;
  }
  [taskArray
      enumerateObjectsWithOptions:NSEnumerationReverse
                       usingBlock:^(LynxTask *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
                         [obj start];
                         if (!obj.repeatable) {
                           [taskArray removeObject:obj];
                         }
                       }];
  pthread_mutex_lock(&_lock);
  if (taskArray.count == 0) {
    [lynx_tasks removeObjectForKey:key];
    if (lynx_tasks.count == 0) {
      lynx_tasks = nil;
    }
  } else {
    [lynx_tasks setObject:taskArray forKey:key];
  }
  pthread_mutex_unlock(&_lock);
}

@end
