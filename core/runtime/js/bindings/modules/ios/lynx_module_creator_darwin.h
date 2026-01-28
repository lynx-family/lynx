// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BINDINGS_MODULES_IOS_LYNX_MODULE_CREATOR_DARWIN_H_
#define CORE_RUNTIME_JS_BINDINGS_MODULES_IOS_LYNX_MODULE_CREATOR_DARWIN_H_

#import <Foundation/Foundation.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

@class LynxContext;
@protocol LynxModule;

@interface LynxModuleWrapper : NSObject

@property(nonatomic, readwrite, strong) Class<LynxModule> moduleClass;
@property(nonatomic, readwrite, strong) id param;
@property(nonatomic, readwrite, weak) NSString *namescope;

@end

namespace lynx {
namespace piper {

// Base class: used in ModuleFactory to find one or a group of LynxContext bound to the current
// ModuleFactory
class LynxContextFinderDarwin {
 public:
  LynxContextFinderDarwin() = default;
  virtual ~LynxContextFinderDarwin() = default;

  virtual LynxContext *FindContext(const std::string &unique_id) { return nullptr; };
  virtual std::string FindSchema(const std::string &unique_id) { return std::string(); };
  virtual void RegisterContext(const std::string &unique_id, LynxContext *context,
                               const std::string &schema){};
  virtual void DeleteLynxContextForInstance(NSString *instanceId){};
  virtual void Destroy(){};
  virtual bool IsShared() { return false; };
};

// Base class: used in ModuleFactory to create lynx module instance
class ModuleCreatorDarwin {
 public:
  ModuleCreatorDarwin() = default;
  virtual ~ModuleCreatorDarwin() = default;

  virtual id<LynxModule> Create(NSString *name, LynxModuleWrapper *modulesClasses) { return nil; };

  virtual void Destroy(){};

  virtual void DeleteLynxContextForInstance(NSString *instanceId){};

  virtual std::shared_ptr<LynxContextFinderDarwin> CurrentContextFinder() { return nullptr; };
  virtual void SetContextFinder(const std::shared_ptr<LynxContextFinderDarwin> &context_finder){};
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_BINDINGS_MODULES_IOS_LYNX_MODULE_CREATOR_DARWIN_H_
