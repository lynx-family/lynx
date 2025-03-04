// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/modules/ios/module_factory_darwin.h"
#import "LynxContext.h"
#import "LynxContextModule.h"
#import "LynxLog.h"
#include "core/runtime/bindings/jsi/interceptor/ios/request_interceptor_darwin.h"
#include "lynx/platform/darwin/common/lynx/public/utils/thread_safe_dictionary/LynxThreadSafeDictionary.h"

#include <string>
#include <unordered_map>

#include "base/include/string/string_utils.h"

@implementation LynxModuleWrapper

@end

namespace lynx {
namespace piper {
ModuleFactoryDarwin::ModuleFactoryDarwin() : parent(nullptr), context(nil) {
  modulesClasses_ = [[LynxThreadSafeDictionary alloc] init];
  extra_ = [NSMutableDictionary dictionary];
  methodAuthBlocks_ = [[NSMutableArray alloc] init];
  methodSessionBlocks_ = [[NSMutableArray alloc] init];
}

ModuleFactoryDarwin::~ModuleFactoryDarwin() {
  LOGI("lynx module_factory_darwin destroy: " << reinterpret_cast<std::uintptr_t>(this));
}

std::shared_ptr<LynxNativeModule> ModuleFactoryDarwin::CreateModule(const std::string &name) {
  NSString *str = [NSString stringWithCString:name.c_str()
                                     encoding:[NSString defaultCStringEncoding]];
  LynxModuleWrapper *wrapper = modulesClasses_[str];
  if (wrapper == nil && parent) {
    wrapper = parent->moduleWrappers()[str];
  }
  Class<LynxModule> aClass = wrapper.moduleClass;
  id param = wrapper.param;
  if (aClass != nil) {
    id<LynxModule> instance = [(Class)aClass alloc];
    if ([instance conformsToProtocol:@protocol(LynxContextModule)]) {
      if (param != nil && [instance respondsToSelector:@selector(initWithLynxContext:WithParam:)]) {
        instance = [(id<LynxContextModule>)instance initWithLynxContext:context WithParam:param];
      } else {
        instance = [(id<LynxContextModule>)instance initWithLynxContext:context];
      }
    } else if (param != nil && [instance respondsToSelector:@selector(initWithParam:)]) {
      instance = [instance initWithParam:param];
    } else {
      instance = [instance init];
    }
    if (lynxModuleExtraData_ && [instance respondsToSelector:@selector(setExtraData:)]) {
      [instance setExtraData:lynxModuleExtraData_];
    }
    std::shared_ptr<lynx::piper::LynxModuleDarwin> moduleDarwin =
        std::make_shared<lynx::piper::LynxModuleDarwin>(instance);
    moduleDarwin->SetMethodAuth(methodAuthBlocks_);
    moduleDarwin->SetMethodSession(methodSessionBlocks_);
    NSString *url = [context getLynxView].url;
    if (context && url) {
      moduleDarwin->SetSchema(base::SafeStringConvert([url UTF8String]));
    }
    if (wrapper.namescope) {
      moduleDarwin->SetMethodScope(wrapper.namescope);
    }
    {
      [[maybe_unused]] auto conformsToLynxContextModule =
          [instance conformsToProtocol:@protocol(LynxContextModule)];
      [[maybe_unused]] auto conformsToLynxModule =
          [instance conformsToProtocol:@protocol(LynxModule)];
      LOGV("LynxModule, module: " << name << "(conforming to LynxModule?: " << conformsToLynxModule
                                  << ", conforming to LynxContextModule?: "
                                  << conformsToLynxContextModule << ", with param(address): "
                                  << reinterpret_cast<std::uintptr_t>(param) << ")"
                                  << ", is created in getModule()");
    }
    return moduleDarwin;
  }
  return std::shared_ptr<LynxNativeModule>(nullptr);
}

void ModuleFactoryDarwin::registerModule(Class<LynxModule> cls) { registerModule(cls, nil); }

void ModuleFactoryDarwin::registerModule(Class<LynxModule> cls, id param) {
  LynxModuleWrapper *wrapper = [[LynxModuleWrapper alloc] init];
  wrapper.moduleClass = cls;
  wrapper.param = param;
  if (param && [param isKindOfClass:[NSDictionary class]] &&
      [((NSDictionary *)param) objectForKey:@"namescope"]) {
    wrapper.namescope = [((NSDictionary *)param) objectForKey:@"namescope"];
  }
  modulesClasses_[[cls name]] = wrapper;
  _LogI(@"LynxModule, module: %@ registered with param (address): %p", cls, param);
}

void ModuleFactoryDarwin::registerMethodAuth(LynxMethodBlock block) {
  [methodAuthBlocks_ addObject:block];
}

void ModuleFactoryDarwin::registerMethodSession(LynxMethodSessionBlock block) {
  [methodSessionBlocks_ addObject:block];
}

void ModuleFactoryDarwin::registerExtraInfo(NSDictionary *extra) {
  [extra_ addEntriesFromDictionary:extra];
}

NSMutableDictionary<NSString *, id> *ModuleFactoryDarwin::moduleWrappers() {
  return modulesClasses_;
}

NSMutableArray<LynxMethodBlock> *ModuleFactoryDarwin::methodAuthWrappers() {
  return methodAuthBlocks_;
}

NSMutableArray<LynxMethodSessionBlock> *ModuleFactoryDarwin::methodSessionWrappers() {
  return methodSessionBlocks_;
}

NSMutableDictionary<NSString *, id> *ModuleFactoryDarwin::extraWrappers() { return extra_; }

void ModuleFactoryDarwin::addWrappers(NSMutableDictionary<NSString *, id> *wrappers) {
  [modulesClasses_ addEntriesFromDictionary:wrappers];
}

void ModuleFactoryDarwin::addModuleParamWrapperIfAbsent(
    NSMutableDictionary<NSString *, id> *wrappers) {
  for (NSString *name in wrappers) {
    if ([modulesClasses_ objectForKey:name]) {
      LOGW("Duplicated LynxModule For Name: " << name << ", will be ignored");
      continue;
    }
    modulesClasses_[name] = wrappers[name];
  }
}

}  // namespace piper
}  // namespace lynx
