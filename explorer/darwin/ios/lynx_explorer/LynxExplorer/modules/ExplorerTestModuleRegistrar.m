// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "ExplorerTestModuleRegistrar.h"

#import <Lynx/LynxBackgroundRuntime.h>
#import <Lynx/LynxConfig.h>

#import "ExplorerJSBTestModule.h"
#import "ExplorerLynxTestModule.h"

@implementation ExplorerTestModuleRegistrar

+ (NSArray<Class> *)moduleClasses {
  return @[
    ExplorerJSBTestModule.class,
    ExplorerJSBTimingTestModule.class,
    ExplorerLynxTestModule.class,
  ];
}

+ (void)registerModulesInConfig:(LynxConfig *)config {
  for (Class moduleClass in self.moduleClasses) {
    [config registerModule:(Class<LynxModule>)moduleClass];
  }
}

+ (void)registerBackgroundModulesInOptions:(LynxBackgroundRuntimeOptions *)options {
  // The standalone platform fixture only depends on JSBTestModule. Keep the
  // page-only timing shim out of the background runtime until a fixture needs it.
  [options registerModule:ExplorerJSBTestModule.class];
}

@end
