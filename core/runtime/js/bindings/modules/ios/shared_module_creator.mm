// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/ios/shared_module_creator.h"
#import <Lynx/LynxContextModule.h>
#import <Lynx/LynxLog.h>

namespace lynx {
namespace piper {

// CommonLynxContextFinderDarwin
SharedLynxContextFinderDarwin::SharedLynxContextFinderDarwin() {
  // init cache
  lynxContextMap_ = [LynxThreadSafeDictionary dictionary];
}

SharedLynxContextFinderDarwin::~SharedLynxContextFinderDarwin() {}

LynxContext* SharedLynxContextFinderDarwin::FindContext(const std::string& unique_id) {
  NSString* str = [NSString stringWithCString:unique_id.c_str()
                                     encoding:[NSString defaultCStringEncoding]];
  return [lynxContextMap_ objectForKey:str];
}

void SharedLynxContextFinderDarwin::DeleteLynxContextForInstance(NSString* instanceId) {
  _LogI(@"SharedLynxContextFinderDarwin Destroy LynxContext for %@", instanceId);
  [lynxContextMap_ removeObjectForKey:instanceId];
}

std::string SharedLynxContextFinderDarwin::FindSchema(const std::string& unique_id) {
  auto it = schemas_.find(unique_id);
  if (it == schemas_.end()) {
    return std::string();
  }
  return it->second;
}

void SharedLynxContextFinderDarwin::RegisterContext(const std::string& unique_id,
                                                    LynxContext* context,
                                                    const std::string& schema) {
  NSString* str = [NSString stringWithCString:unique_id.c_str()
                                     encoding:[NSString defaultCStringEncoding]];
  lynxContextMap_[str] = context;
  schemas_[unique_id] = schema;
}

// CommonModuleCreator
SharedModuleCreator::SharedModuleCreator() : context_finder_(nullptr) {
  moduleInstances_ = [[LynxThreadSafeDictionary alloc] init];
}

void SharedModuleCreator::DeleteLynxContextForInstance(NSString* instanceId) {
  context_finder_->DeleteLynxContextForInstance(instanceId);
}

SharedModuleCreator::~SharedModuleCreator() {}

std::shared_ptr<LynxContextFinderDarwin> SharedModuleCreator::CurrentContextFinder() {
  return context_finder_;
}

void SharedModuleCreator::SetContextFinder(
    const std::shared_ptr<LynxContextFinderDarwin>& context_finder) {
  context_finder_ = context_finder;
}

// only create lynxModule (not lynxContextModule)
id<LynxModule> SharedModuleCreator::Create(NSString* name, LynxModuleWrapper* wrapper) {
  if (wrapper == nil) {
    return nil;
  }

  id<LynxModule> instance = moduleInstances_[name];
  if (instance != nil) {
    return instance;
  }

  // Since the shared NaitveModule does not have a unique LynxContext, the initwithLynxContext
  // method will not be called here.
  Class<LynxModule> aClass = wrapper.moduleClass;
  id param = wrapper.param;
  if (aClass != nil) {
    if ([aClass conformsToProtocol:@protocol(LynxContextModule)]) {
      return nil;
    }
    id<LynxModule> instance = [(Class)aClass alloc];
    if (param != nil && [instance respondsToSelector:@selector(initWithParam:)]) {
      instance = [instance initWithParam:param];
    } else {
      instance = [instance init];
    }
    // cache module instance
    moduleInstances_[name] = instance;
    return instance;
  }
  return nil;
}

// The ModuleFactory calls Destroy during its destruction, thereby reclaiming all LynxModule
// instances.
void SharedModuleCreator::Destroy() {
#if !defined(OS_OSX)
  if (moduleInstances_ == nil) {
    return;
  }

  [moduleInstances_ enumerateKeysAndObjectsUsingBlock:^(NSString* _Nonnull name,
                                                        id _Nonnull instance, BOOL* _Nonnull stop) {
    if (instance != nil) {
      if ([instance respondsToSelector:@selector(destroy)]) {
        [instance destroy];
      }
    }
  }];

  [moduleInstances_ removeAllObjects];
#endif  // !defined(OS_OSX)
}
}  // namespace piper
}  // namespace lynx
