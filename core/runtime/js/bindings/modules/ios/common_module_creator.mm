// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/ios/common_module_creator.h"
#import <Lynx/LynxContextModule.h>
#import <Lynx/LynxLog.h>

namespace lynx {
namespace piper {

// CommonLynxContextFinderDarwin
CommonLynxContextFinderDarwin::CommonLynxContextFinderDarwin() : lynxContext_(nil) {}

CommonLynxContextFinderDarwin::~CommonLynxContextFinderDarwin() {}

LynxContext* CommonLynxContextFinderDarwin::FindContext(const std::string& unique_id) {
  return lynxContext_;
}

std::string CommonLynxContextFinderDarwin::FindSchema(const std::string& unique_id) {
  return schema_;
}

void CommonLynxContextFinderDarwin::RegisterContext(const std::string& unique_id,
                                                    LynxContext* context,
                                                    const std::string& schema) {
  lynxContext_ = context;
  schema_ = schema;
}

void CommonLynxContextFinderDarwin::Destroy() {
  _LogI(@"CommonLynxContextFinderDarwin Destroy LynxContext");
  lynxContext_ = nullptr;
}

// CommonModuleCreator
CommonModuleCreator::CommonModuleCreator() : context_finder_(nullptr) {
  moduleInstances_ = [[LynxThreadSafeDictionary alloc] init];
}

CommonModuleCreator::~CommonModuleCreator() {}

std::shared_ptr<LynxContextFinderDarwin> CommonModuleCreator::CurrentContextFinder() {
  return context_finder_;
}

void CommonModuleCreator::SetContextFinder(
    const std::shared_ptr<LynxContextFinderDarwin>& context_finder) {
  context_finder_ = context_finder;
}

// create module for common native module
id<LynxModule> CommonModuleCreator::Create(NSString* name, LynxModuleWrapper* wrapper) {
  if (wrapper == nullptr) {
    return nil;
  }

  id<LynxModule> instance = moduleInstances_[name];
  if (instance != nil) {
    return instance;
  }

  Class<LynxModule> aClass = wrapper.moduleClass;
  id param = wrapper.param;
  if (aClass != nil) {
    id<LynxModule> instance = [(Class)aClass alloc];
    if ([instance conformsToProtocol:@protocol(LynxContextModule)]) {
      LynxContext* context = context_finder_ ? context_finder_->FindContext("") : nil;
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
    // cache module instance
    moduleInstances_[name] = instance;
    return instance;
  }
  return nil;
}

void CommonModuleCreator::Destroy() {
#if !defined(OS_OSX)
  if (moduleInstances_ == nil) {
    return;
  }
  if (context_finder_) {
    context_finder_->Destroy();
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
