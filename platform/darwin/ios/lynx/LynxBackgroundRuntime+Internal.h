// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxBackgroundRuntime.h"
#import "LynxDevtool.h"

#include "core/inspector/observer/inspector_runtime_observer_ng.h"
#include "core/runtime/bindings/jsi/modules/ios/module_factory_darwin.h"
#include "core/shell/runtime_standalone_helper.h"

@interface LynxBackgroundRuntimeOptions ()
- (NSMutableDictionary<NSString *, id> *)moduleWrappers;
- (NSMutableDictionary<NSString *, id<LynxResourceProvider>> *)providers;
- (std::string)groupThreadName;
- (std::string)groupID;
- (BOOL)enableJSGroupThread;
- (std::vector<std::string>)preloadJSPath;
- (std::string)bytecodeUrlString;
- (instancetype)initWithOptions:(LynxBackgroundRuntimeOptions *)other;
- (void)merge:(LynxBackgroundRuntimeOptions *)other;
@end

@interface LynxBackgroundRuntime ()
- (std::weak_ptr<lynx::piper::LynxModuleManager>)moduleManagerPtr;
- (LynxBackgroundRuntimeOptions *)options;
- (std::shared_ptr<lynx::shell::LynxActor<lynx::runtime::LynxRuntime>>)runtimeActor;
- (std::shared_ptr<lynx::shell::LynxActor<lynx::tasm::timing::TimingHandler>>)timingActor;
- (LynxDevtool *)devtool;
- (BOOL)attachToLynxView;
- (void)setRuntimeObserver:
    (const std::shared_ptr<lynx::piper::InspectorRuntimeObserverNG> &)observer;
@end
