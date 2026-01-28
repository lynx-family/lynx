// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/ios/module_factory_darwin.h"
#import <Lynx/LynxContext.h>
#import <Lynx/LynxContextModule.h>
#import <Lynx/LynxLog.h>
#include <Lynx/LynxThreadSafeDictionary.h>

#include <string>
#include <unordered_map>

#include "base/include/string/string_utils.h"

@implementation LynxModuleWrapper

@end

namespace lynx {
namespace piper {
ModuleFactoryDarwin::ModuleFactoryDarwin() : parent(nullptr) {
  modulesClasses_ = [[LynxThreadSafeDictionary alloc] init];
  extra_ = [NSMutableDictionary dictionary];
  methodAuthBlocks_ = [[NSMutableArray alloc] init];
  methodSessionBlocks_ = [[NSMutableArray alloc] init];
}

ModuleFactoryDarwin::~ModuleFactoryDarwin() {
  // destroy moduleCreator
  if (module_creator_ != nullptr) {
    module_creator_->Destroy();
  }
  LOGI("NativeModule: lynx module_factory_darwin destroy: "
       << reinterpret_cast<std::uintptr_t>(this));
}

void ModuleFactoryDarwin::Bind(std::unique_ptr<ModuleCreatorDarwin> module_creator) {
  module_creator_ = std::move(module_creator);
}

void ModuleFactoryDarwin::DeleteLynxContextForInstance(NSString *instanceId) {
  if (module_creator_) {
    module_creator_->DeleteLynxContextForInstance(instanceId);
  }
}

void ModuleFactoryDarwin::SetContextFinder(
    const std::shared_ptr<LynxContextFinderDarwin> &context_finder) {
  if (module_creator_) {
    module_creator_->SetContextFinder(context_finder);
  }
}

std::shared_ptr<lynx::piper::LynxContextFinderDarwin> ModuleFactoryDarwin::CurrentContextFinder() {
  if (module_creator_) {
    return module_creator_->CurrentContextFinder();
  }
  return nullptr;
}

std::shared_ptr<LynxNativeModule> ModuleFactoryDarwin::CreateModule(const std::string &name) {
  NSString *str = [NSString stringWithCString:name.c_str()
                                     encoding:[NSString defaultCStringEncoding]];
  if (!module_creator_) {
    LOGE("NativeModule: ModuleFactoryDarwin::CreateModule called without ModuleCreatorDarwin "
         "bound, name: "
         << name);
    return std::shared_ptr<LynxNativeModule>(nullptr);
  }

  LynxModuleWrapper *wrapper = modulesClasses_[str];
  if (wrapper == nil && parent) {
    wrapper = parent->moduleWrappers()[str];
  }

  // create lynx module instance
  id<LynxModule> instance = module_creator_->Create(str, wrapper);
  if (instance != nil) {
    if (lynxModuleExtraData_ && [instance respondsToSelector:@selector(setExtraData:)]) {
      [instance setExtraData:lynxModuleExtraData_];
    }
    // create lynx module darwin
    std::shared_ptr<lynx::piper::LynxModuleDarwin> moduleDarwin =
        std::make_shared<lynx::piper::LynxModuleDarwin>(instance);
    moduleDarwin->SetMethodAuth(methodAuthBlocks_);
    moduleDarwin->SetMethodSession(methodSessionBlocks_);
    moduleDarwin->SetContextFinder(module_creator_->CurrentContextFinder());
    if (wrapper.namescope) {
      moduleDarwin->SetMethodScope(wrapper.namescope);
    }
    {
      [[maybe_unused]] auto conformsToLynxContextModule =
          [instance conformsToProtocol:@protocol(LynxContextModule)];
      [[maybe_unused]] auto conformsToLynxModule =
          [instance conformsToProtocol:@protocol(LynxModule)];
      LOGV("NativeModule: LynxModule, module: "
           << name << "(conforming to LynxModule?: " << conformsToLynxModule
           << ", conforming to LynxContextModule?: " << conformsToLynxContextModule
           << ", with param(address): " << reinterpret_cast<std::uintptr_t>(wrapper.param) << ")"
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
  _LogI(@"NativeModule: LynxModule, module: %@ registered with param (address): %p", cls, param);
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
      LOGW("NativeModule: Duplicated LynxModule For Name: " << name << ", will be ignored");
      continue;
    }
    modulesClasses_[name] = wrappers[name];
  }
}

}  // namespace piper
}  // namespace lynx
