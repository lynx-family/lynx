// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/LynxBackgroundRuntime.h>
#import <Lynx/LynxBaseConfigurator+Internal.h>
#import <Lynx/LynxDefaultLogicExecutor.h>
#import <Lynx/LynxDefines.h>
#import <Lynx/LynxEmbeddedModule.h>
#import <Lynx/LynxView.h>
#import <Lynx/LynxViewGroup+Internal.h>
#import <Lynx/LynxViewGroup.h>

static NSString *const kLynxLogicScriptPath = @"/logic.js";
static NSString *const kLynxAppServicePath = @"/app-service.js";
NSString *const kSendGlobalEvent = @"sendGlobalEvent";

@interface LynxDefaultLogicExecutor ()

@property(nonatomic, strong) LynxBackgroundRuntime *runtime;
@property(nonatomic, strong) LynxBackgroundRuntimeOptions *runtimeOptions;
@property(nonatomic, weak) LynxViewGroup *lynxViewGroup;
@property(nonatomic, strong) NSObject *initLock;

@end

@implementation LynxDefaultLogicExecutor
- (instancetype)initWithRuntimeOptions:(LynxBackgroundRuntimeOptions *)backgroundRuntimeOptions
                         lynxViewGroup:(LynxViewGroup *)lynxViewGroup
                            debuggable:(BOOL)debuggable {
  return [self init:lynxViewGroup runtimeOptions:backgroundRuntimeOptions];
}

- (instancetype)initWithTemplateBundle:(nullable LynxTemplateBundle *)bundle
              backgroundRuntimeOptions:(LynxBackgroundRuntimeOptions *)backgroundRuntimeOptions
                         lynxViewGroup:(LynxViewGroup *)lynxViewGroup
                            debuggable:(BOOL)debuggable {
  return [self init:lynxViewGroup runtimeOptions:backgroundRuntimeOptions];
}

- (instancetype)initWithLynxViewGroup:(nonnull LynxViewGroup *)lynxViewGroup {
  return [self init:lynxViewGroup runtimeOptions:nil];
}

- (instancetype)init:(nonnull LynxViewGroup *)lynxViewGroup
      runtimeOptions:(nullable LynxBackgroundRuntimeOptions *)runtimeOptions {
  if (self = [super init]) {
    _lynxViewGroup = lynxViewGroup;
    _runtimeOptions = runtimeOptions;
    _initLock = [[NSObject alloc] init];
  }
  return self;
}

- (void)initLynxBackgroundRuntimeIfNeeded {
  if (_runtime == nil) {
    @synchronized(_initLock) {
      if (_runtime == nil) {
        if (_lynxViewGroup == nil) {
          return;
        }
        LynxTemplateBundle *bundle = [_lynxViewGroup getTemplateBundleNonBlocking];
        LynxBackgroundRuntimeOptions *runtimeOptions =
            _runtimeOptions ? _runtimeOptions : [_lynxViewGroup lynxBackgroundRuntimeOptions];
        if (bundle == nil || runtimeOptions == nil) {
          return;
        }
        [runtimeOptions registerModule:LynxEmbeddedModule.class param:_lynxViewGroup];
        _runtime = [[LynxBackgroundRuntime alloc] initWithOptions:runtimeOptions];
        NSString *url = kLynxLogicScriptPath;
        NSString *bundleUrl = [bundle url];
        if (bundleUrl != nil) {
          if ([bundleUrl hasPrefix:@"/"] || [bundleUrl hasPrefix:@"http"]) {
            url = [bundleUrl stringByAppendingString:url];
          }
        }
        [_runtime evaluateTemplateBundle:url widthBundle:bundle withJSFile:kLynxAppServicePath];
      }
    }
  }
}

- (void)onLynxEvent:(LynxView *)lynxView event:(NSDictionary *)event {
  if (event[@"method"] == nil) {
    return;
  }
  [self initLynxBackgroundRuntimeIfNeeded];
  if (_runtime == nil) {
    return;
  }
  [self processEvent:lynxView event:event];
}

- (void)processEvent:(LynxView *)lynxView event:(NSDictionary *)event {
  NSString *methodName = event[@"method"];
  if ([methodName isEqualToString:kSendGlobalEvent]) {
    NSString *name = event[@"name"];
    NSMutableArray *params = [event[@"params"] mutableCopy];
    [params addObject:@([lynxView lynxViewId])];
    [_runtime sendGlobalEvent:name withParams:params];
  } else {
    NSMutableArray *args = [[NSMutableArray alloc] init];
    id argsData = event[@"args"];
    if ([argsData isKindOfClass:[LynxTemplateData class]]) {
      [args addObject:argsData];
    } else if ([argsData isKindOfClass:[NSDictionary class]]) {
      [args addObject:argsData];
    }
    [args addObject:@([lynxView lynxViewId])];
    [_runtime callFunction:@"embeddedModule" withMethod:methodName withParams:args];
  }
}

- (void)destroy {
  // TBD
}
@end
